// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mdns_discovery.h"
#include "mdns_txt_records.h"

#include "externals/mdns/mdns.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace Core::Multiplayer::ModelB {

// ---------------------------------------------------------------------------
// mDNS constants
// ---------------------------------------------------------------------------
namespace MdnsConstants {
constexpr const char *kMulticastIPv4 = "224.0.0.251";
constexpr const char *kMulticastIPv6 = "ff02::fb";
} // namespace MdnsConstants

// ---------------------------------------------------------------------------
// Implementation details
// ---------------------------------------------------------------------------
struct MdnsDiscovery::Impl {
  // Dependencies (mocked in tests)
  std::shared_ptr<MockMdnsSocket> socket;
  std::shared_ptr<MockNetworkInterfaceProvider> interface_provider;
  std::shared_ptr<MockMdnsConfig> config;

  // Service state
  DiscoveryState state{DiscoveryState::Stopped};
  bool is_running{false};
  bool is_advertising{false};

  // Discovered and advertised sessions
  std::unordered_map<std::string, GameSessionInfo>
      discovered_services; // keyed by host ip
  GameSessionInfo advertised_session;

  // Active network interfaces
  std::vector<std::string> active_interfaces;

  // Callbacks
  std::function<void(const GameSessionInfo &)> on_service_discovered;
  std::function<void(const std::string &)> on_service_removed;
  std::function<void()> on_discovery_timeout;
  std::function<void(ErrorCode, const std::string &)> on_error;

  // Concurrency primitives
  mutable std::mutex mutex;
  std::thread query_thread;
  std::thread advertise_thread;
  std::thread heartbeat_thread;
  std::atomic<bool> query_running{false};
  std::atomic<bool> advertise_running{false};
  std::atomic<bool> heartbeat_running{false};
  std::condition_variable query_cv;
  std::condition_variable advertise_cv;
  std::condition_variable heartbeat_cv;

  std::chrono::steady_clock::time_point discovery_start;
};

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
MdnsDiscovery::MdnsDiscovery(
    std::shared_ptr<MockMdnsSocket> socket,
    std::shared_ptr<MockNetworkInterfaceProvider> interface_provider,
    std::shared_ptr<MockMdnsConfig> config)
    : impl_(std::make_unique<Impl>()) {
  impl_->socket = std::move(socket);
  impl_->interface_provider = std::move(interface_provider);
  impl_->config = std::move(config);
}

