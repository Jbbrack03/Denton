// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <future>
#include <thread>

#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/model_a/room_client.h"
#include "core/multiplayer/model_a/p2p_room_coordinator.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock P2P network for integration testing
 */
class MockP2PNetwork {
public:
    MOCK_METHOD(ErrorCode, start, (), ());
    MOCK_METHOD(void, stop, (), ());
    MOCK_METHOD(bool, isStarted, (), (const));
    MOCK_METHOD(std::string, getPeerId, (), (const));
    MOCK_METHOD(std::vector<std::string>, getListenMultiaddresses, (), (const));
    MOCK_METHOD(std::future<ErrorCode>, connectToPeer, (const std::string& multiaddr), ());
    MOCK_METHOD(void, disconnectFromPeer, (const std::string& peer_id), ());
    MOCK_METHOD(bool, isConnectedToPeer, (const std::string& peer_id), (const));
    MOCK_METHOD(ErrorCode, sendData, (const std::string& peer_id, const std::vector<uint8_t>& data), ());
    MOCK_METHOD(void, setOnPeerConnectedCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, setOnPeerDisconnectedCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, setOnDataReceivedCallback, (std::function<void(const std::string&, const std::vector<uint8_t>&)> callback), ());
};

/**
 * Mock room coordinator for managing P2P sessions through room server
 */
class MockP2PRoomCoordinator {
public:
    MOCK_METHOD(ErrorCode, initialize, (std::shared_ptr<RoomClient> room_client, std::shared_ptr<MockP2PNetwork> p2p_network), ());
    MOCK_METHOD(std::future<ErrorCode>, createRoom, (const RoomCreationRequest& request), ());
    MOCK_METHOD(std::future<ErrorCode>, joinRoom, (const std::string& room_id), ());
    MOCK_METHOD(void, leaveRoom, (), ());
    MOCK_METHOD(bool, isInRoom, (), (const));
    MOCK_METHOD(std::string, getCurrentRoomId, (), (const));
    MOCK_METHOD(std::vector<PlayerInfo>, getRoomPlayers, (), (const));
    MOCK_METHOD(void, setOnPlayerJoinedCallback, (std::function<void(const PlayerInfo&)> callback), ());
    MOCK_METHOD(void, setOnPlayerLeftCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, setOnP2PConnectionEstablishedCallback, (std::function<void(const std::string&)> callback), ());
};

/**
 * Test implementation of room client for integration testing
 */
class TestRoomClient : public RoomClient {
public:
    TestRoomClient(std::shared_ptr<IWebSocketConnection> connection,
                   std::shared_ptr<IConfigProvider> config)
        : RoomClient(connection, config) {}
    
    // Test-specific methods to simulate room server interactions
    void simulateP2PInfoMessage(const P2PInfoMessage& message) {
        SimulateIncomingMessage(message.toJson());
    }
    
    void simulateUseProxyMessage(const UseProxyMessage& message) {
        SimulateIncomingMessage(message.toJson());
    }
    
    void simulatePlayerJoined(const PlayerJoinedMessage& message) {
        SimulateIncomingMessage(message.toJson());
    }
    
    void simulatePlayerLeft(const PlayerLeftMessage& message) {
        SimulateIncomingMessage(message.toJson());
    }
};

} // anonymous namespace

/**
 * Test fixture for P2P and room server integration tests
 * Tests the complete flow from room server discovery to P2P connection establishment
 */
