// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Minimal P2P Network Implementation for TDD Green Phase
 * This is the simplest possible implementation that makes tests pass
 */

#include "p2p_network.h"
#include <stdexcept>

namespace Core::Multiplayer::ModelA {

// Minimal stub constructor for production (not implemented)
P2PNetwork::P2PNetwork(const P2PNetworkConfig& config) : config_(config) {
    throw std::runtime_error("Production P2PNetwork constructor not implemented yet");
}

// Helper method implementations (minimal stubs)
void P2PNetwork::ValidateConfiguration() {
    if (!config_.enable_tcp && !config_.enable_quic && !config_.enable_websocket) {
        throw std::invalid_argument("At least one transport must be enabled");
    }
}

P2PNetwork::NATType P2PNetwork::ConvertMockNATType(int mock_type) const {
    switch (mock_type) {
        case 1: return NATType::FullCone;
        case 2: return NATType::RestrictedCone;
        default: return NATType::Unknown;
    }
}

int P2PNetwork::ConvertToMockNATType(NATType nat_type) const {
    switch (nat_type) {
        case NATType::FullCone: return 1;
        case NATType::RestrictedCone: return 2;
        default: return 0;
    }
}

// Stub implementations that would be properly implemented with real libp2p
MultiplayerResult P2PNetwork::Start() { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::Stop() { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::Shutdown() { throw std::runtime_error("Not implemented in production mode"); }
bool P2PNetwork::IsStarted() const { throw std::runtime_error("Not implemented in production mode"); }
std::string P2PNetwork::GetPeerId() const { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::ConnectToPeer(const std::string&, const std::string&) { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::DisconnectFromPeer(const std::string&) { throw std::runtime_error("Not implemented in production mode"); }
bool P2PNetwork::IsConnectedToPeer(const std::string&) const { throw std::runtime_error("Not implemented in production mode"); }
bool P2PNetwork::IsConnectedViaRelay(const std::string&) const { throw std::runtime_error("Not implemented in production mode"); }
size_t P2PNetwork::GetConnectionCount() const { throw std::runtime_error("Not implemented in production mode"); }
std::vector<std::string> P2PNetwork::GetConnectedPeers() const { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::SendMessage(const std::string&, const std::string&, const std::vector<uint8_t>&) { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::BroadcastMessage(const std::string&, const std::vector<uint8_t>&) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::RegisterProtocolHandler(const std::string&) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::HandleIncomingMessage(const std::string&, const std::string&, const std::vector<uint8_t>&) { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::DetectNATType() { throw std::runtime_error("Not implemented in production mode"); }
bool P2PNetwork::CanTraverseNAT(NATType, NATType) const { throw std::runtime_error("Not implemented in production mode"); }
std::vector<std::string> P2PNetwork::GetTraversalStrategies(NATType, NATType) const { throw std::runtime_error("Not implemented in production mode"); }
std::vector<std::string> P2PNetwork::GetConfiguredRelayServers() const { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnPeerConnectedCallback(std::function<void(const std::string&)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnPeerDisconnectedCallback(std::function<void(const std::string&)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnNATDetectedCallback(std::function<void(NATType, bool)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)>) { throw std::runtime_error("Not implemented in production mode"); }
void P2PNetwork::SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)>) { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::AttemptDirectConnection(const std::string&, const std::string&) { throw std::runtime_error("Not implemented in production mode"); }
MultiplayerResult P2PNetwork::AttemptRelayConnection(const std::string&) { throw std::runtime_error("Not implemented in production mode"); }

} // namespace Core::Multiplayer::ModelA
