// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <future>

// Headers for the P2P network implementation (will be created)
#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/common/error_codes.h"
#include "mock_libp2p.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;
using namespace Core::Multiplayer::ModelA::Test;

namespace {

/**
 * Test fixture for core P2P network functionality tests
 * Tests the essential P2P networking capabilities using cpp-libp2p
 */
class P2PNetworkTest : public Test {
protected:
    void SetUp() override {
        // Create mock dependencies
        mock_host_ = std::make_shared<MockLibp2pHost>();
        mock_transport_manager_ = std::make_shared<MockTransportManager>();
        mock_security_manager_ = std::make_shared<MockSecurityManager>();
        mock_autonat_service_ = std::make_shared<MockAutoNATService>();
        mock_circuit_relay_ = std::make_shared<MockCircuitRelay>();
        mock_performance_monitor_ = std::make_shared<MockPerformanceMonitor>();

        // Set up default configuration
        config_.enable_tcp = true;
        config_.enable_quic = true;
        config_.tcp_port = 4001;
        config_.quic_port = 4001;
        config_.max_connections = 100;
        config_.connection_timeout_ms = 5000;
        config_.enable_autonat = true;
        config_.enable_relay = true;

        // Add default relay servers
        config_.relay_servers = {
            "/ip4/147.75.77.187/tcp/4001/p2p/QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa",
            "/ip4/147.75.195.153/tcp/4001/p2p/QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN"
        };
    }

    // Mock dependencies
    std::shared_ptr<MockLibp2pHost> mock_host_;
    std::shared_ptr<MockTransportManager> mock_transport_manager_;
    std::shared_ptr<MockSecurityManager> mock_security_manager_;
    std::shared_ptr<MockAutoNATService> mock_autonat_service_;
    std::shared_ptr<MockCircuitRelay> mock_circuit_relay_;
    std::shared_ptr<MockPerformanceMonitor> mock_performance_monitor_;

    // Test configuration
    P2PNetworkConfig config_;

    // Test constants
    static constexpr const char* TEST_PEER_ID = "12D3KooWTestPeer123456789ABC";
    static constexpr const char* TEST_MULTIADDR = "/ip4/192.168.1.100/tcp/4001/p2p/12D3KooWTestPeer123456789ABC";
    static constexpr const char* TEST_PROTOCOL = "/sudachi/game/1.0.0";
};

// MARK: - Initialization and Configuration Tests

/**
 * Test: P2P network initializes successfully with valid configuration
 * Verifies that the P2P network can be created and initialized properly
 */
TEST_F(P2PNetworkTest, InitializesSuccessfullyWithValidConfiguration) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId())
        .WillOnce(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_transport_manager_, addTransport("tcp", 4001));
    EXPECT_CALL(*mock_transport_manager_, addTransport("quic", 4001));
    EXPECT_CALL(*mock_security_manager_, initializeNoise(_));
    EXPECT_CALL(*mock_autonat_service_, detectNATType());

    // ACT
    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ASSERT
    EXPECT_NE(p2p_network, nullptr);
    EXPECT_EQ(p2p_network->GetPeerId(), TEST_PEER_ID);
    EXPECT_FALSE(p2p_network->IsStarted());
    EXPECT_EQ(p2p_network->GetConnectionCount(), 0);
}

/**
 * Test: P2P network initialization fails with invalid configuration
 * Verifies proper error handling for invalid configuration parameters
 */
TEST_F(P2PNetworkTest, InitializationFailsWithInvalidConfiguration) {
    // ARRANGE
    P2PNetworkConfig invalid_config;
    // No transports enabled
    invalid_config.enable_tcp = false;
    invalid_config.enable_quic = false;
    invalid_config.enable_websocket = false;

    // ACT & ASSERT
    EXPECT_THROW({
        auto p2p_network = std::make_unique<P2PNetwork>(
            invalid_config, mock_host_, mock_transport_manager_, mock_security_manager_,
            mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
        );
    }, std::invalid_argument);
}

/**
 * Test: P2P network starts and stops successfully
 * Verifies the basic lifecycle management of the P2P network
 */