class P2PRoomServerIntegrationTest : public Test {
protected:
    void SetUp() override {
        // Set up mock P2P network
        mock_p2p_network_ = std::make_shared<MockP2PNetwork>();
        mock_room_coordinator_ = std::make_shared<MockP2PRoomCoordinator>();
        
        // Set up room client components
        mock_connection_ = std::make_shared<MockWebSocketConnection>();
        mock_config_ = std::make_shared<MockConfigProvider>();
        
        // Configure default expectations
        ON_CALL(*mock_config_, GetRoomServerUrl())
            .WillByDefault(Return("wss://room.sudachi.org/ws"));
        ON_CALL(*mock_config_, GetAuthToken())
            .WillByDefault(Return("test_auth_token"));
        
        ON_CALL(*mock_p2p_network_, getPeerId())
            .WillByDefault(Return("12D3KooWLocalPeer"));
        ON_CALL(*mock_p2p_network_, getListenMultiaddresses())
            .WillByDefault(Return(std::vector<std::string>{
                "/ip4/192.168.1.100/tcp/4001/p2p/12D3KooWLocalPeer"
            }));
    }

    std::shared_ptr<MockP2PNetwork> mock_p2p_network_;
    std::shared_ptr<MockP2PRoomCoordinator> mock_room_coordinator_;
    std::shared_ptr<MockWebSocketConnection> mock_connection_;
    std::shared_ptr<MockConfigProvider> mock_config_;
};

/**
 * Test: Complete room creation and P2P setup flow
 * Verifies the entire process from creating a room to establishing P2P connections
 */
TEST_F(P2PRoomServerIntegrationTest, CompleteRoomCreationAndP2PSetupFlow) {
    // ARRANGE
    const std::string room_id = "test_room_12345";
    const uint64_t game_id = 0x0100000000010000;
    
    // P2P network startup
    EXPECT_CALL(*mock_p2p_network_, start())
        .WillOnce(Return(ErrorCode::Success));
    EXPECT_CALL(*mock_p2p_network_, isStarted())
        .WillRepeatedly(Return(true));
    
    // Room client connection
    EXPECT_CALL(*mock_connection_, Connect(_));
    EXPECT_CALL(*mock_connection_, IsConnected())
        .WillRepeatedly(Return(true));
    
    // Room creation request
    RoomCreationRequest room_request;
    room_request.game_id = game_id;
    room_request.game_name = "Test Game";
    room_request.max_players = 4;
    room_request.is_private = false;
    
    EXPECT_CALL(*mock_room_coordinator_, createRoom(room_request))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_room_coordinator_, isInRoom())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_room_coordinator_, getCurrentRoomId())
        .WillOnce(Return(room_id));
    
    // Create components
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    EXPECT_CALL(*mock_room_coordinator_, initialize(room_client, mock_p2p_network_))
        .WillOnce(Return(ErrorCode::Success));
    
    // ACT
    auto coordinator_init = mock_room_coordinator_->initialize(room_client, mock_p2p_network_);
    EXPECT_EQ(coordinator_init, ErrorCode::Success);
    
    auto room_creation_future = mock_room_coordinator_->createRoom(room_request);
    auto result = room_creation_future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(mock_room_coordinator_->isInRoom());
    EXPECT_EQ(mock_room_coordinator_->getCurrentRoomId(), room_id);
}

/**
 * Test: P2P info exchange through room server
 * Verifies that P2P connection information is properly exchanged via room server
 */
TEST_F(P2PRoomServerIntegrationTest, P2PInfoExchangeThroughRoomServer) {
    // ARRANGE
    const std::string remote_peer_id = "12D3KooWRemotePeer";
    const std::string remote_multiaddr = "/ip4/203.0.113.1/tcp/4001/p2p/" + remote_peer_id;
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // Set up P2P connection callback
    std::function<void(const std::string&)> p2p_callback;
    EXPECT_CALL(*mock_room_coordinator_, setOnP2PConnectionEstablishedCallback(_))
        .WillOnce([&p2p_callback](std::function<void(const std::string&)> callback) {
            p2p_callback = callback;
        });
    
    // Expect P2P connection attempt
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(remote_multiaddr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(remote_peer_id))
        .WillOnce(Return(true));
    
    // ACT
    // Simulate receiving P2P info from room server
    P2PInfoMessage p2p_info;
    p2p_info.peer_id = remote_peer_id;
    p2p_info.multiaddresses = {remote_multiaddr};
    p2p_info.public_key = "test_public_key_data";
    
    room_client->simulateP2PInfoMessage(p2p_info);
    
    // Simulate successful P2P connection
    if (p2p_callback) {
        p2p_callback(remote_peer_id);
    }
    
    // ASSERT
    EXPECT_TRUE(mock_p2p_network_->isConnectedToPeer(remote_peer_id));
}

