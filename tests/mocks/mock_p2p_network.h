// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <chrono>
#include <optional>

namespace Sudachi::Multiplayer::Tests {

/**
 * P2P connection states for testing
 */
enum class P2PConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Failed,
    Relaying  // Using relay fallback
};

/**
 * NAT traversal results for testing
 */
enum class NATTraversalResult {
    Success,
    Failed,
    NotNeeded,
    RelayRequired
};

/**
 * P2P Network interface for mocking
 */
class IP2PNetwork {
public:
    using DataCallback = std::function<void(const std::string& peer_id, const std::vector<uint8_t>& data)>;
    using StateCallback = std::function<void(const std::string& peer_id, P2PConnectionState state)>;
    using ErrorCallback = std::function<void(const std::string& peer_id, const std::string& error)>;

    virtual ~IP2PNetwork() = default;
    
    // Lifecycle management
    virtual bool Initialize(const std::string& local_peer_id) = 0;
    virtual void Shutdown() = 0;
    
    // Connection management
    virtual bool Connect(const std::string& peer_id, const std::string& multiaddr) = 0;
    virtual void Disconnect(const std::string& peer_id) = 0;
    virtual P2PConnectionState GetConnectionState(const std::string& peer_id) const = 0;
    virtual std::vector<std::string> GetConnectedPeers() const = 0;
    
    // NAT traversal
    virtual NATTraversalResult CheckNATTraversal(const std::string& peer_id) = 0;
    virtual void EnableRelay(const std::string& relay_server_addr) = 0;
    
    // Data transfer
    virtual bool SendData(const std::string& peer_id, const std::vector<uint8_t>& data) = 0;
    virtual void BroadcastData(const std::vector<uint8_t>& data) = 0;
    
    // Configuration
    virtual void SetTransportPriority(const std::vector<std::string>& transports) = 0;
    virtual void SetMaxConnections(size_t max_connections) = 0;
    virtual void SetConnectionTimeout(std::chrono::milliseconds timeout) = 0;
    
    // Callbacks
    virtual void SetOnDataReceived(DataCallback callback) = 0;
    virtual void SetOnStateChanged(StateCallback callback) = 0;
    virtual void SetOnError(ErrorCallback callback) = 0;
    
    // Metrics
    virtual size_t GetBytesReceived(const std::string& peer_id) const = 0;
    virtual size_t GetBytesSent(const std::string& peer_id) const = 0;
    virtual std::chrono::milliseconds GetLatency(const std::string& peer_id) const = 0;
};

/**
 * Mock P2P Network implementation
 */
