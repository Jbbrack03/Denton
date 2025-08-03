// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <future>
#include <atomic>

#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock P2P connection for testing connection lifecycle
 */
class MockP2PConnection {
public:
    MOCK_METHOD(void, connect, (const std::string& multiaddr), ());
    MOCK_METHOD(void, disconnect, (), ());
    MOCK_METHOD(bool, isConnected, (), (const));
    MOCK_METHOD(std::string, getPeerId, (), (const));
    MOCK_METHOD(std::string, getRemoteMultiaddr, (), (const));
    MOCK_METHOD(void, send, (const std::vector<uint8_t>& data), ());
    MOCK_METHOD(void, setOnDataCallback, (std::function<void(const std::vector<uint8_t>&)> callback), ());
    MOCK_METHOD(void, setOnCloseCallback, (std::function<void(const std::string&)> callback), ());
};

/**
 * Mock connection manager for testing connection pooling
 */
class MockConnectionManager {
public:
    MOCK_METHOD(std::shared_ptr<MockP2PConnection>, createConnection, (const std::string& peer_id), ());
    MOCK_METHOD(void, closeConnection, (const std::string& peer_id), ());
    MOCK_METHOD(std::shared_ptr<MockP2PConnection>, getConnection, (const std::string& peer_id), ());
    MOCK_METHOD(std::vector<std::string>, getConnectedPeers, (), (const));
    MOCK_METHOD(size_t, getConnectionCount, (), (const));
    MOCK_METHOD(void, closeAllConnections, (), ());
};

/**
 * Mock event listener for connection events
 */
class MockConnectionEventListener {
public:
    MOCK_METHOD(void, onConnectionEstablished, (const std::string& peer_id, const std::string& multiaddr), ());
    MOCK_METHOD(void, onConnectionFailed, (const std::string& peer_id, const std::string& error), ());
    MOCK_METHOD(void, onConnectionClosed, (const std::string& peer_id, const std::string& reason), ());
    MOCK_METHOD(void, onDataReceived, (const std::string& peer_id, const std::vector<uint8_t>& data), ());
};

} // anonymous namespace

/**
 * Test fixture for P2P connection lifecycle tests
 * Covers connection establishment, maintenance, and cleanup
 */
class P2PConnectionLifecycleTest : public Test {
protected:
    void SetUp() override {
        mock_connection_ = std::make_shared<MockP2PConnection>();
        mock_connection_manager_ = std::make_shared<MockConnectionManager>();
        mock_event_listener_ = std::make_shared<MockConnectionEventListener>();
        
        // Set up basic P2P network configuration
        config_.enable_tcp = true;
        config_.tcp_port = 4001;
        config_.max_connections = 10;
        config_.connection_timeout_ms = 5000;
    }

    std::shared_ptr<MockP2PConnection> mock_connection_;
    std::shared_ptr<MockConnectionManager> mock_connection_manager_;
    std::shared_ptr<MockConnectionEventListener> mock_event_listener_;
    P2PNetworkConfig config_;
};

/**
 * Test: Successful P2P connection establishment
 * Verifies that direct P2P connections can be established successfully
 */
TEST_F(P2PConnectionLifecycleTest, EstablishesDirectConnectionSuccessfully) {
    // ARRANGE
    const std::string peer_id = "12D3KooWTest456";
    const std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .WillOnce(Return(mock_connection_));
    EXPECT_CALL(*mock_connection_, connect(multiaddr));
    EXPECT_CALL(*mock_connection_, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_connection_, getPeerId())
        .WillOnce(Return(peer_id));
    EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, multiaddr));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto future = p2p_network->connectToPeer(multiaddr);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(p2p_network->isConnectedToPeer(peer_id));
    EXPECT_EQ(p2p_network->getConnectionCount(), 1);
}

/**
 * Test: Connection timeout handling
 * Verifies that connection attempts timeout properly when peer is unreachable
 */
