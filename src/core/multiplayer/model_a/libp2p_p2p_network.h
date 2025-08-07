// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "p2p_types.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// cpp-libp2p includes
#include <libp2p/host/basic_host.hpp>
#include <libp2p/transport/transport_manager.hpp>
#include <libp2p/security/security_manager.hpp>
#include <libp2p/protocol/autonat/autonat.hpp>
#include <libp2p/protocol/relay/relay.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/connection/stream.hpp>

namespace Core::Multiplayer::ModelA {

/**
 * P2P Network implementation using real cpp-libp2p
 * Replaces mock implementation with actual libp2p functionality
 */
class Libp2pP2PNetwork {
public:
    enum class NATType {
        Unknown,
        FullCone,
        RestrictedCone,
        PortRestrictedCone,
        Symmetric,
        NoNAT
    };
    // Constructor for production use
    explicit Libp2pP2PNetwork(const P2PNetworkConfig& config);
    
    // Constructor for dependency injection (testing)
    Libp2pP2PNetwork(
        const P2PNetworkConfig& config,
        std::shared_ptr<libp2p::Host> host,
        std::shared_ptr<libp2p::transport::TransportManager> transport_manager,
        std::shared_ptr<libp2p::security::SecurityManager> security_manager
    );

    ~Libp2pP2PNetwork();

    // Network lifecycle
    MultiplayerResult Start();
    MultiplayerResult Stop();
    MultiplayerResult Shutdown();
    bool IsStarted() const;

    // Identity and addressing
    std::string GetPeerId() const;

    // Connection management
    MultiplayerResult ConnectToPeer(const std::string& peer_id, const std::string& multiaddr);
    MultiplayerResult DisconnectFromPeer(const std::string& peer_id);
    bool IsConnectedToPeer(const std::string& peer_id) const;
    bool IsConnectedViaRelay(const std::string& peer_id) const;
    [[deprecated("Use IsConnectedViaRelay")]]
    bool IsConnectedViaaRelay(const std::string& peer_id) const {
        return IsConnectedViaRelay(peer_id);
    }
    size_t GetConnectionCount() const;
    std::vector<std::string> GetConnectedPeers() const;

    // Message handling
    MultiplayerResult SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data);
    MultiplayerResult BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data);
    void RegisterProtocolHandler(const std::string& protocol);
    void HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data);

    // NAT traversal
    MultiplayerResult DetectNATType();
    bool CanTraverseNAT(NATType local_nat, NATType remote_nat) const;
    std::vector<std::string> GetTraversalStrategies(NATType local_nat, NATType remote_nat) const;

    // Relay management
    std::vector<std::string> GetConfiguredRelayServers() const;

    // Callbacks
    void SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback);
    void SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback);
    void SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback);
    void SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback);
    void SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback);
    void SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback);
    void SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback);

private:
    // Configuration
    P2PNetworkConfig config_;

    // libp2p components
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::transport::TransportManager> transport_manager_;
    std::shared_ptr<libp2p::security::SecurityManager> security_manager_;
    std::shared_ptr<libp2p::protocol::autonat::AutoNAT> autonat_service_;
    std::shared_ptr<libp2p::protocol::relay::Relay> circuit_relay_;

    // State tracking
    mutable std::mutex state_mutex_;
    bool started_;
    std::unordered_set<std::string> connected_peers_;
    std::unordered_set<std::string> relay_connected_peers_;
    std::unordered_map<std::string, std::shared_ptr<libp2p::connection::Stream>> peer_streams_;
    NATType detected_nat_type_;

    // Protocol handlers
    std::unordered_map<std::string, std::function<void(const std::string&, const std::vector<uint8_t>&)>> protocol_handlers_;

    // Callbacks
    std::function<void(const std::string&)> on_peer_connected_;
    std::function<void(const std::string&)> on_peer_disconnected_;
    std::function<void(const std::string&, const std::string&)> on_connection_failed_;
    std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> on_message_received_;
    std::function<void(NATType, bool)> on_nat_detected_;
    std::function<void(const std::string&, const std::string&)> on_relay_connected_;
    std::function<void(const std::string&, const std::string&)> on_relay_failed_;
    mutable std::mutex callback_mutex_;

    // Helper methods
    void InitializeHost();
    void ConfigureTransports();
    void ConfigureSecurity();
    void SetupProtocolHandlers();
    void OnConnectionEstablished(const libp2p::peer::PeerId& peer_id, std::shared_ptr<libp2p::connection::Stream> stream);
    void OnConnectionClosed(const libp2p::peer::PeerId& peer_id);
    void OnStreamReceived(std::shared_ptr<libp2p::connection::Stream> stream);
    MultiplayerResult AttemptDirectConnection(const libp2p::peer::PeerId& peer_id, const libp2p::multi::Multiaddress& addr);
    MultiplayerResult AttemptRelayConnection(const libp2p::peer::PeerId& peer_id);
    NATType ConvertLibp2pNATType(const libp2p::protocol::autonat::NATType& nat_type) const;
    libp2p::protocol::autonat::NATType ConvertToLibp2pNATType(NATType nat_type) const;
    std::string PeerIdToString(const libp2p::peer::PeerId& peer_id) const;
    libp2p::peer::PeerId StringToPeerId(const std::string& peer_id_str) const;
};

} // namespace Core::Multiplayer::ModelA