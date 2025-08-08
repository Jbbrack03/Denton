// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "libp2p_p2p_network.h"
#include "common/error_codes.h"
#include <stdexcept>
#include <sstream>

// cpp-libp2p includes
#include <libp2p/host/basic_host.hpp>
#include <libp2p/transport/tcp.hpp>
#include <libp2p/transport/ws.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/muxer/mplex.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/multi/multiaddress.hpp>

using namespace libp2p;
using namespace libp2p::transport;
using namespace libp2p::security;
using namespace libp2p::muxer;
using namespace libp2p::protocol;

namespace Core::Multiplayer::ModelA {

Libp2pP2PNetwork::Libp2pP2PNetwork(const P2PNetworkConfig& config)
    : config_(config), started_(false), detected_nat_type_(NATType::Unknown) {
    InitializeHost();
}

Libp2pP2PNetwork::Libp2pP2PNetwork(
    const P2PNetworkConfig& config,
    std::shared_ptr<libp2p::Host> host,
    std::shared_ptr<transport::TransportManager> transport_manager,
    std::shared_ptr<security::SecurityManager> security_manager)
    : config_(config), host_(host), transport_manager_(transport_manager), 
      security_manager_(security_manager), started_(false), detected_nat_type_(NATType::Unknown) {
    
    if (!host_ || !transport_manager_ || !security_manager_) {
        InitializeHost();
    }
}

Libp2pP2PNetwork::~Libp2pP2PNetwork() {
    if (started_) {
        Shutdown();
    }
}

void Libp2pP2PNetwork::InitializeHost() {
    try {
        // Create basic host with default configuration
        host_ = std::make_shared<host::BasicHost>();
        
        ConfigureTransports();
        ConfigureSecurity();
        SetupProtocolHandlers();
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize libp2p host: " + std::string(e.what()));
    }
}

void Libp2pP2PNetwork::ConfigureTransports() {
    if (!transport_manager_) {
        transport_manager_ = std::make_shared<transport::TransportManager>();
    }
    
    // Add TCP transport
    if (config_.enable_tcp) {
        auto tcp_transport = std::make_shared<transport::TcpTransport>();
        transport_manager_->add(tcp_transport);
    }
    
    // Add WebSocket transport
    if (config_.enable_websocket) {
        auto ws_transport = std::make_shared<transport::WsTransport>();
        transport_manager_->add(ws_transport);
    }
}

void Libp2pP2PNetwork::ConfigureSecurity() {
    if (!security_manager_) {
        security_manager_ = std::make_shared<security::SecurityManager>();
    }
    
    // Add Noise security protocol
    auto noise_security = std::make_shared<security::Noise>();
    security_manager_->add(noise_security);
}

void Libp2pP2PNetwork::SetupProtocolHandlers() {
    // Register default Sudachi LDN protocol
    RegisterProtocolHandler("/sudachi/ldn/1.0.0");
    
    // Setup AutoNAT for NAT detection
    if (config_.enable_nat_traversal) {
        autonat_service_ = std::make_shared<autonat::AutoNAT>(host_);
    }
    
    // Setup Circuit Relay
    if (config_.enable_relay) {
        circuit_relay_ = std::make_shared<relay::Relay>(host_);
    }
}

MultiplayerResult Libp2pP2PNetwork::Start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (started_) {
        return {ErrorCode::AlreadyConnected, "P2P network already started"};
    }
    
    try {
        // Start the host
        host_->start();
        
        // Start AutoNAT service
        if (autonat_service_) {
            DetectNATType(); // Trigger NAT detection
        }
        
        // Start Circuit Relay
        if (circuit_relay_) {
            circuit_relay_->start();
        }
        
        started_ = true;
        return {ErrorCode::Success, "P2P network started successfully"};
        
    } catch (const std::exception& e) {
        return {ErrorCode::InternalError, "Failed to start P2P network: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::Stop() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!started_) {
        return {ErrorCode::NotConnected, "P2P network not started"};
    }
    
    try {
        // Stop services
        if (circuit_relay_) {
            circuit_relay_->stop();
        }
        
        if (autonat_service_) {
            autonat_service_->stop();
        }
        
        // Close all connections
        connected_peers_.clear();
        relay_connected_peers_.clear();
        peer_streams_.clear();
        
        started_ = false;
        return {ErrorCode::Success, "P2P network stopped successfully"};
        
    } catch (const std::exception& e) {
        return {ErrorCode::InternalError, "Failed to stop P2P network: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::Shutdown() {
    return Stop();
}

bool Libp2pP2PNetwork::IsStarted() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return started_;
}

std::string Libp2pP2PNetwork::GetPeerId() const {
    if (host_) {
        return PeerIdToString(host_->getId());
    }
    return "";
}

MultiplayerResult Libp2pP2PNetwork::ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!started_) {
        return {ErrorCode::NotInitialized, "P2P network not started"};
    }
    