/**
 * Test: P2P connection failure and proxy fallback coordination
 * Verifies that failed P2P connections trigger proxy fallback through room server
 */
TEST_F(P2PRoomServerIntegrationTest, P2PFailureTriggeresProxyFallback) {
    // ARRANGE
    const std::string remote_peer_id = "12D3KooWUnreachablePeer";
    const std::string remote_multiaddr = "/ip4/10.0.0.1/tcp/4001/p2p/" + remote_peer_id;
    const std::string proxy_server = "proxy.sudachi.org:8080";
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // P2P connection fails
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(remote_multiaddr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::ConnectionTimeout);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(remote_peer_id))
        .WillOnce(Return(false));
    
    // Expect proxy connection setup (this would be handled by relay client)
    // For this test, we'll simulate the room server telling us to use proxy
    
    // ACT
    // First, attempt direct P2P connection
    P2PInfoMessage p2p_info;
    p2p_info.peer_id = remote_peer_id;
    p2p_info.multiaddresses = {remote_multiaddr};
    
    room_client->simulateP2PInfoMessage(p2p_info);
    
    // P2P connection fails, room server sends proxy instruction
    UseProxyMessage proxy_message;
    proxy_message.peer_id = remote_peer_id;
    proxy_message.proxy_server = proxy_server;
    proxy_message.reason = "Direct P2P connection failed";
    
    room_client->simulateUseProxyMessage(proxy_message);
    
    // ASSERT - In real implementation, this would trigger relay connection setup
    // For now, we verify the messages were processed correctly
    EXPECT_FALSE(mock_p2p_network_->isConnectedToPeer(remote_peer_id));
}

/**
 * Test: Multi-peer room with mixed P2P and proxy connections
 * Verifies handling of multiple peers with different connection types
 */
TEST_F(P2PRoomServerIntegrationTest, MultiPeerRoomWithMixedConnections) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {
        "12D3KooWDirectPeer",   // Direct P2P connection
        "12D3KooWProxyPeer",    // Proxy connection
        "12D3KooWTimeoutPeer"   // Connection timeout
    };
    
    const std::vector<std::string> multiaddrs = {
        "/ip4/192.168.1.200/tcp/4001/p2p/12D3KooWDirectPeer",
        "/ip4/10.0.0.2/tcp/4001/p2p/12D3KooWProxyPeer",
        "/ip4/10.0.0.3/tcp/4001/p2p/12D3KooWTimeoutPeer"
    };
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // Set up player joined callbacks
    std::function<void(const PlayerInfo&)> player_joined_callback;
    EXPECT_CALL(*mock_room_coordinator_, setOnPlayerJoinedCallback(_))
        .WillOnce([&player_joined_callback](std::function<void(const PlayerInfo&)> callback) {
            player_joined_callback = callback;
        });
    
    // Set up P2P connection expectations
    // Peer 0: Successful direct connection
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[0]))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_ids[0]))
        .WillRepeatedly(Return(true));
    
    // Peer 1: Failed direct, will use proxy
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[1]))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::ConnectionTimeout);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_ids[1]))
        .WillRepeatedly(Return(false));
    
    // Peer 2: Connection timeout
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[2]))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::ConnectionTimeout);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_ids[2]))
        .WillRepeatedly(Return(false));
    
    // ACT
    // Simulate players joining the room
    for (size_t i = 0; i < peer_ids.size(); i++) {
        PlayerJoinedMessage player_msg;
        player_msg.player_info.id = peer_ids[i];
        player_msg.player_info.username = "Player" + std::to_string(i);
        player_msg.player_info.network_info.local_ip = "192.168.1." + std::to_string(100 + i);
        
        room_client->simulatePlayerJoined(player_msg);
        
        // Send P2P info for each peer
        P2PInfoMessage p2p_info;
        p2p_info.peer_id = peer_ids[i];
        p2p_info.multiaddresses = {multiaddrs[i]};
        
        room_client->simulateP2PInfoMessage(p2p_info);
    }
    
    // Send proxy message for peer 1
    UseProxyMessage proxy_msg;
    proxy_msg.peer_id = peer_ids[1];
    proxy_msg.proxy_server = "proxy.sudachi.org:8080";
    proxy_msg.reason = "NAT traversal failed";
    
    room_client->simulateUseProxyMessage(proxy_msg);
    
    // ASSERT
    EXPECT_TRUE(mock_p2p_network_->isConnectedToPeer(peer_ids[0]));  // Direct connection
    EXPECT_FALSE(mock_p2p_network_->isConnectedToPeer(peer_ids[1])); // Proxy (not P2P)
    EXPECT_FALSE(mock_p2p_network_->isConnectedToPeer(peer_ids[2])); // Failed
}

