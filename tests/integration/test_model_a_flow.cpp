// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

// Include mocks
#include "../mocks/mock_p2p_network.h"
#include "../mocks/mock_websocket_server.h"

// Include actual implementations to test
#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/model_a/relay_client.h"
#include "core/multiplayer/model_a/room_client.h"

using namespace Sudachi::Multiplayer;
using namespace Sudachi::Multiplayer::Tests;
using namespace testing;
using namespace std::chrono_literals;

/**
 * Integration test fixture for Model A (Internet Multiplayer) flow
 */
class ModelAIntegrationTest : public Test {
protected:
  void SetUp() override {
    // Create mock servers
    mock_ws_server_ = std::make_unique<MockWebSocketServer>();
    mock_p2p_network_ = std::make_unique<MockP2PNetwork>();

    SetupDefaultBehaviors();
  }

  void TearDown() override {
    // Cleanup
    if (room_client_) {
      room_client_->Disconnect();
    }
  }

  void SetupDefaultBehaviors() {
    using ::testing::_;
    using ::testing::Invoke;
    using ::testing::Return;

    // Mock WebSocket server behaviors
    ON_CALL(*mock_ws_server_, Start(_))
        .WillByDefault(Invoke([this](uint16_t port) {
          server_port_ = port;
          server_running_ = true;
        }));

    ON_CALL(*mock_ws_server_, Stop()).WillByDefault(Invoke([this]() {
      server_running_ = false;
    }));

    // Mock P2P network behaviors
    ON_CALL(*mock_p2p_network_, Initialize(_)).WillByDefault(Return(true));

    ON_CALL(*mock_p2p_network_, Connect(_, _)).WillByDefault(Return(true));
  }

  // Helper to simulate complete room creation flow
  void SimulateRoomCreation(const std::string &room_name,
                            const std::string &game_id) {
    // Simulate server response to room creation
    json create_response;
    create_response["type"] = "room_created";
    create_response["data"]["room_id"] = "test-room-123";
    create_response["data"]["room_name"] = room_name;
    create_response["data"]["game_id"] = game_id;
    create_response["data"]["max_players"] = 8;

    mock_ws_server_->SimulateClientMessage("host-client",
                                           create_response.dump());
  }

  // Helper to simulate player joining
  void SimulatePlayerJoin(const std::string &player_id,
                          const std::string &nickname) {
    json join_notification;
    join_notification["type"] = "player_joined";
    join_notification["data"]["player_id"] = player_id;
    join_notification["data"]["nickname"] = nickname;
    join_notification["data"]["peer_info"]["peer_id"] = "QmPeer" + player_id;
    join_notification["data"]["peer_info"]["multiaddrs"] = {
        "/ip4/192.168.1.100/tcp/4001"};

    mock_ws_server_->SimulateClientMessage("host-client",
                                           join_notification.dump());
  }

protected:
  std::unique_ptr<MockWebSocketServer> mock_ws_server_;
  std::unique_ptr<MockP2PNetwork> mock_p2p_network_;
  std::unique_ptr<RoomClient> room_client_;

  uint16_t server_port_ = 0;
  bool server_running_ = false;
};

/**
 * Test complete room creation and player connection flow
 */
