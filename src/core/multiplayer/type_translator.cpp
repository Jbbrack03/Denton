// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "type_translator.h"

#include <algorithm>
#include <cstring>
#include <random>

#include "sudachi/src/core/hle/service/ldn/ldn_types.h"

namespace Core::Multiplayer::HLE {

/**
 * Concrete Type Translator Implementation
 * Minimal implementation to make tests pass
 */
class ConcreteTypeTranslator : public TypeTranslator {
public:
  Service::LDN::NetworkInfo
  ToLdnNetworkInfo(const InternalNetworkInfo &internal) override {
    Service::LDN::NetworkInfo ldn{};

    // Convert basic network properties
    ldn.network_id.intent_id.local_communication_id =
        internal.local_communication_id;
    ldn.common.channel =
        static_cast<Service::LDN::WifiChannel>(internal.channel);
    ldn.common.link_level =
        static_cast<Service::LDN::LinkLevel>(internal.link_level);
    ldn.ldn.node_count = internal.node_count;
    ldn.ldn.node_count_max = internal.node_count_max;

    // Set SSID from network name
    if (!internal.network_name.empty()) {
      ldn.common.ssid = Service::LDN::Ssid(internal.network_name);
    }

    // Convert advertise data
    if (!internal.advertise_data.empty()) {
      size_t copy_size =
          std::min(internal.advertise_data.size(),
                   static_cast<size_t>(Service::LDN::AdvertiseDataSizeMax));
      std::memcpy(ldn.ldn.advertise_data.data(), internal.advertise_data.data(),
                  copy_size);
      ldn.ldn.advertise_data_size = static_cast<uint16_t>(copy_size);
    }

    return ldn;
  }

  InternalNetworkInfo
  FromLdnNetworkInfo(const Service::LDN::NetworkInfo &ldn) override {
    InternalNetworkInfo internal{};

    // Convert basic properties
    internal.network_name = ldn.common.ssid.GetStringValue();
    internal.local_communication_id =
        ldn.network_id.intent_id.local_communication_id;
    internal.channel = static_cast<uint16_t>(ldn.common.channel);
    internal.node_count = ldn.ldn.node_count;
    internal.node_count_max = ldn.ldn.node_count_max;
    internal.link_level = static_cast<int8_t>(ldn.common.link_level);

    // Convert advertise data
    if (ldn.ldn.advertise_data_size > 0) {
      internal.advertise_data.resize(ldn.ldn.advertise_data_size);
      std::memcpy(internal.advertise_data.data(), ldn.ldn.advertise_data.data(),
                  ldn.ldn.advertise_data_size);
    }

    // Determine if password protected
    internal.has_password =
        (ldn.ldn.security_mode != Service::LDN::SecurityMode::All);

    return internal;
  }

  Service::LDN::NodeInfo
  ToLdnNodeInfo(const InternalNodeInfo &internal) override {
    Service::LDN::NodeInfo ldn{};

    ldn.node_id = static_cast<int8_t>(internal.node_id);
    ldn.is_connected = internal.is_connected ? 1 : 0;
    ldn.local_communication_version =
        static_cast<int16_t>(internal.local_communication_version);

    // Convert user name
    size_t name_size =
        std::min(internal.user_name.size(),
                 static_cast<size_t>(Service::LDN::UserNameBytesMax));
    std::memcpy(ldn.user_name.data(), internal.user_name.data(), name_size);

    // Convert MAC address
    std::memcpy(ldn.mac_address.raw.data(), internal.mac_address.data(), 6);

    // Convert IPv4 address
    std::memcpy(ldn.ipv4_address.data(), internal.ipv4_address.data(), 4);

    return ldn;
  }

  InternalNodeInfo FromLdnNodeInfo(const Service::LDN::NodeInfo &ldn) override {
    InternalNodeInfo internal{};

    internal.node_id = static_cast<uint8_t>(ldn.node_id);
    internal.is_connected = (ldn.is_connected != 0);
    internal.local_communication_version =
        static_cast<uint16_t>(ldn.local_communication_version);

    // Convert user name
    const char *name_data =
        reinterpret_cast<const char *>(ldn.user_name.data());
    const char *name_end =
        std::find(name_data, name_data + Service::LDN::UserNameBytesMax, '\0');
    internal.user_name.assign(name_data, name_end);

    // Convert MAC address
    std::memcpy(internal.mac_address.data(), ldn.mac_address.raw.data(), 6);

    // Convert IPv4 address
    std::memcpy(internal.ipv4_address.data(), ldn.ipv4_address.data(), 4);

    return internal;
  }