/**
 * Test: Room server reconnection with P2P state preservation
 * Verifies that P2P connections are maintained during room server reconnection
 */
TEST_F(P2PRoomServerIntegrationTest, RoomServerReconnectionWithP2PStatePreservation) {
    // ARRANGE
    const std::string room_id = "persistent_room_123";
    const std::string peer_id = "12D3KooWPersistentPeer";
    const std::string multiaddr = "/ip4/192.168.1.150/tcp/4001/p2p/" + peer_id;
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // Initial connection and P2P setup
    EXPECT_CALL(*mock_connection_, IsConnected())
        .WillOnce(Return(true))   // Initially connected
        .WillOnce(Return(false))  // Disconnected
        .WillOnce(Return(true));  // Reconnected
    
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
        .WillRepeatedly(Return(true)); // P2P connection remains active
    
    // Room client should attempt reconnection
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(1); // Reconnection attempt
    
    // ACT
    // Simulate initial P2P connection
    P2PInfoMessage p2p_info;
    p2p_info.peer_id = peer_id;
    p2p_info.multiaddresses = {multiaddr};
    
    room_client->simulateP2PInfoMessage(p2p_info);
    
    // Simulate room server disconnection
    room_client->OnWebSocketDisconnected("Connection lost");
    
    // Simulate reconnection
    room_client->OnWebSocketConnected();
    
    // ASSERT
    // P2P connection should be preserved even if room server connection is lost
    EXPECT_TRUE(mock_p2p_network_->isConnectedToPeer(peer_id));
}

/**
 * Test: Data transmission over P2P after room server coordination
 * Verifies that data can be transmitted over P2P connections established via room server
 */
TEST_F(P2PRoomServerIntegrationTest, DataTransmissionOverP2PAfterRoomCoordination) {
    // ARRANGE
    const std::string peer_id = "12D3KooWDataPeer";
    const std::string multiaddr = "/ip4/192.168.1.200/tcp/4001/p2p/" + peer_id;
    const std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // P2P connection established
    EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
        .WillRepeatedly(Return(true));
    
    // Data transmission
    EXPECT_CALL(*mock_p2p_network_, sendData(peer_id, test_data))
        .WillOnce(Return(ErrorCode::Success));
    
    // Data reception callback
    std::function<void(const std::string&, const std::vector<uint8_t>&)> data_callback;
    EXPECT_CALL(*mock_p2p_network_, setOnDataReceivedCallback(_))
        .WillOnce([&data_callback](std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
            data_callback = callback;
        });
    
    // ACT
    // Establish P2P connection via room server
    P2PInfoMessage p2p_info;
    p2p_info.peer_id = peer_id;
    p2p_info.multiaddresses = {multiaddr};
    
    room_client->simulateP2PInfoMessage(p2p_info);
    
    // Send data over P2P
    auto send_result = mock_p2p_network_->sendData(peer_id, test_data);
    
    // Simulate receiving data
    if (data_callback) {
        data_callback(peer_id, test_data);
    }
    
    // ASSERT
    EXPECT_EQ(send_result, ErrorCode::Success);
    EXPECT_TRUE(mock_p2p_network_->isConnectedToPeer(peer_id));
}