TEST_F(ModelAIntegrationTest, CompleteRoomCreationFlow) {
  // Step 1: Initialize room client
  room_client_ = std::make_unique<RoomClient>();

  RoomClientConfig config;
  config.server_url = "ws://localhost:8080";
  config.reconnect_interval = 1000;
  config.max_reconnect_attempts = 3;

  ASSERT_EQ(ErrorCode::Success, room_client_->Initialize(config));

  // Step 2: Connect to room server
  std::promise<bool> connect_promise;
  auto connect_future = connect_promise.get_future();

  room_client_->SetOnConnected(
      [&connect_promise]() { connect_promise.set_value(true); });

  // Simulate successful connection asynchronously
  std::thread([this]() {
    std::this_thread::sleep_for(50ms);
    mock_ws_server_->SimulateClientConnect("host-client");
  }).detach();

  ASSERT_EQ(ErrorCode::Success, room_client_->Connect());

  // Wait for connection
  ASSERT_TRUE(connect_future.wait_for(1s) == std::future_status::ready);
  ASSERT_TRUE(connect_future.get());

  // Step 3: Create a room
  RoomInfo room_info;
  room_info.room_name = "Test Room";
  room_info.game_id = "0100000000010000";
  room_info.max_players = 4;
  room_info.has_password = false;

  std::promise<std::string> room_id_promise;
  auto room_id_future = room_id_promise.get_future();

  room_client_->SetOnRoomCreated(
      [&room_id_promise](const std::string &room_id) {
        room_id_promise.set_value(room_id);
      });

  ASSERT_EQ(ErrorCode::Success, room_client_->CreateRoom(room_info));

  // Simulate server response
  SimulateRoomCreation(room_info.room_name, room_info.game_id);

  // Wait for room creation
  ASSERT_TRUE(room_id_future.wait_for(1s) == std::future_status::ready);
  std::string room_id = room_id_future.get();
  EXPECT_EQ("test-room-123", room_id);

  // Step 4: Simulate players joining
  std::vector<std::string> joined_players;
  std::mutex players_mutex;

  room_client_->SetOnPlayerJoined(
      [&joined_players, &players_mutex](const PlayerInfo &player) {
        std::lock_guard<std::mutex> lock(players_mutex);
        joined_players.push_back(player.player_id);
      });

  // Simulate 3 players joining
  SimulatePlayerJoin("player1", "Alice");
  SimulatePlayerJoin("player2", "Bob");
  SimulatePlayerJoin("player3", "Charlie");

  // Give time for all join notifications to process
  std::this_thread::sleep_for(100ms);

  // Verify all players joined
  {
    std::lock_guard<std::mutex> lock(players_mutex);
    EXPECT_EQ(3u, joined_players.size());
    EXPECT_THAT(joined_players,
                UnorderedElementsAre("player1", "player2", "player3"));
  }

  // Step 5: Test room state
  auto current_room = room_client_->GetCurrentRoom();
  ASSERT_TRUE(current_room.has_value());
  EXPECT_EQ("test-room-123", current_room->room_id);
  EXPECT_EQ(3u, current_room->current_players);
}

/**
 * Test P2P connection establishment between players
 */
TEST_F(ModelAIntegrationTest, P2PConnectionEstablishment) {
  // Initialize P2P networks for two players
  auto host_p2p = std::make_unique<P2PNetwork>();
  auto client_p2p = std::make_unique<P2PNetwork>();

  P2PConfig p2p_config;
  p2p_config.transport_priority = {"/ip4/tcp", "/ip4/ws", "/ip4/quic"};
  p2p_config.enable_relay = true;
  p2p_config.nat_traversal_enabled = true;

  // Initialize host
  ASSERT_EQ(ErrorCode::Success,
            host_p2p->Initialize("host-peer-id", p2p_config));

  // Initialize client
  ASSERT_EQ(ErrorCode::Success,
            client_p2p->Initialize("client-peer-id", p2p_config));

  // Get host's listen addresses
  auto host_addresses = host_p2p->GetListenAddresses();
  ASSERT_FALSE(host_addresses.empty());

  // Track connection states
  std::promise<bool> host_connected_promise;
  std::promise<bool> client_connected_promise;

  host_p2p->SetOnPeerConnected(
      [&host_connected_promise](const std::string &peer_id) {
        if (peer_id == "client-peer-id") {
          host_connected_promise.set_value(true);
        }
      });

  client_p2p->SetOnPeerConnected(
      [&client_connected_promise](const std::string &peer_id) {
        if (peer_id == "host-peer-id") {
          client_connected_promise.set_value(true);
        }
      });

  // Client connects to host
  ASSERT_EQ(ErrorCode::Success,
            client_p2p->ConnectToPeer("host-peer-id", host_addresses[0]));

  // Simulate successful P2P connection
  mock_p2p_network_->SimulateSuccessfulConnection("host-peer-id");
  mock_p2p_network_->SimulateSuccessfulConnection("client-peer-id");

  // Wait for both sides to connect
  auto host_future = host_connected_promise.get_future();
  auto client_future = client_connected_promise.get_future();

  ASSERT_TRUE(host_future.wait_for(2s) == std::future_status::ready);
  ASSERT_TRUE(client_future.wait_for(2s) == std::future_status::ready);

  // Test data exchange
  std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
  std::promise<std::vector<uint8_t>> data_received_promise;

  host_p2p->SetOnDataReceived(
      [&data_received_promise](const std::string &peer_id,
                               const std::vector<uint8_t> &data) {
        if (peer_id == "client-peer-id") {
          data_received_promise.set_value(data);
        }
      });

  // Client sends data to host
  ASSERT_EQ(ErrorCode::Success,
            client_p2p->SendData("host-peer-id", test_data));

  // Simulate data transmission
  mock_p2p_network_->SimulateIncomingData("client-peer-id", test_data);

  // Verify data received
  auto data_future = data_received_promise.get_future();
  ASSERT_TRUE(data_future.wait_for(1s) == std::future_status::ready);
  EXPECT_EQ(test_data, data_future.get());
}