TEST_F(P2PConnectionLifecycleTest, HandlesConnectionTimeout) {
    // ARRANGE
    config_.connection_timeout_ms = 100; // Short timeout for testing
    const std::string peer_id = "12D3KooWUnreachable";
    const std::string multiaddr = "/ip4/10.0.0.1/tcp/4001/p2p/" + peer_id;
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .WillOnce(Return(mock_connection_));
    EXPECT_CALL(*mock_connection_, connect(multiaddr));
    EXPECT_CALL(*mock_connection_, isConnected())
        .WillRepeatedly(Return(false)); // Never connects
    EXPECT_CALL(*mock_event_listener_, onConnectionFailed(peer_id, "Connection timeout"));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto start_time = std::chrono::steady_clock::now();
    auto future = p2p_network->connectToPeer(multiaddr);
    auto result = future.get();
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::ConnectionTimeout);
    EXPECT_FALSE(p2p_network->isConnectedToPeer(peer_id));
    EXPECT_GE(elapsed, std::chrono::milliseconds(100));
    EXPECT_LT(elapsed, std::chrono::milliseconds(200));
}

/**
 * Test: Multiple concurrent connections
 * Verifies that multiple P2P connections can be established and maintained simultaneously
 */
TEST_F(P2PConnectionLifecycleTest, HandlesConcurrentConnections) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {
        "12D3KooWPeer1", "12D3KooWPeer2", "12D3KooWPeer3"
    };
    
    // Set up mock connections for each peer
    std::vector<std::shared_ptr<MockP2PConnection>> mock_connections;
    for (const auto& peer_id : peer_ids) {
        auto connection = std::make_shared<MockP2PConnection>();
        mock_connections.push_back(connection);
        
        EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
            .WillOnce(Return(connection));
        EXPECT_CALL(*connection, connect(_));
        EXPECT_CALL(*connection, isConnected()).WillRepeatedly(Return(true));
        EXPECT_CALL(*connection, getPeerId()).WillRepeatedly(Return(peer_id));
        EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, _));
    }
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    std::vector<std::future<ErrorCode>> futures;
    for (const auto& peer_id : peer_ids) {
        std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
        futures.push_back(p2p_network->connectToPeer(multiaddr));
    }
    
    // Wait for all connections
    for (auto& future : futures) {
        auto result = future.get();
        EXPECT_EQ(result, ErrorCode::Success);
    }
    
    // ASSERT
    EXPECT_EQ(p2p_network->getConnectionCount(), 3);
    for (const auto& peer_id : peer_ids) {
        EXPECT_TRUE(p2p_network->isConnectedToPeer(peer_id));
    }
}

/**
 * Test: Connection limit enforcement
 * Verifies that the maximum connection limit is enforced
 */
TEST_F(P2PConnectionLifecycleTest, EnforcesConnectionLimit) {
    // ARRANGE
    config_.max_connections = 2; // Limit to 2 connections
    
    const std::vector<std::string> peer_ids = {
        "12D3KooWPeer1", "12D3KooWPeer2", "12D3KooWPeer3" // 3 peers, but limit is 2
    };
    
    // First two connections should succeed
    for (int i = 0; i < 2; i++) {
        auto connection = std::make_shared<MockP2PConnection>();
        EXPECT_CALL(*mock_connection_manager_, createConnection(peer_ids[i]))
            .WillOnce(Return(connection));
        EXPECT_CALL(*connection, connect(_));
        EXPECT_CALL(*connection, isConnected()).WillRepeatedly(Return(true));
        EXPECT_CALL(*connection, getPeerId()).WillRepeatedly(Return(peer_ids[i]));
        EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_ids[i], _));
    }
    
    // Third connection should fail due to limit
    EXPECT_CALL(*mock_event_listener_, onConnectionFailed(peer_ids[2], "Connection limit reached"));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    std::vector<std::future<ErrorCode>> futures;
    for (const auto& peer_id : peer_ids) {
        std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
        futures.push_back(p2p_network->connectToPeer(multiaddr));
    }
    
    auto results = std::vector<ErrorCode>();
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    // ASSERT
    EXPECT_EQ(results[0], ErrorCode::Success);
    EXPECT_EQ(results[1], ErrorCode::Success);
    EXPECT_EQ(results[2], ErrorCode::ConnectionFailed); // Should fail due to limit
    EXPECT_EQ(p2p_network->getConnectionCount(), 2);
}