TEST_F(P2PNetworkTest, StartsAndStopsSuccessfully) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, start());
    EXPECT_CALL(*mock_host_, isStarted()).WillOnce(Return(true));
    EXPECT_CALL(*mock_host_, stop());
    EXPECT_CALL(*mock_host_, isStarted()).WillOnce(Return(false));
    
    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto start_result = p2p_network->Start();
    auto stop_result = p2p_network->Stop();

    // ASSERT
    EXPECT_EQ(start_result, MultiplayerResult::Success);
    EXPECT_EQ(stop_result, MultiplayerResult::Success);
}

// MARK: - Peer Connection Management Tests

/**
 * Test: Successfully establishes P2P connection to peer
 * Verifies that direct P2P connections can be established
 */
TEST_F(P2PNetworkTest, EstablishesP2PConnectionSuccessfully) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, connect(TEST_MULTIADDR));
    EXPECT_CALL(*mock_host_, isConnected(TEST_PEER_ID)).WillOnce(Return(true));
    EXPECT_CALL(*mock_performance_monitor_, recordConnectionEstablished(TEST_PEER_ID, _));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool connection_established = false;
    p2p_network->SetOnPeerConnectedCallback([&](const std::string& peer_id) {
        connection_established = true;
        EXPECT_EQ(peer_id, TEST_PEER_ID);
    });

    // ACT
    auto result = p2p_network->ConnectToPeer(TEST_PEER_ID, TEST_MULTIADDR);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
    EXPECT_TRUE(p2p_network->IsConnectedToPeer(TEST_PEER_ID));
}

/**
 * Test: Connection fails for unreachable peer
 * Verifies proper error handling when peer cannot be reached
 */
TEST_F(P2PNetworkTest, ConnectionFailsForUnreachablePeer) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, connect(TEST_MULTIADDR))
        .WillOnce(Throw(std::runtime_error("Connection timeout")));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool connection_failed = false;
    p2p_network->SetOnConnectionFailedCallback([&](const std::string& peer_id, const std::string& error) {
        connection_failed = true;
        EXPECT_EQ(peer_id, TEST_PEER_ID);
        EXPECT_THAT(error, HasSubstr("timeout"));
    });

    // ACT
    auto result = p2p_network->ConnectToPeer(TEST_PEER_ID, TEST_MULTIADDR);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::NetworkError);
    EXPECT_FALSE(p2p_network->IsConnectedToPeer(TEST_PEER_ID));
    EXPECT_TRUE(connection_failed);
}

/**
 * Test: Successfully disconnects from peer
 * Verifies that peer connections can be cleanly terminated
 */
TEST_F(P2PNetworkTest, DisconnectsFromPeerSuccessfully) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, isConnected(TEST_PEER_ID)).WillOnce(Return(true));
    EXPECT_CALL(*mock_host_, disconnect(TEST_PEER_ID));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool peer_disconnected = false;
    p2p_network->SetOnPeerDisconnectedCallback([&](const std::string& peer_id) {
        peer_disconnected = true;
        EXPECT_EQ(peer_id, TEST_PEER_ID);
    });

    // ACT
    auto result = p2p_network->DisconnectFromPeer(TEST_PEER_ID);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
    EXPECT_TRUE(peer_disconnected);
}

/**
 * Test: Manages multiple concurrent connections
 * Verifies that the P2P network can handle multiple simultaneous peer connections
 */
TEST_F(P2PNetworkTest, ManagesMultipleConcurrentConnections) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {
        "12D3KooWPeer1", "12D3KooWPeer2", "12D3KooWPeer3"
    };
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, getConnectionCount()).WillOnce(Return(peer_ids.size()));
    EXPECT_CALL(*mock_host_, getConnectedPeers()).WillOnce(Return(peer_ids));

    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_host_, connect(_));
        EXPECT_CALL(*mock_host_, isConnected(peer_id)).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_performance_monitor_, recordConnectionEstablished(peer_id, _));
    }

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    for (const auto& peer_id : peer_ids) {
        auto result = p2p_network->ConnectToPeer(peer_id, "/ip4/192.168.1.100/tcp/4001/p2p/" + peer_id);
        EXPECT_EQ(result, MultiplayerResult::Success);
    }

    // ASSERT
    EXPECT_EQ(p2p_network->GetConnectionCount(), peer_ids.size());
    auto connected_peers = p2p_network->GetConnectedPeers();
    EXPECT_EQ(connected_peers.size(), peer_ids.size());
    for (const auto& peer_id : peer_ids) {
        EXPECT_THAT(connected_peers, Contains(peer_id));
        EXPECT_TRUE(p2p_network->IsConnectedToPeer(peer_id));
    }
}