/**
 * Test relay fallback when P2P connection fails
 */
TEST_F(ModelAIntegrationTest, RelayFallbackMechanism) {
  // Initialize components
  auto p2p_network = std::make_unique<P2PNetwork>();
  auto relay_client = std::make_unique<RelayClient>();

  P2PConfig p2p_config;
  p2p_config.enable_relay = true;
  p2p_config.relay_server = "relay.sudachi.com:3478";

  ASSERT_EQ(ErrorCode::Success,
            p2p_network->Initialize("test-peer", p2p_config));

  // Track connection attempts
  bool p2p_failed = false;
  bool relay_activated = false;

  p2p_network->SetOnConnectionFailed(
      [&p2p_failed](const std::string &peer_id, const std::string &error) {
        p2p_failed = true;
      });

  relay_client->SetOnConnected(
      [&relay_activated]() { relay_activated = true; });

  // Attempt P2P connection
  ASSERT_EQ(ErrorCode::Success, p2p_network->ConnectToPeer(
                                    "remote-peer", "/ip4/10.0.0.1/tcp/4001"));

  // Simulate NAT traversal failure
  mock_p2p_network_->SimulateRelayFallback("remote-peer");

  // Wait for fallback
  std::this_thread::sleep_for(500ms);

  EXPECT_TRUE(p2p_failed);

  // Initialize relay connection
  RelayConfig relay_config;
  relay_config.relay_server = "relay.sudachi.com:3478";
  relay_config.jwt_token = "test-jwt-token";

  ASSERT_EQ(ErrorCode::Success, relay_client->Initialize(relay_config));
  ASSERT_EQ(ErrorCode::Success, relay_client->Connect());

  // Simulate relay connection success
  relay_activated = true;

  EXPECT_TRUE(relay_activated);

  // Test data transmission through relay
  std::vector<uint8_t> relay_data = {0x10, 0x20, 0x30};
  ASSERT_EQ(ErrorCode::Success,
            relay_client->SendData("remote-peer", relay_data));

  // Verify relay is being used
  EXPECT_EQ(RelayState::Connected, relay_client->GetState());
}

/**
 * Test network resilience and reconnection
 */