  Service::LDN::SessionId
  ToLdnSessionId(const std::vector<uint8_t> &internal_id) override {
    Service::LDN::SessionId ldn_id{};

    if (internal_id.size() >= sizeof(ldn_id)) {
      std::memcpy(&ldn_id, internal_id.data(), sizeof(ldn_id));
    }

    return ldn_id;
  }

  std::vector<uint8_t>
  FromLdnSessionId(const Service::LDN::SessionId &ldn_id) override {
    std::vector<uint8_t> internal_id(sizeof(ldn_id));
    std::memcpy(internal_id.data(), &ldn_id, sizeof(ldn_id));
    return internal_id;
  }

  std::vector<Service::LDN::NetworkInfo> ToLdnScanResults(
      const std::vector<InternalScanResult> &internal_results) override {

    std::vector<Service::LDN::NetworkInfo> ldn_results;
    ldn_results.reserve(internal_results.size());

    for (const auto &internal_result : internal_results) {
      auto ldn_info = ToLdnNetworkInfo(internal_result.network);

      // Store RSSI in link level (approximate conversion)
      if (internal_result.rssi >= -40) {
        ldn_info.common.link_level = Service::LDN::LinkLevel::Excellent;
      } else if (internal_result.rssi >= -60) {
        ldn_info.common.link_level = Service::LDN::LinkLevel::Good;
      } else if (internal_result.rssi >= -80) {
        ldn_info.common.link_level = Service::LDN::LinkLevel::Low;
      } else {
        ldn_info.common.link_level = Service::LDN::LinkLevel::Bad;
      }

      ldn_results.push_back(ldn_info);
    }

    return ldn_results;
  }

  std::vector<InternalScanResult> FromLdnScanResults(
      const std::vector<Service::LDN::NetworkInfo> &ldn_results) override {

    std::vector<InternalScanResult> internal_results;
    internal_results.reserve(ldn_results.size());

    for (const auto &ldn_info : ldn_results) {
      InternalScanResult internal_result{};
      internal_result.network = FromLdnNetworkInfo(ldn_info);

      // Convert link level to approximate RSSI
      switch (ldn_info.common.link_level) {
      case Service::LDN::LinkLevel::Excellent:
        internal_result.rssi = -30;
        break;
      case Service::LDN::LinkLevel::Good:
        internal_result.rssi = -50;
        break;
      case Service::LDN::LinkLevel::Low:
        internal_result.rssi = -70;
        break;
      case Service::LDN::LinkLevel::Bad:
      default:
        internal_result.rssi = -90;
        break;
      }

      internal_result.timestamp = 0;             // Will be set by backend
      internal_result.platform_info = "Unknown"; // Will be set by backend

      internal_results.push_back(internal_result);
    }

    return internal_results;
  }

  Service::LDN::MacAddress
  ToLdnMacAddress(const std::array<uint8_t, 6> &internal_mac) override {
    Service::LDN::MacAddress ldn_mac{};
    std::memcpy(ldn_mac.raw.data(), internal_mac.data(), 6);
    return ldn_mac;
  }

  std::array<uint8_t, 6>
  FromLdnMacAddress(const Service::LDN::MacAddress &ldn_mac) override {
    std::array<uint8_t, 6> internal_mac{};
    std::memcpy(internal_mac.data(), ldn_mac.raw.data(), 6);
    return internal_mac;
  }

  Service::LDN::Ipv4Address
  ToLdnIpv4Address(const std::array<uint8_t, 4> &internal_ip) override {
    Service::LDN::Ipv4Address ldn_ip{};
    std::memcpy(ldn_ip.data(), internal_ip.data(), 4);
    return ldn_ip;
  }

  std::array<uint8_t, 4>
  FromLdnIpv4Address(const Service::LDN::Ipv4Address &ldn_ip) override {
    std::array<uint8_t, 4> internal_ip{};
    std::memcpy(internal_ip.data(), ldn_ip.data(), 4);
    return internal_ip;
  }

  Service::LDN::CreateNetworkConfig
  ToLdnCreateConfig(const InternalSessionInfo &internal) override {
    Service::LDN::CreateNetworkConfig config{};

    // Set network config
    config.network_config.intent_id.local_communication_id =
        internal.local_communication_id;
    config.network_config.intent_id.scene_id = internal.scene_id;

    // Set security config
    config.security_config.security_mode =
        static_cast<Service::LDN::SecurityMode>(internal.security_mode);

    if (!internal.passphrase.empty()) {
      size_t pass_size =
          std::min(internal.passphrase.size(),
                   static_cast<size_t>(Service::LDN::PassphraseLengthMax));
      std::memcpy(config.security_config.passphrase.data(),
                  internal.passphrase.data(), pass_size);
      config.security_config.passphrase_size = static_cast<uint16_t>(pass_size);
    }

    return config;
  }