// MARK: - Message Sending and Receiving Tests

/**
 * Test: Successfully sends message to connected peer
 * Verifies that messages can be sent over established P2P connections
 */
TEST_F(P2PNetworkTest, SendsMessageToConnectedPeerSuccessfully) {
    // ARRANGE
    const std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, isConnected(TEST_PEER_ID)).WillOnce(Return(true));
    EXPECT_CALL(*mock_host_, sendMessage(TEST_PEER_ID, TEST_PROTOCOL, test_data))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_performance_monitor_, recordMessageSent(TEST_PEER_ID, test_data.size()));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto result = p2p_network->SendMessage(TEST_PEER_ID, TEST_PROTOCOL, test_data);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
}

/**
 * Test: Message sending fails for disconnected peer
 * Verifies proper error handling when trying to send to disconnected peer
 */
TEST_F(P2PNetworkTest, MessageSendingFailsForDisconnectedPeer) {
    // ARRANGE
    const std::vector<uint8_t> test_data = {0x01, 0x02, 0x03};
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, isConnected(TEST_PEER_ID)).WillOnce(Return(false));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto result = p2p_network->SendMessage(TEST_PEER_ID, TEST_PROTOCOL, test_data);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::NotConnected);
}

/**
 * Test: Successfully receives message from peer
 * Verifies that incoming messages are properly handled and callbacks triggered
 */
TEST_F(P2PNetworkTest, ReceivesMessageFromPeerSuccessfully) {
    // ARRANGE
    const std::vector<uint8_t> test_data = {0x06, 0x07, 0x08, 0x09};
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, setProtocolHandler(TEST_PROTOCOL, _));
    EXPECT_CALL(*mock_performance_monitor_, recordMessageReceived(TEST_PEER_ID, test_data.size()));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool message_received = false;
    p2p_network->SetOnMessageReceivedCallback(
        [&](const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
            message_received = true;
            EXPECT_EQ(peer_id, TEST_PEER_ID);
            EXPECT_EQ(protocol, TEST_PROTOCOL);
            EXPECT_EQ(data, test_data);
        }
    );

    // ACT
    p2p_network->RegisterProtocolHandler(TEST_PROTOCOL);
    // Simulate receiving a message (this would normally come from libp2p)
    p2p_network->HandleIncomingMessage(TEST_PEER_ID, TEST_PROTOCOL, test_data);

    // ASSERT
    EXPECT_TRUE(message_received);
}

/**
 * Test: Successfully broadcasts message to all connected peers
 * Verifies that messages can be sent to multiple peers simultaneously
 */
TEST_F(P2PNetworkTest, BroadcastsMessageToAllPeersSuccessfully) {
    // ARRANGE
    const std::vector<uint8_t> test_data = {0x0A, 0x0B, 0x0C};
    const std::vector<std::string> connected_peers = {"12D3KooWPeer1", "12D3KooWPeer2"};
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, getConnectedPeers()).WillOnce(Return(connected_peers));
    EXPECT_CALL(*mock_host_, broadcast(TEST_PROTOCOL, test_data));
    
    for (const auto& peer_id : connected_peers) {
        EXPECT_CALL(*mock_performance_monitor_, recordMessageSent(peer_id, test_data.size()));
    }

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto result = p2p_network->BroadcastMessage(TEST_PROTOCOL, test_data);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
}

// MARK: - NAT Traversal Detection Tests

/**
 * Test: Successfully detects NAT type
 * Verifies that NAT detection works correctly and returns appropriate results
 */