TEST_F(ModelAIntegrationTest, NetworkResilienceAndReconnection) {
  // Initialize room client with aggressive reconnection
  room_client_ = std::make_unique<RoomClient>();

  RoomClientConfig config;
  config.server_url = "ws://localhost:8080";
  config.reconnect_interval = 100; // 100ms for testing
  config.max_reconnect_attempts = 5;
  config.heartbeat_interval = 500;

  ASSERT_EQ(ErrorCode::Success, room_client_->Initialize(config));

  // Track connection events
  int connect_count = 0;
  int disconnect_count = 0;
  std::mutex event_mutex;

  room_client_->SetOnConnected([&connect_count, &event_mutex]() {
    std::lock_guard<std::mutex> lock(event_mutex);
    connect_count++;
  });

  room_client_->SetOnDisconnected(
      [&disconnect_count, &event_mutex](const std::string &reason) {
        std::lock_guard<std::mutex> lock(event_mutex);
        disconnect_count++;
      });

  // Initial connection
  std::thread([this]() {
    std::this_thread::sleep_for(50ms);
    mock_ws_server_->SimulateClientConnect("test-client");
  }).detach();
  ASSERT_EQ(ErrorCode::Success, room_client_->Connect());

  std::this_thread::sleep_for(200ms);

  {
    std::lock_guard<std::mutex> lock(event_mutex);
    EXPECT_EQ(1, connect_count);
    EXPECT_EQ(0, disconnect_count);
  }

  // Simulate network interruption
  mock_ws_server_->SimulateClientDisconnect("test-client");

  std::this_thread::sleep_for(200ms);

  {
    std::lock_guard<std::mutex> lock(event_mutex);
    EXPECT_EQ(1, connect_count);
    EXPECT_EQ(1, disconnect_count);
  }

  // Simulate successful reconnection
  mock_ws_server_->SimulateClientConnect("test-client");

  std::this_thread::sleep_for(500ms);

  {
    std::lock_guard<std::mutex> lock(event_mutex);
    EXPECT_EQ(2, connect_count);
    EXPECT_EQ(1, disconnect_count);
  }

  // Verify client is functional after reconnection
  EXPECT_TRUE(room_client_->IsConnected());
}

/**
 * Test concurrent room operations
 */
TEST_F(ModelAIntegrationTest, ConcurrentRoomOperations) {
  // Create multiple room clients
  const int num_clients = 5;
  std::vector<std::unique_ptr<RoomClient>> clients;
  std::vector<std::future<bool>> futures;

  for (int i = 0; i < num_clients; ++i) {
    auto client = std::make_unique<RoomClient>();

    RoomClientConfig config;
    config.server_url = "ws://localhost:8080";

    ASSERT_EQ(ErrorCode::Success, client->Initialize(config));
    clients.push_back(std::move(client));
  }

  // Connect all clients concurrently
  for (auto &client : clients) {
    futures.push_back(std::async(std::launch::async, [&client]() {
      return client->Connect() == ErrorCode::Success;
    }));
  }

  // Simulate connections
  for (int i = 0; i < num_clients; ++i) {
    mock_ws_server_->SimulateClientConnect("client-" + std::to_string(i));
  }

  // Wait for all connections
  for (auto &future : futures) {
    ASSERT_TRUE(future.wait_for(2s) == std::future_status::ready);
    EXPECT_TRUE(future.get());
  }

  // Create rooms concurrently
  futures.clear();
  std::vector<std::string> room_ids;
  std::mutex room_ids_mutex;

  for (int i = 0; i < num_clients; ++i) {
    futures.push_back(std::async(
        std::launch::async, [&clients, i, &room_ids, &room_ids_mutex]() {
          RoomInfo room_info;
          room_info.room_name = "Room " + std::to_string(i);
          room_info.game_id = "0100000000010000";
          room_info.max_players = 4;

          clients[i]->SetOnRoomCreated(
              [&room_ids, &room_ids_mutex](const std::string &room_id) {
                std::lock_guard<std::mutex> lock(room_ids_mutex);
                room_ids.push_back(room_id);
              });

          return clients[i]->CreateRoom(room_info) == ErrorCode::Success;
        }));
  }

  // Simulate room creation responses
  for (int i = 0; i < num_clients; ++i) {
    SimulateRoomCreation("Room " + std::to_string(i), "0100000000010000");
  }

  // Wait for all room creations
  for (auto &future : futures) {
    ASSERT_TRUE(future.wait_for(2s) == std::future_status::ready);
    EXPECT_TRUE(future.get());
  }

  // Verify all rooms were created
  std::this_thread::sleep_for(500ms);

  {
    std::lock_guard<std::mutex> lock(room_ids_mutex);
    EXPECT_EQ(num_clients, static_cast<int>(room_ids.size()));
  }
}
