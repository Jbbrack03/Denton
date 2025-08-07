// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "p2p_types.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <future>

namespace Core::Multiplayer::ModelA {

/**
 * Interface for P2P network implementation
 * Provides dependency injection for testing
 */
class IP2PNetwork {
public:
    virtual ~IP2PNetwork() = default;

    enum class NATType {
        Unknown,
        FullCone,
        RestrictedCone,
        PortRestrictedCone,
        Symmetric,
        NoNAT
    };

    // Network lifecycle
    virtual std::future<MultiplayerResult> Start() = 0;
    virtual MultiplayerResult Stop() = 0;
    virtual MultiplayerResult Shutdown() = 0;
    virtual bool IsStarted() const = 0;

    // Identity and addressing
    virtual std::string GetPeerId() const = 0;

    // Connection management
    virtual std::future<MultiplayerResult> ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) = 0;
    virtual MultiplayerResult DisconnectFromPeer(const std::string& peer_id) = 0;
    virtual bool IsConnectedToPeer(const std::string& peer_id) const = 0;
    virtual bool IsConnectedViaaRelay(const std::string& peer_id) const = 0;
    virtual size_t GetConnectionCount() const = 0;
    virtual std::vector<std::string> GetConnectedPeers() const = 0;

    // Message handling
    virtual MultiplayerResult SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) = 0;
    virtual MultiplayerResult BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data) = 0;
    virtual void RegisterProtocolHandler(const std::string& protocol) = 0;
    virtual void HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) = 0;

    // NAT traversal
    virtual std::future<MultiplayerResult> DetectNATType() = 0;
    virtual bool CanTraverseNAT(NATType local_nat, NATType remote_nat) const = 0;
    virtual std::vector<std::string> GetTraversalStrategies(NATType local_nat, NATType remote_nat) const = 0;

    // Relay management
    virtual std::vector<std::string> GetConfiguredRelayServers() const = 0;

    // Callbacks
    virtual void SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback) = 0;
    virtual void SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback) = 0;
    virtual void SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback) = 0;
    virtual void SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback) = 0;
    virtual void SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback) = 0;
};

} // namespace Core::Multiplayer::ModelA