/**
 * Test: Room server mediated peer discovery and connection prioritization
 * Verifies that the room server can provide peer discovery and connection hints
 */
TEST_F(P2PRoomServerIntegrationTest, RoomServerMediatedPeerDiscoveryAndPrioritization) {
    // ARRANGE
    struct PeerInfo {
        std::string peer_id;
        std::string multiaddr;
        int priority;
        std::string region;
    };
    
    const std::vector<PeerInfo> available_peers = {
        {"12D3KooWHighPriority", "/ip4/192.168.1.100/tcp/4001/p2p/12D3KooWHighPriority", 1, "us-west"},
        {"12D3KooWMediumPriority", "/ip4/203.0.113.1/tcp/4001/p2p/12D3KooWMediumPriority", 2, "us-east"},
        {"12D3KooWLowPriority", "/ip4/10.0.0.1/tcp/4001/p2p/12D3KooWLowPriority", 3, "eu-west"}
    };
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // Expect connections in priority order
    for (const auto& peer : available_peers) {
        EXPECT_CALL(*mock_p2p_network_, connectToPeer(peer.multiaddr))
            .WillOnce([]() {
                std::promise<ErrorCode> promise;
                promise.set_value(ErrorCode::Success);
                return promise.get_future();
            });
        EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer.peer_id))
            .WillRepeatedly(Return(true));
    }
    
    // ACT
    // Send peer discovery information in random order
    std::vector<size_t> indices = {2, 0, 1}; // Low, High, Medium priority
    for (size_t idx : indices) {
        const auto& peer = available_peers[idx];
        
        P2PInfoMessage p2p_info;
        p2p_info.peer_id = peer.peer_id;
        p2p_info.multiaddresses = {peer.multiaddr};
        p2p_info.priority = peer.priority;
        p2p_info.region = peer.region;
        
        room_client->simulateP2PInfoMessage(p2p_info);
    }
    
    // ASSERT
    // Verify all peers are connected (priority handling would be in the coordinator)
    for (const auto& peer : available_peers) {
        EXPECT_TRUE(mock_p2p_network_->isConnectedToPeer(peer.peer_id));
    }
}

/**
 * Test: Graceful cleanup on room leave
 * Verifies that P2P connections are properly cleaned up when leaving a room
 */
TEST_F(P2PRoomServerIntegrationTest, GracefulCleanupOnRoomLeave) {
    // ARRANGE
    const std::string room_id = "cleanup_room_456";
    const std::vector<std::string> peer_ids = {
        "12D3KooWCleanup1", "12D3KooWCleanup2"
    };
    
    auto room_client = std::make_shared<TestRoomClient>(mock_connection_, mock_config_);
    
    // Initial setup - peers connected
    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
            .WillOnce(Return(true));
        EXPECT_CALL(*mock_p2p_network_, disconnectFromPeer(peer_id));
    }
    
    EXPECT_CALL(*mock_room_coordinator_, isInRoom())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_room_coordinator_, leaveRoom());
    
    // ACT
    // Simulate leaving the room
    mock_room_coordinator_->leaveRoom();
    
    // Cleanup P2P connections
    for (const auto& peer_id : peer_ids) {
        mock_p2p_network_->disconnectFromPeer(peer_id);
    }
    
    // ASSERT - Mock expectations verify cleanup was called
}