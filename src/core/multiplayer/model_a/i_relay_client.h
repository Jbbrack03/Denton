// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include "relay_types.h"

namespace Core::Multiplayer::ModelA {

/**
 * Interface for relay client - enables dependency injection in tests
 */
class IRelayClient {
public:
    virtual ~IRelayClient() = default;

    // Connection management
    virtual void ConnectAsync(const std::string& jwt_token, std::function<void(bool)> callback) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Session management
    virtual void CreateSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) = 0;
    virtual void JoinSessionAsync(uint32_t session_token, std::function<void(bool, uint32_t)> callback) = 0;
    virtual uint32_t GetCurrentSession() const = 0;

    // Data transmission
    virtual bool SendData(const std::vector<uint8_t>& data) = 0;
    virtual void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback) = 0;

    // P2P fallback
    virtual void ConnectToPeerAsync(const std::string& peer_id, 
                                   std::function<void(bool, const std::string&)> callback) = 0;
    virtual std::string GetConnectionType() const = 0;

    // Performance and monitoring
    virtual std::chrono::milliseconds GetLatency() const = 0;
    virtual uint64_t GetBandwidthLimit() const = 0;
    virtual void SetErrorCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void TriggerRecovery() = 0;
    virtual ConnectionMetrics GetConnectionMetrics(const std::string& peer_id) const = 0;
};

} // namespace Core::Multiplayer::ModelA