/**
 * Test: Graceful connection cleanup
 * Verifies that connections are properly cleaned up when disconnected
 */
TEST_F(P2PConnectionLifecycleTest, PerformsGracefulConnectionCleanup) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCleanup";
    const std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .WillOnce(Return(mock_connection_));
    EXPECT_CALL(*mock_connection_, connect(multiaddr));
    EXPECT_CALL(*mock_connection_, isConnected())
        .WillOnce(Return(true))  // Connected initially
        .WillOnce(Return(false)); // Disconnected after cleanup
    EXPECT_CALL(*mock_connection_, getPeerId()).WillRepeatedly(Return(peer_id));
    EXPECT_CALL(*mock_connection_, disconnect());
    EXPECT_CALL(*mock_connection_manager_, closeConnection(peer_id));
    EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, multiaddr));
    EXPECT_CALL(*mock_event_listener_, onConnectionClosed(peer_id, "Client requested disconnect"));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto connect_future = p2p_network->connectToPeer(multiaddr);
    EXPECT_EQ(connect_future.get(), ErrorCode::Success);
    
    p2p_network->disconnectFromPeer(peer_id);
    
    // ASSERT
    EXPECT_FALSE(p2p_network->isConnectedToPeer(peer_id));
    EXPECT_EQ(p2p_network->getConnectionCount(), 0);
}

/**
 * Test: Connection recovery after temporary failure
 * Verifies that connections can recover from temporary network issues
 */
TEST_F(P2PConnectionLifecycleTest, RecoversFromTemporaryFailure) {
    // ARRANGE
    const std::string peer_id = "12D3KooWRecovery";
    const std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .Times(2) // First attempt and retry
        .WillRepeatedly(Return(mock_connection_));
    
    // First connection attempt fails
    EXPECT_CALL(*mock_connection_, connect(multiaddr)).Times(2);
    EXPECT_CALL(*mock_connection_, isConnected())
        .WillOnce(Return(false))  // First attempt fails
        .WillOnce(Return(true));  // Retry succeeds
    EXPECT_CALL(*mock_connection_, getPeerId()).WillRepeatedly(Return(peer_id));
    
    EXPECT_CALL(*mock_event_listener_, onConnectionFailed(peer_id, _));
    EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, multiaddr));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto future1 = p2p_network->connectToPeer(multiaddr);
    EXPECT_EQ(future1.get(), ErrorCode::ConnectionFailed);
    
    // Retry connection
    auto future2 = p2p_network->connectToPeer(multiaddr);
    EXPECT_EQ(future2.get(), ErrorCode::Success);
    
    // ASSERT
    EXPECT_TRUE(p2p_network->isConnectedToPeer(peer_id));
    EXPECT_EQ(p2p_network->getConnectionCount(), 1);
}

/**
 * Test: Connection state callbacks
 * Verifies that connection state changes properly trigger callbacks
 */