class MockP2PNetwork : public IP2PNetwork {
public:
    MOCK_METHOD(bool, Initialize, (const std::string& local_peer_id), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    
    MOCK_METHOD(bool, Connect, (const std::string& peer_id, const std::string& multiaddr), (override));
    MOCK_METHOD(void, Disconnect, (const std::string& peer_id), (override));
    MOCK_METHOD(P2PConnectionState, GetConnectionState, (const std::string& peer_id), (const, override));
    MOCK_METHOD(std::vector<std::string>, GetConnectedPeers, (), (const, override));
    
    MOCK_METHOD(NATTraversalResult, CheckNATTraversal, (const std::string& peer_id), (override));
    MOCK_METHOD(void, EnableRelay, (const std::string& relay_server_addr), (override));
    
    MOCK_METHOD(bool, SendData, (const std::string& peer_id, const std::vector<uint8_t>& data), (override));
    MOCK_METHOD(void, BroadcastData, (const std::vector<uint8_t>& data), (override));
    
    MOCK_METHOD(void, SetTransportPriority, (const std::vector<std::string>& transports), (override));
    MOCK_METHOD(void, SetMaxConnections, (size_t max_connections), (override));
    MOCK_METHOD(void, SetConnectionTimeout, (std::chrono::milliseconds timeout), (override));
    
    MOCK_METHOD(void, SetOnDataReceived, (DataCallback callback), (override));
    MOCK_METHOD(void, SetOnStateChanged, (StateCallback callback), (override));
    MOCK_METHOD(void, SetOnError, (ErrorCallback callback), (override));
    
    MOCK_METHOD(size_t, GetBytesReceived, (const std::string& peer_id), (const, override));
    MOCK_METHOD(size_t, GetBytesSent, (const std::string& peer_id), (const, override));
    MOCK_METHOD(std::chrono::milliseconds, GetLatency, (const std::string& peer_id), (const, override));

    // Test helpers
    void SimulateIncomingData(const std::string& peer_id, const std::vector<uint8_t>& data) {
        if (on_data_received_) {
            on_data_received_(peer_id, data);
        }
    }

    void SimulateConnectionStateChange(const std::string& peer_id, P2PConnectionState state) {
        peer_states_[peer_id] = state;
        if (on_state_changed_) {
            on_state_changed_(peer_id, state);
        }
    }

    void SimulateError(const std::string& peer_id, const std::string& error) {
        if (on_error_) {
            on_error_(peer_id, error);
        }
    }

    void SimulateLatency(const std::string& peer_id, std::chrono::milliseconds latency) {
        peer_latencies_[peer_id] = latency;
    }

    // Track sent data for verification
    struct SentData {
        std::string peer_id;
        std::vector<uint8_t> data;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::vector<SentData> GetSentData() const { return sent_data_; }
    void ClearSentData() { sent_data_.clear(); }

private:
    DataCallback on_data_received_;
    StateCallback on_state_changed_;
    ErrorCallback on_error_;
    
    std::map<std::string, P2PConnectionState> peer_states_;
    std::map<std::string, std::chrono::milliseconds> peer_latencies_;
    std::vector<SentData> sent_data_;
};

/**
 * Test fixture for P2P network tests
 */
class P2PNetworkTestFixture {
public:
    P2PNetworkTestFixture() : mock_network_(std::make_unique<MockP2PNetwork>()) {
        SetupDefaultBehavior();
    }

    void SetupDefaultBehavior() {
        using ::testing::_;
        using ::testing::Invoke;
        using ::testing::Return;

        // Default initialization success
        ON_CALL(*mock_network_, Initialize(_))
            .WillByDefault(Return(true));

        // Track sent data by default
        ON_CALL(*mock_network_, SendData(_, _))
            .WillByDefault(Invoke([this](const std::string& peer_id, const std::vector<uint8_t>& data) {
                sent_data_.push_back({peer_id, data, std::chrono::steady_clock::now()});
                return true;
            }));

        // Default connection state tracking
        ON_CALL(*mock_network_, GetConnectionState(_))
            .WillByDefault(Invoke([this](const std::string& peer_id) {
                auto it = connection_states_.find(peer_id);
                return it != connection_states_.end() ? it->second : P2PConnectionState::Disconnected;
            }));

        // Default connected peers
        ON_CALL(*mock_network_, GetConnectedPeers())
            .WillByDefault(Invoke([this]() {
                std::vector<std::string> peers;
                for (const auto& [peer_id, state] : connection_states_) {
                    if (state == P2PConnectionState::Connected) {
                        peers.push_back(peer_id);
                    }
                }
                return peers;
            }));

        // Default latency
        ON_CALL(*mock_network_, GetLatency(_))
            .WillByDefault(Return(std::chrono::milliseconds(10)));
    }

    // Helper to simulate successful P2P connection
    void SimulateSuccessfulConnection(const std::string& peer_id) {
        connection_states_[peer_id] = P2PConnectionState::Connecting;
        mock_network_->SimulateConnectionStateChange(peer_id, P2PConnectionState::Connecting);
        
        // Simulate connection delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        connection_states_[peer_id] = P2PConnectionState::Connected;
        mock_network_->SimulateConnectionStateChange(peer_id, P2PConnectionState::Connected);
    }

    // Helper to simulate NAT traversal failure and relay fallback
    void SimulateRelayFallback(const std::string& peer_id) {
        connection_states_[peer_id] = P2PConnectionState::Connecting;
        mock_network_->SimulateConnectionStateChange(peer_id, P2PConnectionState::Connecting);
        
        // Simulate NAT traversal failure
        mock_network_->SimulateError(peer_id, "NAT traversal failed");
        
        // Transition to relay mode
        connection_states_[peer_id] = P2PConnectionState::Relaying;
        mock_network_->SimulateConnectionStateChange(peer_id, P2PConnectionState::Relaying);
    }

    // Helper to verify packet was sent
    bool VerifyDataSent(const std::string& peer_id, const std::vector<uint8_t>& expected_data) {
        return std::any_of(sent_data_.begin(), sent_data_.end(),
            [&](const MockP2PNetwork::SentData& sent) {
                return sent.peer_id == peer_id && sent.data == expected_data;
            });
    }

    // Helper to get last sent data to a peer
    std::optional<std::vector<uint8_t>> GetLastSentData(const std::string& peer_id) {
        for (auto it = sent_data_.rbegin(); it != sent_data_.rend(); ++it) {
            if (it->peer_id == peer_id) {
                return it->data;
            }
        }
        return std::nullopt;
    }

protected:
    std::unique_ptr<MockP2PNetwork> mock_network_;
    std::map<std::string, P2PConnectionState> connection_states_;
    std::vector<MockP2PNetwork::SentData> sent_data_;
};

} // namespace Sudachi::Multiplayer::Tests