    try {
        auto peer_id_obj = StringToPeerId(peer_id);
        auto addr = multi::Multiaddress::create(multiaddr);
        
        if (!addr) {
            return {ErrorCode::InvalidParameter, "Invalid multiaddress: " + multiaddr};
        }
        
        // Try direct connection first
        auto result = AttemptDirectConnection(peer_id_obj, addr.value());
        if (result.IsSuccess()) {
            return result;
        }
        
        // Fall back to relay connection if direct fails
        if (config_.enable_relay) {
            return AttemptRelayConnection(peer_id_obj);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        return {ErrorCode::ConnectionFailed, "Failed to connect to peer: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::AttemptDirectConnection(const peer::PeerId& peer_id, const multi::Multiaddress& addr) {
    try {
        // Connect using libp2p host
        auto connection_result = host_->connect(peer_id, addr);
        
        if (connection_result) {
            auto stream = connection_result.value();
            peer_streams_[PeerIdToString(peer_id)] = stream;
            connected_peers_.insert(PeerIdToString(peer_id));
            
            // Setup stream handling
            OnConnectionEstablished(peer_id, stream);
            
            if (on_peer_connected_) {
                on_peer_connected_(PeerIdToString(peer_id));
            }
            
            return {ErrorCode::Success, "Connected to peer directly"};
        } else {
            return {ErrorCode::ConnectionFailed, "Direct connection failed"};
        }
        
    } catch (const std::exception& e) {
        return {ErrorCode::ConnectionFailed, "Direct connection error: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::AttemptRelayConnection(const peer::PeerId& peer_id) {
    try {
        if (!circuit_relay_) {
            return {ErrorCode::NotSupported, "Circuit relay not enabled"};
        }
        
        // Use circuit relay to connect
        auto relay_result = circuit_relay_->connect(peer_id);
        
        if (relay_result) {
            auto stream = relay_result.value();
            peer_streams_[PeerIdToString(peer_id)] = stream;
            connected_peers_.insert(PeerIdToString(peer_id));
            relay_connected_peers_.insert(PeerIdToString(peer_id));
            
            OnConnectionEstablished(peer_id, stream);
            
            if (on_peer_connected_) {
                on_peer_connected_(PeerIdToString(peer_id));
            }
            
            if (on_relay_connected_) {
                on_relay_connected_(PeerIdToString(peer_id), "relay-connection");
            }
            
            return {ErrorCode::Success, "Connected to peer via relay"};
        } else {
            if (on_relay_failed_) {
                on_relay_failed_(PeerIdToString(peer_id), "relay-connection-failed");
            }
            return {ErrorCode::ConnectionFailed, "Relay connection failed"};
        }
        
    } catch (const std::exception& e) {
        if (on_relay_failed_) {
            on_relay_failed_(PeerIdToString(peer_id), e.what());
        }
        return {ErrorCode::ConnectionFailed, "Relay connection error: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::DisconnectFromPeer(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    try {
        auto it = peer_streams_.find(peer_id);
        if (it != peer_streams_.end()) {
            it->second->close();
            peer_streams_.erase(it);
        }
        
        connected_peers_.erase(peer_id);
        relay_connected_peers_.erase(peer_id);
        
        if (on_peer_disconnected_) {
            on_peer_disconnected_(peer_id);
        }
        
        return {ErrorCode::Success, "Disconnected from peer"};
        
    } catch (const std::exception& e) {
        return {ErrorCode::InternalError, "Failed to disconnect: " + std::string(e.what())};
    }
}

bool Libp2pP2PNetwork::IsConnectedToPeer(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return connected_peers_.count(peer_id) > 0;
}

bool Libp2pP2PNetwork::IsConnectedViaRelay(const std::string& peer_id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return relay_connected_peers_.count(peer_id) > 0;
}

size_t Libp2pP2PNetwork::GetConnectionCount() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return connected_peers_.size();
}

std::vector<std::string> Libp2pP2PNetwork::GetConnectedPeers() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return std::vector<std::string>(connected_peers_.begin(), connected_peers_.end());
}

MultiplayerResult Libp2pP2PNetwork::SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = peer_streams_.find(peer_id);
    if (it == peer_streams_.end()) {
        return {ErrorCode::NotConnected, "Not connected to peer: " + peer_id};
    }
    
    try {
        auto& stream = it->second;
        
        // Write protocol header
        std::string header = protocol + "\n";
        stream->write(gsl::span<const uint8_t>(reinterpret_cast<const uint8_t*>(header.data()), header.size()));
        
        // Write data
        stream->write(gsl::span<const uint8_t>(data.data(), data.size()));
        
        return {ErrorCode::Success, "Message sent successfully"};
        
    } catch (const std::exception& e) {
        return {ErrorCode::NetworkError, "Failed to send message: " + std::string(e.what())};
    }
}

MultiplayerResult Libp2pP2PNetwork::BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    size_t success_count = 0;
    size_t total_peers = connected_peers_.size();
    
    for (const auto& peer_id : connected_peers_) {
        auto result = SendMessage(peer_id, protocol, data);
        if (result.IsSuccess()) {
            success_count++;
        }
    }
    
    if (success_count == total_peers) {
        return {ErrorCode::Success, "Broadcast sent to all peers"};
    } else if (success_count > 0) {
        return {ErrorCode::Success, "Broadcast sent to " + std::to_string(success_count) + "/" + std::to_string(total_peers) + " peers"};
    } else {
        return {ErrorCode::NetworkError, "Broadcast failed to all peers"};
    }
}

void Libp2pP2PNetwork::RegisterProtocolHandler(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    protocol_handlers_[protocol] = [this](const std::string& peer_id, const std::vector<uint8_t>& data) {
        if (on_message_received_) {
            on_message_received_(peer_id, "/sudachi/ldn/1.0.0", data);
        }
    };
}

void Libp2pP2PNetwork::HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = protocol_handlers_.find(protocol);
    if (it != protocol_handlers_.end()) {
        it->second(peer_id, data);
    }
}

MultiplayerResult Libp2pP2PNetwork::DetectNATType() {
    if (!autonat_service_) {
        return {ErrorCode::NotSupported, "AutoNAT service not enabled"};
    }
    
    try {
        // Start NAT detection
        autonat_service_->detectNAT([this](const autonat::NATType& nat_type, bool can_traverse) {
            detected_nat_type_ = ConvertLibp2pNATType(nat_type);
            
            if (on_nat_detected_) {
                on_nat_detected_(detected_nat_type_, can_traverse);
            }
        });
        
        return {ErrorCode::Success, "NAT detection started"};
        
    } catch (const std::exception& e) {
        return {ErrorCode::InternalError, "NAT detection failed: " + std::string(e.what())};
    }
}

bool Libp2pP2PNetwork::CanTraverseNAT(NATType local_nat, NATType remote_nat) const {
    // Simple NAT traversal logic - can be enhanced
    if (local_nat == NATType::None || remote_nat == NATType::None) {
        return true; // Direct connection possible
    }
    
    if (local_nat == NATType::Cone && remote_nat == NATType::Cone) {
        return true; // Both cone NATs can traverse
    }
    
    return false; // Symmetric NATs require relay
}

std::vector<std::string> Libp2pP2PNetwork::GetTraversalStrategies(NATType local_nat, NATType remote_nat) const {
    std::vector<std::string> strategies;
    
    if (CanTraverseNAT(local_nat, remote_nat)) {
        strategies.push_back("direct");
        strategies.push_back("hole-punching");
    }
    
    if (config_.enable_relay) {
        strategies.push_back("circuit-relay");
    }
    
    return strategies;
}

std::vector<std::string> Libp2pP2PNetwork::GetConfiguredRelayServers() const {
    return config_.relay_servers;
}

// Callback setters
void Libp2pP2PNetwork::SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_peer_connected_ = callback;
}

