// SPDX-FileCopyrightText: 2025 sudachi Emulator Project  
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <map>
#include "i_relay_client.h"
#include "relay_types.h"

namespace Core::Multiplayer::ModelA {

// Forward declarations for test support
namespace Test {
    class MockRelayConnection;
    class MockP2PConnection;
    class MockBandwidthLimiter;
    class MockRelayServerSelector;
}

/**
 * Token bucket bandwidth limiter implementation
 */
class BandwidthLimiter {
public:
    BandwidthLimiter(uint64_t bytes_per_second, size_t burst_size);
    bool CanSendBytes(size_t byte_count);
    void ConsumeBytes(size_t byte_count);
    std::chrono::milliseconds GetNextAvailableTime(size_t byte_count);
    uint64_t GetBandwidthLimit() const { return bytes_per_second_; }
    void Reset();

private:
    uint64_t bytes_per_second_;
    size_t burst_size_;
    std::atomic<int64_t> tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
    
    void RefillTokens();
};

/**
 * RelayClient implementation
 * Handles connections to relay servers with bandwidth limiting and P2P fallback
 */
class RelayClient : public IRelayClient {
public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Authenticating,
        Authenticated,
        Error
    };

    // Constructor for production use
    RelayClient();
    
    // Constructor for dependency injection (tests)
    RelayClient(std::unique_ptr<Test::MockRelayConnection> connection,
                std::unique_ptr<Test::MockP2PConnection> p2p,
                std::unique_ptr<Test::MockBandwidthLimiter> bandwidth_limiter,
                std::unique_ptr<Test::MockRelayServerSelector> server_selector);
    
    ~RelayClient() override;

    // Connection management
    void ConnectAsync(const std::string& jwt_token, std::function<void(bool)> callback) override;
    void Disconnect() override;
    bool IsConnected() const override;
    ConnectionState GetConnectionState() const;

    // Session management
    void CreateSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) override;
    void JoinSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) override;
    uint32_t GetCurrentSession() const override;

    // Data transmission
    bool SendData(const std::vector<uint8_t>& data) override;
    void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback) override;

    // P2P fallback
    void ConnectToPeerAsync(const std::string& peer_id, 
                           std::function<void(bool, const std::string&)> callback) override;
    std::string GetConnectionType() const override;

    // Performance and monitoring
    std::chrono::milliseconds GetLatency() const override;
    uint64_t GetBandwidthLimit() const override;
    void SetErrorCallback(std::function<void(const std::string&)> callback) override;
    void TriggerRecovery() override;
    ConnectionMetrics GetConnectionMetrics(const std::string& peer_id) const override;

private:
    // Connection state
    std::atomic<ConnectionState> connection_state_{ConnectionState::Disconnected};
    std::atomic<uint32_t> current_session_{0};
    std::atomic<bool> is_connected_{false};
    std::string connection_type_{"disconnected"};
    
    // Bandwidth limiting (10 Mbps = 10 * 1024 * 1024 bytes/second)
    static constexpr uint64_t DEFAULT_BANDWIDTH_LIMIT = 10 * 1024 * 1024;
    std::unique_ptr<BandwidthLimiter> bandwidth_limiter_;
    
    // Mock dependencies for testing (only used when BUILDING_TESTS is defined)
#ifdef BUILDING_TESTS
    std::unique_ptr<Test::MockRelayConnection> mock_connection_;
    std::unique_ptr<Test::MockP2PConnection> mock_p2p_;
    std::unique_ptr<Test::MockBandwidthLimiter> mock_bandwidth_limiter_;
    std::unique_ptr<Test::MockRelayServerSelector> mock_server_selector_;
#endif
    
    // Callbacks
    std::function<void(const std::vector<uint8_t>&)> on_data_received_;
    std::function<void(const std::string&)> on_error_;
    
    // Connection metrics tracking
    mutable std::mutex metrics_mutex_;
    std::map<std::string, ConnectionMetrics> peer_metrics_;
    
    // Thread safety
    mutable std::mutex state_mutex_;
    
    // Helper methods
    void HandleConnectionError(const std::string& error);
    void UpdateConnectionState(ConnectionState new_state);
    bool IsUsingMocks() const { 
#ifdef BUILDING_TESTS
        return mock_connection_ != nullptr;
#else
        return false;
#endif
    }
};

} // namespace Core::Multiplayer::ModelA