  InternalSessionInfo
  FromLdnCreateConfig(const Service::LDN::CreateNetworkConfig &ldn) override {
    InternalSessionInfo internal{};

    internal.local_communication_id =
        ldn.network_config.intent_id.local_communication_id;
    internal.scene_id = ldn.network_config.intent_id.scene_id;
    internal.security_mode =
        static_cast<uint8_t>(ldn.security_config.security_mode);

    // Extract passphrase
    if (ldn.security_config.passphrase_size > 0) {
      internal.passphrase = std::string(
          reinterpret_cast<const char *>(ldn.security_config.passphrase.data()),
          ldn.security_config.passphrase_size);
    }

    // Generate cryptographically secure random session ID
    internal.session_id.resize(16);
    std::random_device rd;
    for (auto &byte : internal.session_id) {
      byte = static_cast<uint8_t>(rd());
    }

    return internal;
  }

  Service::LDN::SecurityParameter
  ToLdnSecurityParameter(const std::vector<uint8_t> &internal) override {
    Service::LDN::SecurityParameter param{};

    if (internal.size() >= sizeof(param.data)) {
      std::memcpy(param.data.data(), internal.data(), sizeof(param.data));
    }

    return param;
  }

  std::vector<uint8_t> FromLdnSecurityParameter(
      const Service::LDN::SecurityParameter &ldn) override {
    std::vector<uint8_t> internal(sizeof(ldn.data) + sizeof(ldn.session_id));

    std::memcpy(internal.data(), ldn.data.data(), sizeof(ldn.data));
    std::memcpy(internal.data() + sizeof(ldn.data), &ldn.session_id,
                sizeof(ldn.session_id));

    return internal;
  }

  Service::LDN::State ToLdnState(const std::string &internal_state) override {
    if (internal_state == "None" || internal_state == "Uninitialized") {
      return Service::LDN::State::None;
    } else if (internal_state == "Initialized") {
      return Service::LDN::State::Initialized;
    } else if (internal_state == "AccessPointOpened") {
      return Service::LDN::State::AccessPointOpened;
    } else if (internal_state == "AccessPointCreated" ||
               internal_state == "Hosting") {
      return Service::LDN::State::AccessPointCreated;
    } else if (internal_state == "StationOpened") {
      return Service::LDN::State::StationOpened;
    } else if (internal_state == "StationConnected" ||
               internal_state == "Connected") {
      return Service::LDN::State::StationConnected;
    } else {
      return Service::LDN::State::Error;
    }
  }

  std::string FromLdnState(Service::LDN::State ldn_state) override {
    switch (ldn_state) {
    case Service::LDN::State::None:
      return "None";
    case Service::LDN::State::Initialized:
      return "Initialized";
    case Service::LDN::State::AccessPointOpened:
      return "AccessPointOpened";
    case Service::LDN::State::AccessPointCreated:
      return "AccessPointCreated";
    case Service::LDN::State::StationOpened:
      return "StationOpened";
    case Service::LDN::State::StationConnected:
      return "StationConnected";
    case Service::LDN::State::Error:
    default:
      return "Error";
    }
  }

  bool ValidateLdnNetworkInfo(const Service::LDN::NetworkInfo &info) override {
    // Validate required fields
    if (info.network_id.intent_id.local_communication_id == 0) {
      return false;
    }

    if (info.ldn.node_count > info.ldn.node_count_max) {
      return false;
    }

    if (info.ldn.node_count_max > Service::LDN::NodeCountMax) {
      return false;
    }

    if (info.ldn.advertise_data_size > Service::LDN::AdvertiseDataSizeMax) {
      return false;
    }

    return true;
  }

  bool ValidateInternalNetworkInfo(const InternalNetworkInfo &info) override {
    if (info.local_communication_id == 0) {
      return false;
    }

    if (info.network_name.empty()) {
      return false;
    }

    if (info.node_count > info.node_count_max) {
      return false;
    }

    if (info.advertise_data.size() > Service::LDN::AdvertiseDataSizeMax) {
      return false;
    }

    return true;
  }
};

} // namespace Core::Multiplayer::HLE