// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Core::Multiplayer {

/**
 * Result codes for multiplayer operations
 */
enum class MultiplayerResult {
    Success = 0,
    NetworkError,
    NotConnected,
    ConnectionLimitReached,
    Timeout,
    InvalidParameter,
    InternalError
};

} // namespace Core::Multiplayer

namespace Core::Multiplayer::ModelA {

/**
 * P2P Network Configuration
 */
struct P2PNetworkConfig {
    // Transport configuration
    bool enable_tcp = true;
    bool enable_quic = false;
    bool enable_websocket = false;
    uint16_t tcp_port = 4001;
    uint16_t quic_port = 4001;
    uint16_t websocket_port = 4001;
    
    // Connection limits
    size_t max_connections = 100;
    uint32_t connection_timeout_ms = 5000;
    
    // NAT traversal
    bool enable_autonat = true;
    bool enable_relay = true;
    
    // Relay servers
    std::vector<std::string> relay_servers;
};

} // namespace Core::Multiplayer::ModelA
