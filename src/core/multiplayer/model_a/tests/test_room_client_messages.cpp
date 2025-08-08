// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

#include "core/multiplayer/model_a/room_client.h"
#include "core/multiplayer/model_a/room_messages.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;
using json = nlohmann::json;

namespace {

/**
 * Mock message handler for testing message processing
 */
class MockMessageHandler {
public:
    MOCK_METHOD(void, OnRoomCreated, (const RoomCreatedResponse& response), ());
    MOCK_METHOD(void, OnRoomListUpdate, (const RoomListResponse& response), ());
    MOCK_METHOD(void, OnJoinedRoom, (const JoinRoomResponse& response), ());
    MOCK_METHOD(void, OnP2PInfoReceived, (const P2PInfoMessage& message), ());
    MOCK_METHOD(void, OnUseProxyMessage, (const UseProxyMessage& message), ());
    MOCK_METHOD(void, OnErrorReceived, (const ErrorMessage& error), ());
    MOCK_METHOD(void, OnPlayerJoined, (const PlayerJoinedMessage& message), ());
    MOCK_METHOD(void, OnPlayerLeft, (const PlayerLeftMessage& message), ());
};

/**
 * Mock WebSocket connection for message testing
 */
class MockWebSocketConnection {
public:
    MOCK_METHOD(void, SendMessage, (const std::string& message), ());
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(void, SetOnMessageCallback, (std::function<void(const std::string&)> callback), ());
    
    // Store the message callback for test use
    void SetMessageCallback(std::function<void(const std::string&)> callback) {
        message_callback_ = callback;
    }
    
    // Simulate receiving a message
    void SimulateMessage(const std::string& message) {
        if (message_callback_) {
            message_callback_(message);
        }
    }
    
private:
    std::function<void(const std::string&)> message_callback_;
};

} // anonymous namespace

/**
 * Test fixture for RoomClient message handling tests
 * Tests JSON message parsing, serialization, and handling for all message types
 */
class RoomClientMessagesTest : public Test {
protected:
    void SetUp() override {
        mock_connection_ = std::make_shared<MockWebSocketConnection>();
        mock_handler_ = std::make_shared<MockMessageHandler>();
        
        // Default connection state
        ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(true));
    }

    std::shared_ptr<MockWebSocketConnection> mock_connection_;
    std::shared_ptr<MockMessageHandler> mock_handler_;
};

/**
 * Test: RegisterRequest message serialization
 * Verifies that RegisterRequest messages are properly serialized to JSON
 */
TEST_F(RoomClientMessagesTest, RegisterRequestSerialization) {
    // This test will fail because RegisterRequest doesn't exist yet
    
    // ARRANGE
    RegisterRequest request;
    request.client_id = "test_client_12345";
    request.username = "TestPlayer";
    request.platform = "Windows";
    request.sudachi_version = "1.0.0";
    
    // ACT
    auto json_str = MessageSerializer::Serialize(request);
    auto parsed = json::parse(json_str);
    
    // ASSERT
    EXPECT_EQ(parsed["type"], "register");
    EXPECT_EQ(parsed["client_id"], "test_client_12345");
    EXPECT_EQ(parsed["username"], "TestPlayer");
    EXPECT_EQ(parsed["platform"], "Windows");
    EXPECT_EQ(parsed["sudachi_version"], "1.0.0");
    EXPECT_TRUE(parsed.contains("timestamp"));
}

/**
 * Test: RegisterResponse message deserialization
 * Verifies that RegisterResponse messages are properly parsed from JSON
 */
TEST_F(RoomClientMessagesTest, RegisterResponseDeserialization) {
    // This test will fail because RegisterResponse parsing doesn't exist yet
    
    // ARRANGE
    json response_json = {
        {"type", "register_response"},
        {"success", true},
        {"client_id", "assigned_client_67890"},
        {"server_time", 1641024000000},
        {"server_version", "2.1.0"}
    };
    
    // ACT
    auto response = MessageDeserializer::Deserialize<RegisterResponse>(response_json.dump());
    
    // ASSERT
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.client_id, "assigned_client_67890");
    EXPECT_EQ(response.server_time, 1641024000000);
    EXPECT_EQ(response.server_version, "2.1.0");
}

