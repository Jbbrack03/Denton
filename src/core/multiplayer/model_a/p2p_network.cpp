// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "p2p_network.h"
#include "libp2p_p2p_network.h"
#include <future>

namespace Core::Multiplayer::ModelA {

P2PNetwork::P2PNetwork(const P2PNetworkConfig& config)
    : impl_(std::make_unique<Libp2pP2PNetwork>(config)) {}

P2PNetwork::~P2PNetwork() = default;

std::future<MultiplayerResult> P2PNetwork::Start() {
    return std::async(std::launch::async, [this] { return impl_->Start(); });
}

MultiplayerResult P2PNetwork::Stop() { return impl_->Stop(); }

MultiplayerResult P2PNetwork::Shutdown() { return impl_->Shutdown(); }

bool P2PNetwork::IsStarted() const { return impl_->IsStarted(); }

std::string P2PNetwork::GetPeerId() const { return impl_->GetPeerId(); }

std::future<MultiplayerResult> P2PNetwork::ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) {
    return std::async(std::launch::async, [this, peer_id, multiaddr] {
        return impl_->ConnectToPeer(peer_id, multiaddr);
    });
}

MultiplayerResult P2PNetwork::DisconnectFromPeer(const std::string& peer_id) {
    return impl_->DisconnectFromPeer(peer_id);
}

bool P2PNetwork::IsConnectedToPeer(const std::string& peer_id) const {
    return impl_->IsConnectedToPeer(peer_id);
}

bool P2PNetwork::IsConnectedViaaRelay(const std::string& peer_id) const {
    return impl_->IsConnectedViaaRelay(peer_id);
}

size_t P2PNetwork::GetConnectionCount() const { return impl_->GetConnectionCount(); }

std::vector<std::string> P2PNetwork::GetConnectedPeers() const { return impl_->GetConnectedPeers(); }

MultiplayerResult P2PNetwork::SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    return impl_->SendMessage(peer_id, protocol, data);
}

MultiplayerResult P2PNetwork::BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data) {
    return impl_->BroadcastMessage(protocol, data);
}

void P2PNetwork::RegisterProtocolHandler(const std::string& protocol) {
    impl_->RegisterProtocolHandler(protocol);
}

void P2PNetwork::HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    impl_->HandleIncomingMessage(peer_id, protocol, data);
}

std::future<MultiplayerResult> P2PNetwork::DetectNATType() {
    return std::async(std::launch::async, [this] { return impl_->DetectNATType(); });
}

bool P2PNetwork::CanTraverseNAT(NATType local_nat, NATType remote_nat) const {
    return impl_->CanTraverseNAT(local_nat, remote_nat);
}

std::vector<std::string> P2PNetwork::GetTraversalStrategies(NATType local_nat, NATType remote_nat) const {
    return impl_->GetTraversalStrategies(local_nat, remote_nat);
}

std::vector<std::string> P2PNetwork::GetConfiguredRelayServers() const {
    return impl_->GetConfiguredRelayServers();
}

void P2PNetwork::SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnPeerConnectedCallback(callback);
}

void P2PNetwork::SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnPeerDisconnectedCallback(callback);
}

void P2PNetwork::SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnConnectionFailedCallback(callback);
}

void P2PNetwork::SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnMessageReceivedCallback(callback);
}

void P2PNetwork::SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnNATDetectedCallback(callback);
}

void P2PNetwork::SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnRelayConnectedCallback(callback);
}

void P2PNetwork::SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnRelayFailedCallback(callback);
}

} // namespace Core::Multiplayer::ModelA