TEST_F(P2PConnectionLifecycleTest, TriggersConnectionStateCallbacks) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCallbacks";
    const std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
    
    std::atomic<bool> established_called{false};
    std::atomic<bool> closed_called{false};
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .WillOnce(Return(mock_connection_));
    EXPECT_CALL(*mock_connection_, connect(multiaddr));
    EXPECT_CALL(*mock_connection_, isConnected())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_connection_, getPeerId()).WillRepeatedly(Return(peer_id));
    EXPECT_CALL(*mock_connection_, disconnect());
    EXPECT_CALL(*mock_connection_manager_, closeConnection(peer_id));
    
    EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, multiaddr))
        .WillOnce([&established_called](const std::string&, const std::string&) {
            established_called = true;
        });
    EXPECT_CALL(*mock_event_listener_, onConnectionClosed(peer_id, _))
        .WillOnce([&closed_called](const std::string&, const std::string&) {
            closed_called = true;
        });
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto connect_future = p2p_network->connectToPeer(multiaddr);
    EXPECT_EQ(connect_future.get(), ErrorCode::Success);
    
    p2p_network->disconnectFromPeer(peer_id);
    
    // Give callbacks time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // ASSERT
    EXPECT_TRUE(established_called.load());
    EXPECT_TRUE(closed_called.load());
}

/**
 * Test: Data transmission over established connection
 * Verifies that data can be sent and received over P2P connections
 */
TEST_F(P2PConnectionLifecycleTest, TransmitsDataOverConnection) {
    // ARRANGE
    const std::string peer_id = "12D3KooWData";
    const std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
    const std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    std::function<void(const std::vector<uint8_t>&)> data_callback;
    
    EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
        .WillOnce(Return(mock_connection_));
    EXPECT_CALL(*mock_connection_, connect(multiaddr));
    EXPECT_CALL(*mock_connection_, isConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_connection_, getPeerId()).WillRepeatedly(Return(peer_id));
    EXPECT_CALL(*mock_connection_, send(test_data));
    EXPECT_CALL(*mock_connection_, setOnDataCallback(_))
        .WillOnce([&data_callback](std::function<void(const std::vector<uint8_t>&)> callback) {
            data_callback = callback;
        });
    EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, multiaddr));
    EXPECT_CALL(*mock_event_listener_, onDataReceived(peer_id, test_data));
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // ACT
    auto connect_future = p2p_network->connectToPeer(multiaddr);
    EXPECT_EQ(connect_future.get(), ErrorCode::Success);
    
    // Send data
    auto send_result = p2p_network->sendData(peer_id, test_data);
    EXPECT_EQ(send_result, ErrorCode::Success);
    
    // Simulate receiving data
    ASSERT_NE(data_callback, nullptr);
    data_callback(test_data);
    
    // ASSERT - Mock expectations verify the data flow
}

/**
 * Test: Connection cleanup on network shutdown
 * Verifies that all connections are properly cleaned up when network shuts down
 */
TEST_F(P2PConnectionLifecycleTest, CleansUpAllConnectionsOnShutdown) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {"12D3KooWPeer1", "12D3KooWPeer2"};
    
    for (const auto& peer_id : peer_ids) {
        auto connection = std::make_shared<MockP2PConnection>();
        EXPECT_CALL(*mock_connection_manager_, createConnection(peer_id))
            .WillOnce(Return(connection));
        EXPECT_CALL(*connection, connect(_));
        EXPECT_CALL(*connection, isConnected()).WillRepeatedly(Return(true));
        EXPECT_CALL(*connection, getPeerId()).WillRepeatedly(Return(peer_id));
        EXPECT_CALL(*connection, disconnect());
        EXPECT_CALL(*mock_event_listener_, onConnectionEstablished(peer_id, _));
        EXPECT_CALL(*mock_event_listener_, onConnectionClosed(peer_id, "Network shutdown"));
    }
    
    EXPECT_CALL(*mock_connection_manager_, closeAllConnections());
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_connection_manager_);
    p2p_network->setEventListener(mock_event_listener_);
    
    // Establish connections
    for (const auto& peer_id : peer_ids) {
        std::string multiaddr = "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id;
        auto future = p2p_network->connectToPeer(multiaddr);
        EXPECT_EQ(future.get(), ErrorCode::Success);
    }
    
    EXPECT_EQ(p2p_network->getConnectionCount(), 2);
    
    // ACT
    p2p_network->shutdown();
    
    // ASSERT
    EXPECT_EQ(p2p_network->getConnectionCount(), 0);
}