/**
 * Test: CreateRoomRequest message serialization
 * Verifies that CreateRoomRequest messages include all required fields
 */
TEST_F(RoomClientMessagesTest, CreateRoomRequestSerialization) {
    // This test will fail because CreateRoomRequest doesn't exist yet
    
    // ARRANGE
    CreateRoomRequest request;
    request.game_id = 0x0100123456789ABC;
    request.game_name = "Super Mario Odyssey";
    request.max_players = 4;
    request.is_private = false;
    request.password = "";
    request.description = "Fun multiplayer session!";
    
    // ACT
    auto json_str = MessageSerializer::Serialize(request);
    auto parsed = json::parse(json_str);
    
    // ASSERT
    EXPECT_EQ(parsed["type"], "create_room");
    EXPECT_EQ(parsed["game_id"], "0100123456789ABC"); // Should be hex string
    EXPECT_EQ(parsed["game_name"], "Super Mario Odyssey");
    EXPECT_EQ(parsed["max_players"], 4);
    EXPECT_EQ(parsed["is_private"], false);
    EXPECT_EQ(parsed["password"], "");
    EXPECT_EQ(parsed["description"], "Fun multiplayer session!");
}

/**
 * Test: RoomCreatedResponse message deserialization
 * Verifies that RoomCreatedResponse contains all room information
 */
TEST_F(RoomClientMessagesTest, RoomCreatedResponseDeserialization) {
    // This test will fail because RoomCreatedResponse parsing doesn't exist yet
    
    // ARRANGE
    json response_json = {
        {"type", "room_created"},
        {"success", true},
        {"room", {
            {"id", "room_uuid_12345"},
            {"game_id", "0100123456789ABC"},
            {"game_name", "Super Mario Odyssey"},
            {"host_id", "client_host_123"},
            {"host_name", "HostPlayer"},
            {"max_players", 4},
            {"current_players", 1},
            {"is_private", false},
            {"created_at", 1641024000000},
            {"description", "Fun session"}
        }}
    };
    
    // ACT
    auto response = MessageDeserializer::Deserialize<RoomCreatedResponse>(response_json.dump());
    
    // ASSERT
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.room.id, "room_uuid_12345");
    EXPECT_EQ(response.room.game_id, 0x0100123456789ABC);
    EXPECT_EQ(response.room.game_name, "Super Mario Odyssey");
    EXPECT_EQ(response.room.host_id, "client_host_123");
    EXPECT_EQ(response.room.host_name, "HostPlayer");
    EXPECT_EQ(response.room.max_players, 4);
    EXPECT_EQ(response.room.current_players, 1);
    EXPECT_FALSE(response.room.is_private);
    EXPECT_EQ(response.room.created_at, 1641024000000);
    EXPECT_EQ(response.room.description, "Fun session");
}

/**
 * Test: RoomListRequest message serialization
 * Verifies that room list requests include proper filtering
 */
TEST_F(RoomClientMessagesTest, RoomListRequestSerialization) {
    // This test will fail because RoomListRequest doesn't exist yet
    
    // ARRANGE
    RoomListRequest request;
    request.game_id = 0x0100123456789ABC;
    request.region = "US";
    request.max_results = 20;
    request.offset = 0;
    request.include_private = false;
    request.include_full = false;
    
    // ACT
    auto json_str = MessageSerializer::Serialize(request);
    auto parsed = json::parse(json_str);
    
    // ASSERT
    EXPECT_EQ(parsed["type"], "room_list");
    EXPECT_EQ(parsed["game_id"], "0100123456789ABC");
    EXPECT_EQ(parsed["region"], "US");
    EXPECT_EQ(parsed["max_results"], 20);
    EXPECT_EQ(parsed["offset"], 0);
    EXPECT_EQ(parsed["include_private"], false);
    EXPECT_EQ(parsed["include_full"], false);
}

/**
 * Test: RoomListResponse message deserialization
 * Verifies that room list responses contain array of rooms
 */
