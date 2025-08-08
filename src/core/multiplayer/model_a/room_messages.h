// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <map>
#include "room_types.h"

namespace Core::Multiplayer::ModelA {

/**
 * Message types for room communication
 */
enum class MessageType {
    Unknown,
    Register,
    RegisterResponse,
    CreateRoom,
    RoomCreated,
    RoomList,
    RoomListResponse,
    JoinRoom,
    JoinRoomResponse,
    LeaveRoom,
    P2PInfo,
    UseProxy,
    Error,
    PlayerJoined,
    PlayerLeft,
    Heartbeat
};

/**
 * Base message structure
 */
struct BaseMessage {
    MessageType type = MessageType::Unknown;
    uint64_t timestamp = 0;
    bool is_valid = false;
};

/**
 * Client registration request
 */
struct RegisterRequest {
    std::string client_id;
    std::string username;
    std::string platform;
    std::string sudachi_version;
};

/**
 * Registration response from server
 */
struct RegisterResponse {
    bool success = false;
    std::string client_id;
    uint64_t server_time = 0;
    std::string server_version;
};

/**
 * Create room request
 */
struct CreateRoomRequest {
    uint64_t game_id = 0;
    std::string game_name;
    int max_players = 2;
    bool is_private = false;
    std::string password;
    std::string description;
};

/**
 * Room created response
 */
struct RoomCreatedResponse {
    bool success = false;
    RoomInfo room;
};

/**
 * Room list request
 */
struct RoomListRequest {
    uint64_t game_id = 0;
    std::string region;
    int max_results = 20;
    int offset = 0;
    bool include_private = false;
    bool include_full = false;
};

/**
 * Room list response
 */
struct RoomListResponse {
    bool success = false;
    int total_count = 0;
    std::vector<RoomSummary> rooms;
};

/**
 * Join room request
 */
struct JoinRoomRequest {
    std::string room_id;
    std::string password;
    ClientInfo client_info;
};

/**
 * Join room response
 */
struct JoinRoomResponse {
    bool success = false;
    std::string room_id;
    std::string player_id;
    std::vector<PlayerInfo> players;
};

/**
 * P2P connection information message
 */
struct P2PInfoMessage {
    std::string from_player;
    std::string to_player;
    std::string connection_type;
    std::vector<IceCandidate> ice_candidates;
    SessionDescription session_description;
};

/**
 * Use proxy server message
 */
struct UseProxyMessage {
    std::string relay_server;
    uint16_t relay_port = 0;
    std::string auth_token;
    std::string session_id;
    std::string target_player;
    uint64_t expires_at = 0;
};

/**
 * Error message
 */
struct ErrorMessage {
    std::string error_code;
    std::string message;
    std::map<std::string, std::string> details;
    uint64_t retry_after = 0;
};

/**
 * Player joined room message
 */
struct PlayerJoinedMessage {
    std::string room_id;
    PlayerInfo player;
};

/**
 * Player left room message
 */
struct PlayerLeftMessage {
    std::string room_id;
    std::string player_id;
    std::string reason;
};

/**
 * Message parsing exceptions
 */
class MessageParsingException : public std::runtime_error {
public:
    explicit MessageParsingException(const std::string& message) : std::runtime_error(message) {}
};

class MessageTooLargeException : public std::runtime_error {
public:
    explicit MessageTooLargeException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * Message serialization utilities
 */
class MessageSerializer {
public:
    static std::string Serialize(const RegisterRequest& request);
    static std::string Serialize(const CreateRoomRequest& request);
    static std::string Serialize(const RoomListRequest& request);
    static std::string Serialize(const JoinRoomRequest& request);
};

/**
 * Message deserialization utilities
 */
class MessageDeserializer {
public:
    template<typename T>
    static T Deserialize(const std::string& json_str);
    
    static BaseMessage DeserializeAny(const std::string& json_str);
};

/**
 * Message validation utilities
 */
class MessageValidator {
public:
    static bool ValidateTimestamp(const std::string& json_str, std::chrono::minutes tolerance);
    static void ValidateSize(const std::string& json_str);
};

} // namespace Core::Multiplayer::ModelA