TEST_F(P2PNetworkTest, DetectsNATTypeSuccessfully) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_autonat_service_, detectNATType());
    EXPECT_CALL(*mock_autonat_service_, getNATType())
        .WillOnce(Return(MockAutoNATService::NATType::FULL_CONE));
    EXPECT_CALL(*mock_autonat_service_, isReachable()).WillOnce(Return(true));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool nat_detected = false;
    p2p_network->SetOnNATDetectedCallback([&](P2PNetwork::NATType nat_type, bool reachable) {
        nat_detected = true;
        EXPECT_EQ(nat_type, P2PNetwork::NATType::FullCone);
        EXPECT_TRUE(reachable);
    });

    // ACT
    auto result = p2p_network->DetectNATType();

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
    EXPECT_TRUE(nat_detected);
}

/**
 * Test: Determines correct traversal strategy based on NAT types
 * Verifies that appropriate traversal strategies are selected for different NAT combinations
 */
TEST_F(P2PNetworkTest, DeterminesCorrectTraversalStrategy) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_autonat_service_, canTraverseNAT(
        MockAutoNATService::NATType::FULL_CONE, 
        MockAutoNATService::NATType::RESTRICTED_CONE
    )).WillOnce(Return(true));
    EXPECT_CALL(*mock_autonat_service_, getTraversalStrategies(_, _))
        .WillOnce(Return(std::vector<std::string>{"hole_punch", "relay"}));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto can_traverse = p2p_network->CanTraverseNAT(
        P2PNetwork::NATType::FullCone, 
        P2PNetwork::NATType::RestrictedCone
    );
    auto strategies = p2p_network->GetTraversalStrategies(
        P2PNetwork::NATType::FullCone, 
        P2PNetwork::NATType::RestrictedCone
    );

    // ASSERT
    EXPECT_TRUE(can_traverse);
    EXPECT_EQ(strategies.size(), 2);
    EXPECT_THAT(strategies, Contains("hole_punch"));
    EXPECT_THAT(strategies, Contains("relay"));
}

// MARK: - Relay Fallback Mechanism Tests

/**
 * Test: Successfully falls back to relay when direct connection fails
 * Verifies that relay connections are used when P2P connection attempts fail
 */
TEST_F(P2PNetworkTest, FallsBackToRelayWhenDirectConnectionFails) {
    // ARRANGE
    const std::string relay_addr = "/ip4/147.75.77.187/tcp/4001/p2p/QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa";
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, connect(TEST_MULTIADDR))
        .WillOnce(Throw(std::runtime_error("Direct connection failed")));
    EXPECT_CALL(*mock_circuit_relay_, selectBestRelay(TEST_PEER_ID))
        .WillOnce(Return(relay_addr));
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(TEST_PEER_ID, relay_addr))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_circuit_relay_, isConnectedViaRelay(TEST_PEER_ID))
        .WillOnce(Return(true));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool relay_connected = false;
    p2p_network->SetOnRelayConnectedCallback([&](const std::string& peer_id, const std::string& relay) {
        relay_connected = true;
        EXPECT_EQ(peer_id, TEST_PEER_ID);
        EXPECT_EQ(relay, relay_addr);
    });

    // ACT
    auto result = p2p_network->ConnectToPeer(TEST_PEER_ID, TEST_MULTIADDR);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
    EXPECT_TRUE(p2p_network->IsConnectedViaRelay(TEST_PEER_ID));
    EXPECT_TRUE(relay_connected);
}

/**
 * Test: Configures multiple relay servers correctly
 * Verifies that multiple relay servers can be configured and managed
 */
TEST_F(P2PNetworkTest, ConfiguresMultipleRelayServersCorrectly) {
    // ARRANGE
    const std::vector<std::string> relay_servers = {
        "/ip4/147.75.77.187/tcp/4001/p2p/QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa",
        "/ip4/147.75.195.153/tcp/4001/p2p/QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN"
    };
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    for (const auto& relay : relay_servers) {
        EXPECT_CALL(*mock_circuit_relay_, addRelayServer(relay));
    }
    EXPECT_CALL(*mock_circuit_relay_, getRelayServers()).WillOnce(Return(relay_servers));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto configured_relays = p2p_network->GetConfiguredRelayServers();

    // ASSERT
    EXPECT_EQ(configured_relays.size(), relay_servers.size());
    for (const auto& relay : relay_servers) {
        EXPECT_THAT(configured_relays, Contains(relay));
    }
}

