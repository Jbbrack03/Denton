// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "i_p2p_network.h"
#include "p2p_types.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// Forward declarations for mock interfaces
namespace Core::Multiplayer::ModelA::Test {
class MockLibp2pHost;
class MockTransportManager;
class MockSecurityManager;
class MockAutoNATService;
class MockCircuitRelay;
class MockPerformanceMonitor;
}

namespace Core::Multiplayer::ModelA {

/**
 * P2P Network implementation using cpp-libp2p
 * Handles peer-to-peer connections, NAT traversal, and relay fallback
 */
class P2PNetwork : public IP2PNetwork {
public:
    // Constructor for dependency injection (primarily for testing)
    P2PNetwork(
        const P2PNetworkConfig& config,
        std::shared_ptr<Test::MockLibp2pHost> host,
        std::shared_ptr<Test::MockTransportManager> transport_manager,
        std::shared_ptr<Test::MockSecurityManager> security_manager,
        std::shared_ptr<Test::MockAutoNATService> autonat_service,
        std::shared_ptr<Test::MockCircuitRelay> circuit_relay,
        std::shared_ptr<Test::MockPerformanceMonitor> performance_monitor
    );

    // Regular constructor (for production use, not implemented yet)
    explicit P2PNetwork(const P2PNetworkConfig& config);

    ~P2PNetwork() override = default;

    // Network lifecycle
    MultiplayerResult Start() override;
    MultiplayerResult Stop() override;
    MultiplayerResult Shutdown() override;
    bool IsStarted() const override;

    // Identity and addressing
    std::string GetPeerId() const override;

    // Connection management
    MultiplayerResult ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) override;
    MultiplayerResult DisconnectFromPeer(const std::string& peer_id) override;
    bool IsConnectedToPeer(const std::string& peer_id) const override;
    bool IsConnectedViaaRelay(const std::string& peer_id) const override;
    size_t GetConnectionCount() const override;
    std::vector<std::string> GetConnectedPeers() const override;

    // Message handling
    MultiplayerResult SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) override;
    MultiplayerResult BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data) override;
    void RegisterProtocolHandler(const std::string& protocol) override;
    void HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) override;

    // NAT traversal
    MultiplayerResult DetectNATType() override;
    bool CanTraverseNAT(NATType local_nat, NATType remote_nat) const override;
    std::vector<std::string> GetTraversalStrategies(NATType local_nat, NATType remote_nat) const override;

    // Relay management
    std::vector<std::string> GetConfiguredRelayServers() const override;

    // Callbacks
    void SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback) override;
    void SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback) override;
    void SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback) override;
    void SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback) override;
    void SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback) override;
    void SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback) override;
    void SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback) override;

private:
    // Configuration
    P2PNetworkConfig config_;

    // Mock dependencies (for testing)
    std::shared_ptr<Test::MockLibp2pHost> mock_host_;
    std::shared_ptr<Test::MockTransportManager> mock_transport_manager_;
    std::shared_ptr<Test::MockSecurityManager> mock_security_manager_;
    std::shared_ptr<Test::MockAutoNATService> mock_autonat_service_;
    std::shared_ptr<Test::MockCircuitRelay> mock_circuit_relay_;
    std::shared_ptr<Test::MockPerformanceMonitor> mock_performance_monitor_;

    // State tracking
    mutable std::mutex state_mutex_;
    std::unordered_set<std::string> connected_peers_;
    std::unordered_set<std::string> relay_connected_peers_;
    std::unordered_map<std::string, std::function<void(const std::string&, const std::vector<uint8_t>&)>> protocol_handlers_;

    // Callbacks
    std::function<void(const std::string&)> on_peer_connected_;
    std::function<void(const std::string&)> on_peer_disconnected_;
    std::function<void(const std::string&, const std::string&)> on_connection_failed_;
    std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> on_message_received_;
    std::function<void(NATType, bool)> on_nat_detected_;
    std::function<void(const std::string&, const std::string&)> on_relay_connected_;
    std::function<void(const std::string&, const std::string&)> on_relay_failed_;

    // Helper methods
    void ValidateConfiguration();
    NATType ConvertMockNATType(int mock_type) const;
    int ConvertToMockNATType(NATType nat_type) const;
    MultiplayerResult AttemptDirectConnection(const std::string& peer_id, const std::string& multiaddr);
    MultiplayerResult AttemptRelayConnection(const std::string& peer_id);
};

} // namespace Core::Multiplayer::ModelA