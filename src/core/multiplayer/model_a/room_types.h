// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <map>

namespace Core::Multiplayer::ModelA {

/**
 * Connection states for RoomClient
 */
enum class ConnectionState {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Reconnecting = 3,
    Failed = 4
};

/**
 * Network information for a player
 */
struct NetworkInfo {
    std::string local_ip;
    std::string public_ip;
    uint16_t port = 0;
};

/**
 * Client information structure
 */
struct ClientInfo {
    std::string username;
    std::string platform;
    NetworkInfo network_info;
};

/**
 * Player information in a room
 */
struct PlayerInfo {
    std::string id;
    std::string username;
    bool is_host = false;
    NetworkInfo network_info;
};

/**
 * Room summary information
 */
struct RoomSummary {
    std::string id;
    std::string game_name;
    std::string host_name;
    int current_players = 0;
    int max_players = 0;
    int ping = 0;
    std::string region;
};

/**
 * Complete room information
 */
struct RoomInfo {
    std::string id;
    uint64_t game_id = 0;
    std::string game_name;
    std::string host_id;
    std::string host_name;
    int max_players = 0;
    int current_players = 0;
    bool is_private = false;
    uint64_t created_at = 0;
    std::string description;
};

/**
 * Connection information structure
 */
struct ConnectionInfo {
    std::string server_url;
    ConnectionState state = ConnectionState::Disconnected;
    std::string client_id;
    uint64_t created_at = 0;
};

/**
 * ICE candidate for WebRTC P2P connections
 */
struct IceCandidate {
    std::string candidate;
    std::string sdp_mid;
    int sdp_mline_index = 0;
};

/**
 * Session description for WebRTC
 */
struct SessionDescription {
    std::string type;
    std::string sdp;
};

/**
 * Reconnection statistics
 */
struct ReconnectionStatistics {
    int total_attempts = 0;
    int successful_reconnections = 0;
    int failed_reconnections = 0;
    uint64_t total_downtime_ms = 0;
    uint64_t average_reconnect_time_ms = 0;
};

} // namespace Core::Multiplayer::ModelA