TEST_F(RoomClientMessagesTest, RoomListResponseDeserialization) {
    // This test will fail because RoomListResponse parsing doesn't exist yet
    
    // ARRANGE
    json response_json = {
        {"type", "room_list_response"},
        {"success", true},
        {"total_count", 42},
        {"rooms", json::array({
            {
                {"id", "room_1"},
                {"game_id", "0100123456789ABC"},
                {"game_name", "Game 1"},
                {"host_name", "Host1"},
                {"current_players", 2},
                {"max_players", 4},
                {"ping", 45},
                {"region", "US"}
            },
            {
                {"id", "room_2"},
                {"game_id", "0100123456789ABC"},
                {"game_name", "Game 1"},
                {"host_name", "Host2"},
                {"current_players", 1},
                {"max_players", 2},
                {"ping", 78},
                {"region", "US"}
            }
        })}
    };
    
    // ACT
    auto response = MessageDeserializer::Deserialize<RoomListResponse>(response_json.dump());
    
    // ASSERT
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.total_count, 42);
    EXPECT_EQ(response.rooms.size(), 2);
    
    EXPECT_EQ(response.rooms[0].id, "room_1");
    EXPECT_EQ(response.rooms[0].current_players, 2);
    EXPECT_EQ(response.rooms[0].ping, 45);
    
    EXPECT_EQ(response.rooms[1].id, "room_2");
    EXPECT_EQ(response.rooms[1].current_players, 1);
    EXPECT_EQ(response.rooms[1].ping, 78);
}

/**
 * Test: JoinRoomRequest message serialization
 * Verifies that join room requests include authentication
 */
TEST_F(RoomClientMessagesTest, JoinRoomRequestSerialization) {
    // This test will fail because JoinRoomRequest doesn't exist yet
    
    // ARRANGE
    JoinRoomRequest request;
    request.room_id = "room_uuid_to_join";
    request.password = "secret123";
    request.client_info.username = "JoiningPlayer";
    request.client_info.platform = "Android";
    request.client_info.network_info.local_ip = "192.168.1.100";
    request.client_info.network_info.public_ip = "203.0.113.1";
    request.client_info.network_info.port = 7755;
    
    // ACT
    auto json_str = MessageSerializer::Serialize(request);
    auto parsed = json::parse(json_str);
    
    // ASSERT
    EXPECT_EQ(parsed["type"], "join_room");
    EXPECT_EQ(parsed["room_id"], "room_uuid_to_join");
    EXPECT_EQ(parsed["password"], "secret123");
    EXPECT_EQ(parsed["client_info"]["username"], "JoiningPlayer");
    EXPECT_EQ(parsed["client_info"]["platform"], "Android");
    EXPECT_EQ(parsed["client_info"]["network_info"]["local_ip"], "192.168.1.100");
    EXPECT_EQ(parsed["client_info"]["network_info"]["public_ip"], "203.0.113.1");
    EXPECT_EQ(parsed["client_info"]["network_info"]["port"], 7755);
}

/**
 * Test: JoinRoomResponse message deserialization with success
 * Verifies successful room join response parsing
 */
TEST_F(RoomClientMessagesTest, JoinRoomResponseDeserializationSuccess) {
    // This test will fail because JoinRoomResponse parsing doesn't exist yet
    
    // ARRANGE
    json response_json = {
        {"type", "join_room_response"},
        {"success", true},
        {"room_id", "room_uuid_joined"},
        {"player_id", "player_slot_2"},
        {"players", json::array({
            {
                {"id", "player_1"},
                {"username", "Host"},
                {"is_host", true},
                {"network_info", {
                    {"local_ip", "192.168.1.1"},
                    {"public_ip", "203.0.113.10"},
                    {"port", 7755}
                }}
            },
            {
                {"id", "player_2"},
                {"username", "JoiningPlayer"},
                {"is_host", false},
                {"network_info", {
                    {"local_ip", "192.168.1.100"},
                    {"public_ip", "203.0.113.1"},
                    {"port", 7755}
                }}
            }
        })}
    };
    
    // ACT
    auto response = MessageDeserializer::Deserialize<JoinRoomResponse>(response_json.dump());
    
    // ASSERT
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.room_id, "room_uuid_joined");
    EXPECT_EQ(response.player_id, "player_slot_2");
    EXPECT_EQ(response.players.size(), 2);
    
    EXPECT_EQ(response.players[0].username, "Host");
    EXPECT_TRUE(response.players[0].is_host);
    EXPECT_EQ(response.players[0].network_info.local_ip, "192.168.1.1");
    
    EXPECT_EQ(response.players[1].username, "JoiningPlayer");
    EXPECT_FALSE(response.players[1].is_host);
    EXPECT_EQ(response.players[1].network_info.port, 7755);
}