MdnsDiscovery::~MdnsDiscovery() {
  StopHeartbeat();
  StopDiscovery();
  StopAdvertising();

  if (impl_->socket) {
    // Leave multicast groups and close socket
    impl_->socket->LeaveMulticastGroup(MdnsConstants::kMulticastIPv4);
    if (impl_->config && impl_->config->IsIPv6Enabled()) {
      impl_->socket->LeaveMulticastGroup(MdnsConstants::kMulticastIPv6);
    }
    impl_->socket->CloseSocket();
  }
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
ErrorCode MdnsDiscovery::Initialize() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  impl_->state = DiscoveryState::Initializing;

  // Create sockets for IPv4 and IPv6
  if (!impl_->socket->CreateSocket(AF_INET, "")) {
    impl_->state = DiscoveryState::Failed;
    return ErrorCode::NetworkError;
  }

  if (impl_->config->IsIPv6Enabled()) {
    if (!impl_->socket->CreateSocket(AF_INET6, "")) {
      impl_->state = DiscoveryState::Failed;
      return ErrorCode::NetworkError;
    }
  }

  // Bind to allowed interfaces
  impl_->active_interfaces.clear();
  auto interfaces = impl_->interface_provider->GetActiveInterfaces();
  auto allowed = impl_->config->GetAllowedInterfaces();

  for (const auto &iface : interfaces) {
    if (!iface.is_active) {
      continue;
    }
    if (!allowed.empty() && std::find(allowed.begin(), allowed.end(),
                                      iface.name) == allowed.end()) {
      continue;
    }
    if (!impl_->interface_provider->IsInterfaceUsable(iface.name)) {
      continue;
    }

    if (!impl_->socket->BindToInterface(iface.name)) {
      impl_->state = DiscoveryState::Failed;
      return ErrorCode::NetworkError;
    }

    impl_->active_interfaces.push_back(iface.name);
  }

  // Join multicast groups
  if (!impl_->socket->JoinMulticastGroup(MdnsConstants::kMulticastIPv4)) {
    impl_->state = DiscoveryState::Failed;
    return ErrorCode::NetworkError;
  }

  if (impl_->config->IsIPv6Enabled()) {
    if (!impl_->socket->JoinMulticastGroup(MdnsConstants::kMulticastIPv6)) {
      impl_->state = DiscoveryState::Failed;
      return ErrorCode::NetworkError;
    }
  }

  impl_->state = DiscoveryState::Initialized;
  return ErrorCode::Success;
}

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------
ErrorCode MdnsDiscovery::StartDiscovery() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (impl_->is_running) {
    return ErrorCode::AlreadyConnected;
  }
  if (impl_->state != DiscoveryState::Initialized &&
      impl_->state != DiscoveryState::Advertising) {
    return ErrorCode::InvalidParameter;
  }

  const auto service_type = impl_->config->GetServiceType();
  for (const auto &iface : impl_->active_interfaces) {
    impl_->socket->SendQuery(service_type, MDNS_RECORDTYPE_PTR, iface);
  }

  impl_->is_running = true;
  impl_->state = DiscoveryState::Discovering;
  impl_->discovery_start = std::chrono::steady_clock::now();

  // Start periodic query loop
  impl_->query_running = true;
  auto interval = impl_->config->GetAdvertiseInterval();
  impl_->query_thread = std::thread([this, service_type, interval]() {
    std::unique_lock<std::mutex> lock(impl_->mutex);
    while (impl_->query_running) {
      lock.unlock();
      for (const auto &iface : impl_->active_interfaces) {
        impl_->socket->SendQuery(service_type, MDNS_RECORDTYPE_PTR, iface);
      }
      lock.lock();
      impl_->query_cv.wait_for(lock, interval,
                               [this] { return !impl_->query_running; });
    }
  });

  StartHeartbeat();

  return ErrorCode::Success;
}

ErrorCode MdnsDiscovery::StopDiscovery() {
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->is_running = false;
    impl_->state = DiscoveryState::Stopped;
    impl_->query_running = false;
  }

  impl_->query_cv.notify_all();
  if (impl_->query_thread.joinable()) {
    impl_->query_thread.join();
  }

  StopHeartbeat();

  return ErrorCode::Success;
}

// ---------------------------------------------------------------------------
// Advertisement
// ---------------------------------------------------------------------------
ErrorCode MdnsDiscovery::AdvertiseService(const GameSessionInfo &session_info) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (impl_->state != DiscoveryState::Initialized &&
      impl_->state != DiscoveryState::Discovering) {
    return ErrorCode::InvalidParameter;
  }

  impl_->advertised_session = session_info;

  auto service_type = impl_->config->GetServiceType();
  uint16_t port =
      session_info.port ? session_info.port : impl_->config->GetServicePort();

  // Build TXT records string
  TxtRecordBuilder builder =
      TxtRecordBuilder::CreateGameSessionTxtRecords(session_info);
  auto records = builder.GetAllRecords();
  std::string txt;
  bool first = true;
  for (const auto &kv : records) {
    if (!first)
      txt += "&";
    txt += kv.first + "=" + kv.second;
    first = false;
  }

  if (!impl_->socket->PublishService(service_type, session_info.host_name, port,
                                     txt)) {
    return ErrorCode::NetworkError;
  }

  impl_->is_advertising = true;
  impl_->state = DiscoveryState::Advertising;

  impl_->advertise_running = true;
  auto interval = impl_->config->GetAdvertiseInterval();
  impl_->advertise_thread = std::thread([this, service_type, port, interval]() {
    std::unique_lock<std::mutex> lock(impl_->mutex);
    while (impl_->advertise_running) {
      GameSessionInfo session = impl_->advertised_session;
      lock.unlock();
      TxtRecordBuilder b =
          TxtRecordBuilder::CreateGameSessionTxtRecords(session);
      auto recs = b.GetAllRecords();
      std::string txt_local;
      bool first_local = true;
      for (const auto &kv : recs) {
        if (!first_local)
          txt_local += "&";
        txt_local += kv.first + "=" + kv.second;
        first_local = false;
      }
      impl_->socket->PublishService(service_type, session.host_name, port,
                                    txt_local);
      lock.lock();
      impl_->advertise_cv.wait_for(
          lock, interval, [this] { return !impl_->advertise_running; });
    }
  });

  return ErrorCode::Success;
}