/**
 * Test: Relay connection fails gracefully
 * Verifies proper error handling when relay connections also fail
 */
TEST_F(P2PNetworkTest, RelayConnectionFailsGracefully) {
    // ARRANGE
    const std::string relay_addr = "/ip4/147.75.77.187/tcp/4001/p2p/QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa";
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, connect(TEST_MULTIADDR))
        .WillOnce(Throw(std::runtime_error("Direct connection failed")));
    EXPECT_CALL(*mock_circuit_relay_, selectBestRelay(TEST_PEER_ID))
        .WillOnce(Return(relay_addr));
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(TEST_PEER_ID, relay_addr))
        .WillOnce(Return(false));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    bool relay_failed = false;
    p2p_network->SetOnRelayFailedCallback([&](const std::string& peer_id, const std::string& error) {
        relay_failed = true;
        EXPECT_EQ(peer_id, TEST_PEER_ID);
        EXPECT_THAT(error, HasSubstr("Relay connection failed"));
    });

    // ACT
    auto result = p2p_network->ConnectToPeer(TEST_PEER_ID, TEST_MULTIADDR);

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::NetworkError);
    EXPECT_FALSE(p2p_network->IsConnectedToPeer(TEST_PEER_ID));
    EXPECT_TRUE(relay_failed);
}

// MARK: - Edge Cases and Error Handling Tests

/**
 * Test: Handles connection limit enforcement
 * Verifies that connection limits are properly enforced
 */
TEST_F(P2PNetworkTest, EnforcesConnectionLimits) {
    // ARRANGE
    config_.max_connections = 2;
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, getConnectionCount()).WillOnce(Return(2));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto result = p2p_network->ConnectToPeer("12D3KooWNewPeer", "/ip4/192.168.1.200/tcp/4001/p2p/12D3KooWNewPeer");

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::ConnectionLimitReached);
}

/**
 * Test: Handles network shutdown gracefully
 * Verifies that all resources are properly cleaned up during shutdown
 */
TEST_F(P2PNetworkTest, HandlesNetworkShutdownGracefully) {
    // ARRANGE
    const std::vector<std::string> connected_peers = {"12D3KooWPeer1", "12D3KooWPeer2"};
    
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, getConnectedPeers()).WillOnce(Return(connected_peers));
    EXPECT_CALL(*mock_host_, stop());
    
    for (const auto& peer_id : connected_peers) {
        EXPECT_CALL(*mock_host_, disconnect(peer_id));
    }

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    auto result = p2p_network->Shutdown();

    // ASSERT
    EXPECT_EQ(result, MultiplayerResult::Success);
}

/**
 * Test: Handles concurrent operations safely
 * Verifies that the P2P network can handle multiple simultaneous operations
 */
TEST_F(P2PNetworkTest, HandlesConcurrentOperationsSafely) {
    // ARRANGE
    EXPECT_CALL(*mock_host_, getId()).WillRepeatedly(Return(TEST_PEER_ID));
    EXPECT_CALL(*mock_host_, connect(_)).Times(AtLeast(1));
    EXPECT_CALL(*mock_host_, sendMessage(_, _, _)).Times(AtLeast(1)).WillRepeatedly(Return(true));

    auto p2p_network = std::make_unique<P2PNetwork>(
        config_, mock_host_, mock_transport_manager_, mock_security_manager_,
        mock_autonat_service_, mock_circuit_relay_, mock_performance_monitor_
    );

    // ACT
    std::vector<std::future<MultiplayerResult>> futures;
    
    // Start multiple concurrent operations
    futures.push_back(std::async(std::launch::async, [&]() {
        return p2p_network->ConnectToPeer("12D3KooWPeer1", "/ip4/192.168.1.100/tcp/4001/p2p/12D3KooWPeer1");
    }));
    
    futures.push_back(std::async(std::launch::async, [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return p2p_network->SendMessage("12D3KooWPeer1", TEST_PROTOCOL, {0x01, 0x02});
    }));

    // Wait for all operations to complete
    std::vector<MultiplayerResult> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }

    // ASSERT
    // At least one operation should succeed (the connection)
    EXPECT_THAT(results, Contains(MultiplayerResult::Success));
}

} // anonymous namespace