/**
 * Test: P2PInfoMessage message deserialization
 * Verifies P2P connection information parsing
 */
TEST_F(RoomClientMessagesTest, P2PInfoMessageDeserialization) {
    // This test will fail because P2PInfoMessage parsing doesn't exist yet
    
    // ARRANGE
    json message_json = {
        {"type", "p2p_info"},
        {"from_player", "player_1"},
        {"to_player", "player_2"},
        {"connection_type", "direct"},
        {"ice_candidates", json::array({
            {
                {"candidate", "candidate:1 1 UDP 2113667326 192.168.1.1 54400 typ host"},
                {"sdp_mid", "0"},
                {"sdp_mline_index", 0}
            }
        })},
        {"session_description", {
            {"type", "offer"},
            {"sdp", "v=0\r\no=- 123456 1 IN IP4 192.168.1.1\r\n..."}
        }}
    };
    
    // ACT
    auto message = MessageDeserializer::Deserialize<P2PInfoMessage>(message_json.dump());
    
    // ASSERT
    EXPECT_EQ(message.from_player, "player_1");
    EXPECT_EQ(message.to_player, "player_2");
    EXPECT_EQ(message.connection_type, "direct");
    EXPECT_EQ(message.ice_candidates.size(), 1);
    EXPECT_EQ(message.ice_candidates[0].candidate, "candidate:1 1 UDP 2113667326 192.168.1.1 54400 typ host");
    EXPECT_EQ(message.session_description.type, "offer");
    EXPECT_TRUE(message.session_description.sdp.find("v=0") != std::string::npos);
}

/**
 * Test: UseProxyMessage message deserialization
 * Verifies proxy server allocation message parsing
 */
TEST_F(RoomClientMessagesTest, UseProxyMessageDeserialization) {
    // This test will fail because UseProxyMessage parsing doesn't exist yet
    
    // ARRANGE
    json message_json = {
        {"type", "use_proxy"},
        {"relay_server", "relay1.sudachi.org"},
        {"relay_port", 8080},
        {"auth_token", "relay_token_abcdef"},
        {"session_id", "proxy_session_12345"},
        {"target_player", "player_3"},
        {"expires_at", 1641027600000}
    };
    
    // ACT
    auto message = MessageDeserializer::Deserialize<UseProxyMessage>(message_json.dump());
    
    // ASSERT
    EXPECT_EQ(message.relay_server, "relay1.sudachi.org");
    EXPECT_EQ(message.relay_port, 8080);
    EXPECT_EQ(message.auth_token, "relay_token_abcdef");
    EXPECT_EQ(message.session_id, "proxy_session_12345");
    EXPECT_EQ(message.target_player, "player_3");
    EXPECT_EQ(message.expires_at, 1641027600000);
}

/**
 * Test: ErrorMessage message deserialization
 * Verifies error message parsing with different error types
 */
TEST_F(RoomClientMessagesTest, ErrorMessageDeserialization) {
    // This test will fail because ErrorMessage parsing doesn't exist yet
    
    // ARRANGE
    json error_json = {
        {"type", "error"},
        {"error_code", "ROOM_FULL"},
        {"message", "The requested room is full"},
        {"details", {
            {"room_id", "room_that_was_full"},
            {"current_players", 4},
            {"max_players", 4}
        }},
        {"retry_after", 30000}
    };
    
    // ACT
    auto error = MessageDeserializer::Deserialize<ErrorMessage>(error_json.dump());
    
    // ASSERT
    EXPECT_EQ(error.error_code, "ROOM_FULL");
    EXPECT_EQ(error.message, "The requested room is full");
    EXPECT_TRUE(error.details.contains("room_id"));
    EXPECT_EQ(error.details["current_players"], 4);
    EXPECT_EQ(error.retry_after, 30000);
}

/**
 * Test: Malformed JSON message handling
 * Verifies that malformed JSON messages are ignored without throwing exceptions
 */