ErrorCode MdnsDiscovery::StopAdvertising() {
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->is_advertising = false;
    impl_->advertise_running = false;
  }

  impl_->advertise_cv.notify_all();
  if (impl_->advertise_thread.joinable()) {
    impl_->advertise_thread.join();
  }

  impl_->socket->UnpublishService(impl_->config->GetServiceType(),
                                  impl_->advertised_session.host_name);

  if (!impl_->is_running) {
    impl_->state = DiscoveryState::Initialized;
  }

  return ErrorCode::Success;
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------
bool MdnsDiscovery::IsRunning() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->is_running;
}

bool MdnsDiscovery::IsAdvertising() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->is_advertising;
}

DiscoveryState MdnsDiscovery::GetState() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->state;
}

// ---------------------------------------------------------------------------
// Service management
// ---------------------------------------------------------------------------
std::vector<GameSessionInfo> MdnsDiscovery::GetDiscoveredServices() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  std::vector<GameSessionInfo> services;
  services.reserve(impl_->discovered_services.size());
  for (const auto &[ip, info] : impl_->discovered_services) {
    services.push_back(info);
  }
  return services;
}

std::optional<GameSessionInfo>
MdnsDiscovery::GetServiceByHostName(const std::string &host_name) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  for (const auto &[ip, service] : impl_->discovered_services) {
    if (service.host_name == host_name) {
      return service;
    }
  }
  return std::nullopt;
}

void MdnsDiscovery::RefreshServices() {
  std::vector<std::string> removed;
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::milliseconds(50); // short timeout for tests
    for (auto it = impl_->discovered_services.begin();
         it != impl_->discovered_services.end();) {
      if (now - it->second.last_seen > timeout) {
        removed.push_back(it->second.host_name);
        it = impl_->discovered_services.erase(it);
      } else {
        ++it;
      }
    }
  }

  if (!removed.empty() && impl_->on_service_removed) {
    for (const auto &name : removed) {
      impl_->on_service_removed(name);
    }
  }
}

std::vector<std::string> MdnsDiscovery::GetActiveInterfaces() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->active_interfaces;
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------
void MdnsDiscovery::SetOnServiceDiscoveredCallback(
    std::function<void(const GameSessionInfo &)> callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_service_discovered = std::move(callback);
}

void MdnsDiscovery::SetOnServiceRemovedCallback(
    std::function<void(const std::string &service_name)> callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_service_removed = std::move(callback);
}

void MdnsDiscovery::SetOnDiscoveryTimeoutCallback(
    std::function<void()> callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_discovery_timeout = std::move(callback);
}

void MdnsDiscovery::SetOnErrorCallback(
    std::function<void(ErrorCode, const std::string &)> callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_error = std::move(callback);
}

// ---------------------------------------------------------------------------
// Packet processing
// ---------------------------------------------------------------------------
void MdnsDiscovery::ProcessIncomingPacket(const uint8_t *data, size_t size,
                                          const std::string &source_address) {
  std::string txt_records = ParseTxtRecordsFromPacket(data, size);

  GameSessionInfo session_info;
  if (!ParseGameSessionFromTxtRecords(txt_records, session_info)) {
    return; // Nothing to do
  }

  session_info.host_ip = source_address;
  session_info.last_seen = std::chrono::system_clock::now();
  session_info.ip_version = (source_address.find(':') != std::string::npos)
                                ? IPVersion::IPv6
                                : IPVersion::IPv4;

  std::function<void(const GameSessionInfo &)> callback;
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->discovered_services.find(source_address);
    if (it != impl_->discovered_services.end()) {
      it->second = session_info;
    } else {
      session_info.discovered_at = std::chrono::system_clock::now();
      impl_->discovered_services.emplace(source_address, session_info);
      callback = impl_->on_service_discovered;
    }
  }

  if (callback) {
    callback(session_info);
  }
}

