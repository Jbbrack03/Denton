// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace Core::Multiplayer::ModelA {

/**
 * Connection types for relay clients
 */
enum class ConnectionType {
    Direct,
    P2P,
    Relay
};

/**
 * Connection metrics for performance monitoring
 */
struct ConnectionMetrics {
    uint64_t connection_time_ms;
    ConnectionType connection_type;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    std::chrono::milliseconds latency;
};

/**
 * Relay protocol flag constants
 */
namespace RelayProtocolFlags {
    constexpr uint8_t FLAG_DATA = 0x00;
    constexpr uint8_t FLAG_CONTROL = 0x80;
    constexpr uint8_t FLAG_KEEPALIVE = 0x40;
    constexpr uint8_t FLAG_SESSION_CREATE = 0x20;
    constexpr uint8_t FLAG_SESSION_JOIN = 0x10;
    constexpr uint8_t FLAG_SESSION_LEAVE = 0x08;
}

} // namespace Core::Multiplayer::ModelA
