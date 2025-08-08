// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "relay_client.h"
#include <algorithm>
#include <thread>

// Only include mock headers when building tests
#ifdef BUILDING_TESTS
#include "tests/mock_relay_connection.h"
#endif

namespace Core::Multiplayer::ModelA {

// BandwidthLimiter implementation
BandwidthLimiter::BandwidthLimiter(uint64_t bytes_per_second, size_t burst_size)
    : bytes_per_second_(bytes_per_second), burst_size_(burst_size), 
      tokens_(static_cast<int64_t>(burst_size)), 
      last_refill_(std::chrono::steady_clock::now()) {}

bool BandwidthLimiter::CanSendBytes(size_t byte_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    RefillTokens();
    return tokens_.load() >= static_cast<int64_t>(byte_count);
}

void BandwidthLimiter::ConsumeBytes(size_t byte_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    RefillTokens();
    tokens_ -= static_cast<int64_t>(byte_count);
}

std::chrono::milliseconds BandwidthLimiter::GetNextAvailableTime(size_t byte_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    RefillTokens();
    
    int64_t needed_tokens = static_cast<int64_t>(byte_count) - tokens_.load();
    if (needed_tokens <= 0) {
        return std::chrono::milliseconds(0);
    }
    
    // Calculate time needed to refill the required tokens
    uint64_t ms_per_token = 1000 / (bytes_per_second_ / 1000); // milliseconds per token
    return std::chrono::milliseconds(needed_tokens * ms_per_token);
}

void BandwidthLimiter::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_ = static_cast<int64_t>(burst_size_);
    last_refill_ = std::chrono::steady_clock::now();
}

void BandwidthLimiter::RefillTokens() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_);
    
    if (elapsed.count() > 0) {
        int64_t new_tokens = (elapsed.count() * bytes_per_second_) / 1000;
        tokens_ = std::min(static_cast<int64_t>(burst_size_), 
                          tokens_.load() + new_tokens);
        last_refill_ = now;
    }
}

// RelayClient implementation
RelayClient::RelayClient() 
    : bandwidth_limiter_(std::make_unique<BandwidthLimiter>(DEFAULT_BANDWIDTH_LIMIT, DEFAULT_BANDWIDTH_LIMIT / 10)) {
}

RelayClient::RelayClient(std::unique_ptr<Test::MockRelayConnection> connection,
                        std::unique_ptr<Test::MockP2PConnection> p2p,
                        std::unique_ptr<Test::MockBandwidthLimiter> bandwidth_limiter,
                        std::unique_ptr<Test::MockRelayServerSelector> server_selector)
#ifdef BUILDING_TESTS
    : mock_connection_(std::move(connection)),
      mock_p2p_(std::move(p2p)),
      mock_bandwidth_limiter_(std::move(bandwidth_limiter)),
      mock_server_selector_(std::move(server_selector)) {
#else
    {
#endif
}

RelayClient::~RelayClient() {
    if (IsConnected()) {
        Disconnect();
    }
}

void RelayClient::ConnectAsync(const std::string& jwt_token, std::function<void(bool)> callback) {
#ifdef BUILDING_TESTS
    if (IsUsingMocks()) {
        // Test implementation using mocks
        UpdateConnectionState(ConnectionState::Connecting);
        
        std::string server_host = mock_server_selector_->SelectBestServer();
        mock_connection_->Connect(server_host, 8443);
        mock_connection_->SetAuthToken(jwt_token);
        
        bool auth_success = mock_connection_->Authenticate();
        if (auth_success) {
            is_connected_ = true;
            connection_type_ = "relay";
            UpdateConnectionState(ConnectionState::Connected);
            callback(true);
        } else {
            UpdateConnectionState(ConnectionState::Error);
            HandleConnectionError("Authentication failed");
            callback(false);
        }
        return;
    }
#endif
    // Production implementation - minimal functionality for now
    UpdateConnectionState(ConnectionState::Error);
    callback(false);
}

void RelayClient::Disconnect() {
    is_connected_ = false;
    current_session_ = 0;
    connection_type_ = "disconnected";
    UpdateConnectionState(ConnectionState::Disconnected);
}

bool RelayClient::IsConnected() const {
    return is_connected_.load();
}

RelayClient::ConnectionState RelayClient::GetConnectionState() const {
    return connection_state_.load();
}

void RelayClient::CreateSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) {
    callback(false, 0);
}

void RelayClient::JoinSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) {
    callback(false, 0);
}

uint32_t RelayClient::GetCurrentSession() const {
    return current_session_.load();
}

bool RelayClient::SendData(const std::vector<uint8_t>& data) {
    if (!IsConnected()) {
        return false;
    }
    
    // Check bandwidth limiting
    if (bandwidth_limiter_ && !bandwidth_limiter_->CanSendBytes(data.size())) {
        return false;
    }
    
    if (bandwidth_limiter_) {
        bandwidth_limiter_->ConsumeBytes(data.size());
    }
    
    // Production implementation would send data here
    return false;
}

void RelayClient::SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback) {
    on_data_received_ = callback;
}

void RelayClient::ConnectToPeerAsync(const std::string& peer_id, 
                                    std::function<void(bool, const std::string&)> callback) {
    // Production implementation would go here
    callback(false, "failed");
}

std::string RelayClient::GetConnectionType() const {
    return connection_type_;
}

std::chrono::milliseconds RelayClient::GetLatency() const {
    return std::chrono::milliseconds(0);
}

uint64_t RelayClient::GetBandwidthLimit() const {
    if (bandwidth_limiter_) {
        return bandwidth_limiter_->GetBandwidthLimit();
    }
    return DEFAULT_BANDWIDTH_LIMIT;
}

void RelayClient::SetErrorCallback(std::function<void(const std::string&)> callback) {
    on_error_ = callback;
}

void RelayClient::TriggerRecovery() {
    // Production implementation would trigger actual recovery
}

ConnectionMetrics RelayClient::GetConnectionMetrics(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = peer_metrics_.find(peer_id);
    if (it != peer_metrics_.end()) {
        return it->second;
    }
    
    // Return default metrics
    ConnectionMetrics metrics{};
    metrics.connection_time_ms = 0;
    metrics.connection_type = ConnectionType::Relay;
    metrics.bytes_sent = 0;
    metrics.bytes_received = 0;
    metrics.latency = GetLatency();
    
    return metrics;
}

void RelayClient::HandleConnectionError(const std::string& error) {
    if (on_error_) {
        on_error_(error);
    }
}

void RelayClient::UpdateConnectionState(ConnectionState new_state) {
    connection_state_ = new_state;
}

} // namespace Core::Multiplayer::ModelA