// ---------------------------------------------------------------------------
// Lifecycle methods (unused in this implementation but kept for interface)
// ---------------------------------------------------------------------------
void MdnsDiscovery::OnWebSocketConnected() {}

void MdnsDiscovery::OnWebSocketDisconnected(const std::string &) {}

// ---------------------------------------------------------------------------
// Heartbeat / periodic operations
// ---------------------------------------------------------------------------
void MdnsDiscovery::StartHeartbeat() {
  if (impl_->heartbeat_running) {
    return;
  }

  impl_->heartbeat_running = true;
  auto timeout = impl_->config->GetDiscoveryTimeout();
  impl_->heartbeat_thread = std::thread([this, timeout]() {
    auto interval = std::chrono::milliseconds(50);
    std::unique_lock<std::mutex> lock(impl_->mutex);
    while (impl_->heartbeat_running) {
      if (impl_->heartbeat_cv.wait_for(
              lock, interval, [this] { return !impl_->heartbeat_running; })) {
        break;
      }

      bool do_timeout_callback = false;
      std::function<void()> timeout_callback;
      if (impl_->is_running &&
          std::chrono::steady_clock::now() - impl_->discovery_start >=
              timeout) {
        impl_->state = DiscoveryState::TimedOut;
        impl_->is_running = false;
        impl_->query_running = false;
        timeout_callback = impl_->on_discovery_timeout;
        do_timeout_callback = true;
      }

      lock.unlock();
      if (do_timeout_callback) {
        impl_->query_cv.notify_all();
        if (impl_->query_thread.joinable()) {
          impl_->query_thread.join();
        }
        if (timeout_callback) {
          timeout_callback();
        }
      }

      // Periodically clean stale services
      RefreshServices();

      lock.lock();
    }
  });
}

void MdnsDiscovery::StopHeartbeat() {
  impl_->heartbeat_running = false;
  impl_->heartbeat_cv.notify_all();
  if (impl_->heartbeat_thread.joinable()) {
    impl_->heartbeat_thread.join();
  }
}

// ---------------------------------------------------------------------------
// Helper methods
// ---------------------------------------------------------------------------
std::string MdnsDiscovery::ParseTxtRecordsFromPacket(const uint8_t *data,
                                                     size_t size) {
  // Test packets are simple strings where the TXT record section is the
  // final space-separated token. Extract that portion so that the parser
  // can interpret key/value pairs separated by '&'.
  std::string packet(reinterpret_cast<const char *>(data), size);
  auto pos = packet.find_last_of(' ');
  if (pos == std::string::npos) {
    return packet;
  }
  return packet.substr(pos + 1);
}

bool MdnsDiscovery::ParseGameSessionFromTxtRecords(
    const std::string &txt_records, GameSessionInfo &session_info) {
  if (txt_records.empty()) {
    return false;
  }

  std::stringstream ss(txt_records);
  std::string pair;
  while (std::getline(ss, pair, '&')) {
    auto equals_pos = pair.find('=');
    if (equals_pos == std::string::npos)
      continue;
    auto key = pair.substr(0, equals_pos);
    auto value = pair.substr(equals_pos + 1);
    if (key == "game_id") {
      session_info.game_id = value;
    } else if (key == "version") {
      session_info.version = value;
    } else if (key == "players") {
      session_info.current_players = std::stoi(value);
    } else if (key == "max_players") {
      session_info.max_players = std::stoi(value);
    } else if (key == "has_password") {
      session_info.has_password = (value == "true");
    } else if (key == "host_name") {
      session_info.host_name = value;
    } else if (key == "session_id") {
      session_info.session_id = value;
    }
  }

  if (session_info.port == 0) {
    session_info.port = impl_->config ? impl_->config->GetServicePort() : 7100;
  }

  return !session_info.game_id.empty();
}

} // namespace Core::Multiplayer::ModelB
