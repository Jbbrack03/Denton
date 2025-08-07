// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "i_p2p_network.h"
#include "p2p_types.h"
#include <future>
#include <memory>
#include <mutex>

namespace Core::Multiplayer::ModelA {

class Libp2pP2PNetwork; // forward declaration
class AsyncExecutor;    // forward declaration for shared executor

/**
 * High level P2P network facade.
 * Wraps the Libp2pP2PNetwork implementation and exposes
 * asynchronous operations.
 */
class P2PNetwork : public IP2PNetwork {
public:
    explicit P2PNetwork(const P2PNetworkConfig& config);
    ~P2PNetwork() override;

    // Network lifecycle
    std::future<MultiplayerResult> Start() override;
    MultiplayerResult Stop() override;
    MultiplayerResult Shutdown() override;
    bool IsStarted() const override;

    // Identity and addressing
    std::string GetPeerId() const override;

    // Connection management
    std::future<MultiplayerResult> ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) override;
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
    std::future<MultiplayerResult> DetectNATType() override;
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
    std::unique_ptr<Libp2pP2PNetwork> impl_;
    mutable std::mutex callback_mutex_;

    /**
     * @brief Shared executor providing a reusable worker thread pool.
     *
     * The executor is thread-safe and used by asynchronous APIs such as
     * Start(), ConnectToPeer() and DetectNATType(). Tasks submitted to the
     * executor run on a fixed pool of worker threads rather than spawning new
     * threads on each request. The executor's lifetime is bound to the
     * P2PNetwork instance and all worker threads are joined on destruction.
     */
    std::shared_ptr<AsyncExecutor> executor_;
};

} // namespace Core::Multiplayer::ModelA