TEST_F(RoomClientMessagesTest, MalformedJsonMessageHandledGracefully) {
    auto client = std::make_unique<RoomClient>(mock_connection_, nullptr);
    client->SetMessageHandler(mock_handler_);

    EXPECT_CALL(*mock_handler_, OnErrorReceived(_)).Times(0);

    std::string invalid_json = "{invalid json structure";
    EXPECT_NO_THROW(client->SimulateIncomingMessage(invalid_json));
}

/**
 * Test: Partial JSON message handling
 * Ensures that truncated JSON payloads do not raise exceptions
 */
TEST_F(RoomClientMessagesTest, PartialJsonMessageHandledGracefully) {
    auto client = std::make_unique<RoomClient>(mock_connection_, nullptr);
    client->SetMessageHandler(mock_handler_);

    EXPECT_CALL(*mock_handler_, OnErrorReceived(_)).Times(0);

    std::string partial_json = "{\"type\":\"room_created\""; // Missing closing brace
    EXPECT_NO_THROW(client->SimulateIncomingMessage(partial_json));
}

/**
 * Test: Unknown message type handling
 * Verifies that unknown message types are handled appropriately
 */
TEST_F(RoomClientMessagesTest, UnknownMessageTypeHandling) {
    // This test will fail because unknown message handling doesn't exist yet
    
    // ARRANGE
    json unknown_json = {
        {"type", "unknown_message_type"},
        {"data", "some data"}
    };
    
    // ACT
    auto result = MessageDeserializer::DeserializeAny(unknown_json.dump());
    
    // ASSERT
    EXPECT_EQ(result.type, MessageType::Unknown);
    EXPECT_FALSE(result.is_valid);
}

/**
 * Test: Message timestamp validation
 * Verifies that message timestamps are within acceptable range
 */
TEST_F(RoomClientMessagesTest, MessageTimestampValidation) {
    // This test will fail because timestamp validation doesn't exist yet
    
    // ARRANGE
    auto current_time = std::chrono::system_clock::now();
    auto current_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time.time_since_epoch()).count();
    
    json message_json = {
        {"type", "register"},
        {"timestamp", current_ms - 60000}, // 1 minute old
        {"client_id", "test_client"}
    };
    
    // ACT
    auto is_valid = MessageValidator::ValidateTimestamp(message_json.dump(), 
                                                       std::chrono::minutes(5)); // 5 minute tolerance
    
    // ASSERT
    EXPECT_TRUE(is_valid);
    
    // Test expired message
    message_json["timestamp"] = current_ms - 600000; // 10 minutes old
    is_valid = MessageValidator::ValidateTimestamp(message_json.dump(),
                                                  std::chrono::minutes(5));
    EXPECT_FALSE(is_valid);
}

/**
 * Test: Message size limits
 * Verifies that oversized messages are rejected
 */
TEST_F(RoomClientMessagesTest, MessageSizeLimits) {
    // This test will fail because size validation doesn't exist yet
    
    // ARRANGE
    std::string large_data(65536, 'A'); // 64KB of data
    json large_message = {
        {"type", "create_room"},
        {"description", large_data}
    };
    
    // ACT & ASSERT
    EXPECT_THROW({
        MessageValidator::ValidateSize(large_message.dump());
    }, MessageTooLargeException);
}

/**
 * Test: Message handler registration and dispatch
 * Verifies that message handlers are properly called for each message type
 */
TEST_F(RoomClientMessagesTest, MessageHandlerDispatch) {
    // This test will fail because message dispatch system doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, nullptr);
    client->SetMessageHandler(mock_handler_);
    
    // Set up expectations
    EXPECT_CALL(*mock_handler_, OnRoomCreated(_));
    EXPECT_CALL(*mock_handler_, OnErrorReceived(_));
    
    // ACT - Simulate receiving messages
    json room_created = {
        {"type", "room_created"},
        {"success", true},
        {"room", {{"id", "test_room"}}}
    };
    
    json error_message = {
        {"type", "error"},
        {"error_code", "TEST_ERROR"},
        {"message", "Test error message"}
    };
    
    mock_connection_->SimulateMessage(room_created.dump());
    mock_connection_->SimulateMessage(error_message.dump());
    
    // ASSERT - Mock expectations verify handlers were called
}