void Libp2pP2PNetwork::SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_peer_disconnected_ = callback;
}

void Libp2pP2PNetwork::SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_connection_failed_ = callback;
}

void Libp2pP2PNetwork::SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_message_received_ = callback;
}

void Libp2pP2PNetwork::SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_nat_detected_ = callback;
}

void Libp2pP2PNetwork::SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_relay_connected_ = callback;
}

void Libp2pP2PNetwork::SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_relay_failed_ = callback;
}

// Helper methods
void Libp2pP2PNetwork::OnConnectionEstablished(const peer::PeerId& peer_id, std::shared_ptr<connection::Stream> stream) {
    // Setup stream reading
    stream->read([this, peer_id](outcome::result<size_t> result) {
        if (result) {
            // Handle incoming data
            std::vector<uint8_t> buffer(result.value());
            HandleIncomingMessage(PeerIdToString(peer_id), "/sudachi/ldn/1.0.0", buffer);
        }
    });
}

void Libp2pP2PNetwork::OnConnectionClosed(const peer::PeerId& peer_id) {
    std::string peer_id_str = PeerIdToString(peer_id);
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    connected_peers_.erase(peer_id_str);
    relay_connected_peers_.erase(peer_id_str);
    peer_streams_.erase(peer_id_str);
    
    if (on_peer_disconnected_) {
        on_peer_disconnected_(peer_id_str);
    }
}

NATType Libp2pP2PNetwork::ConvertLibp2pNATType(const autonat::NATType& nat_type) const {
    // Convert libp2p NAT type to our enum
    switch (nat_type) {
        case autonat::NATType::NONE:
            return NATType::None;
        case autonat::NATType::CONE:
            return NATType::Cone;
        case autonat::NATType::SYMMETRIC:
            return NATType::Symmetric;
        default:
            return NATType::Unknown;
    }
}

autonat::NATType Libp2pP2PNetwork::ConvertToLibp2pNATType(NATType nat_type) const {
    switch (nat_type) {
        case NATType::None:
            return autonat::NATType::NONE;
        case NATType::Cone:
            return autonat::NATType::CONE;
        case NATType::Symmetric:
            return autonat::NATType::SYMMETRIC;
        default:
            return autonat::NATType::UNKNOWN;
    }
}

std::string Libp2pP2PNetwork::PeerIdToString(const peer::PeerId& peer_id) const {
    return peer_id.toBase58();
}

peer::PeerId Libp2pP2PNetwork::StringToPeerId(const std::string& peer_id_str) const {
    return peer::PeerId::fromBase58(peer_id_str).value();
}

} // namespace Core::Multiplayer::ModelA
