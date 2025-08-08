// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <future>
#include <map>

#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/model_a/nat_traversal.h"
#include "core/multiplayer/model_a/relay_client.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock AutoNAT service for testing NAT detection
 */
class MockAutoNATService {
public:
    MOCK_METHOD(void, startDetection, (), ());
    MOCK_METHOD(void, stopDetection, (), ());
    MOCK_METHOD(NATType, getNATType, (), (const));
    MOCK_METHOD(bool, isPubliclyReachable, (), (const));
    MOCK_METHOD(std::string, getPublicAddress, (), (const));
    MOCK_METHOD(std::vector<std::string>, getObservedAddresses, (), (const));
    MOCK_METHOD(void, setOnNATDetectedCallback, (std::function<void(NATType)> callback), ());
};

/**
 * Mock Circuit Relay v2 service for testing relay functionality
 */
class MockCircuitRelay {
public:
    MOCK_METHOD(bool, canRelay, (), (const));
    MOCK_METHOD(void, enableRelay, (bool enable), ());
    MOCK_METHOD(std::vector<std::string>, getRelayAddresses, (), (const));
    MOCK_METHOD(void, addRelayServer, (const std::string& multiaddr), ());
    MOCK_METHOD(void, removeRelayServer, (const std::string& multiaddr), ());
    MOCK_METHOD(std::future<ErrorCode>, connectViaRelay, (const std::string& peer_id, const std::string& relay_addr), ());
    MOCK_METHOD(void, setOnRelayConnectionCallback, (std::function<void(const std::string&, bool)> callback), ());
};

/**
 * Mock hole punching service for testing direct P2P connections
 */
class MockHolePunching {
public:
    MOCK_METHOD(std::future<ErrorCode>, attemptHolePunch, (const std::string& peer_id, const std::string& remote_addr), ());
    MOCK_METHOD(bool, isHolePunchingSupported, (NATType local_nat, NATType remote_nat), (const));
    MOCK_METHOD(void, setHolePunchTimeout, (std::chrono::milliseconds timeout), ());
    MOCK_METHOD(std::chrono::milliseconds, getHolePunchTimeout, (), (const));
    MOCK_METHOD(void, setOnHolePunchResultCallback, (std::function<void(const std::string&, bool, const std::string&)> callback), ());
};

/**
 * Mock NAT traversal coordinator
 */
class MockNATTraversalCoordinator {
public:
    MOCK_METHOD(std::future<ErrorCode>, establishConnection, (const std::string& peer_id, const P2PConnectionOptions& options), ());
    MOCK_METHOD(void, setPreferDirectConnection, (bool prefer), ());
    MOCK_METHOD(bool, preferDirectConnection, (), (const));
    MOCK_METHOD(void, setFallbackToRelay, (bool fallback), ());
    MOCK_METHOD(bool, fallbackToRelay, (), (const));
    MOCK_METHOD(NATTraversalStrategy, getLastUsedStrategy, (const std::string& peer_id), (const));
};

} // anonymous namespace

/**
 * Test fixture for NAT traversal and relay fallback tests
 * Tests the critical NAT traversal capabilities and relay fallback mechanisms
 */
class NATTraversalRelayTest : public Test {
protected:
    void SetUp() override {
        mock_autonat_ = std::make_shared<MockAutoNATService>();
        mock_circuit_relay_ = std::make_shared<MockCircuitRelay>();
        mock_hole_punching_ = std::make_shared<MockHolePunching>();
        mock_nat_coordinator_ = std::make_shared<MockNATTraversalCoordinator>();
        
        // Configure test environment
        config_.enable_nat_traversal = true;
        config_.enable_relay_fallback = true;
        config_.hole_punch_timeout_ms = 5000;
        config_.relay_servers = {
            "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1",
            "/ip4/relay2.sudachi.org/tcp/4002/p2p/12D3KooWRelay2"
        };
    }

    std::shared_ptr<MockAutoNATService> mock_autonat_;
    std::shared_ptr<MockCircuitRelay> mock_circuit_relay_;
    std::shared_ptr<MockHolePunching> mock_hole_punching_;
    std::shared_ptr<MockNATTraversalCoordinator> mock_nat_coordinator_;
    NATTraversalConfig config_;
};

/**
 * Test: AutoNAT detection identifies NAT type correctly
 * Verifies that the AutoNAT service can detect different NAT types
 */
TEST_F(NATTraversalRelayTest, AutoNATDetectsNATTypeCorrectly) {
    // ARRANGE
    EXPECT_CALL(*mock_autonat_, startDetection());
    EXPECT_CALL(*mock_autonat_, getNATType())
        .WillOnce(Return(NATType::Cone));
    EXPECT_CALL(*mock_autonat_, isPubliclyReachable())
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_autonat_, getPublicAddress())
        .WillOnce(Return("203.0.113.1"));
    EXPECT_CALL(*mock_autonat_, getObservedAddresses())
        .WillOnce(Return(std::vector<std::string>{"203.0.113.1"}));
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_);
    
    // ACT
    nat_traversal->startNATDetection();
    auto nat_info = nat_traversal->getNATInfo();
    
    // ASSERT
    EXPECT_EQ(nat_info.type, NATType::Cone);
    EXPECT_FALSE(nat_info.is_publicly_reachable);
    EXPECT_EQ(nat_info.public_address, "203.0.113.1");
    EXPECT_EQ(nat_info.observed_addresses.size(), 1);
}

/**
 * Test: Different NAT type combinations and traversal strategies
 * Verifies correct strategy selection based on NAT type combinations
 */
TEST_F(NATTraversalRelayTest, SelectsCorrectTraversalStrategy) {
    // Test case 1: Both peers behind cone NAT - should attempt hole punching
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Cone, NATType::Cone))
        .WillOnce(Return(true));
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_);
    
    auto strategy = nat_traversal->selectTraversalStrategy(NATType::Cone, NATType::Cone);
    EXPECT_EQ(strategy, NATTraversalStrategy::HolePunching);
    
    // Test case 2: One peer behind symmetric NAT - should use relay
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Symmetric, NATType::Cone))
        .WillOnce(Return(false));
    
    strategy = nat_traversal->selectTraversalStrategy(NATType::Symmetric, NATType::Cone);
    EXPECT_EQ(strategy, NATTraversalStrategy::Relay);
    
    // Test case 3: Both peers publicly reachable - direct connection
    strategy = nat_traversal->selectTraversalStrategy(NATType::None, NATType::None);
    EXPECT_EQ(strategy, NATTraversalStrategy::Direct);
}

/**
 * Test: Successful hole punching between cone NAT peers
 * Verifies that hole punching works for compatible NAT types
 */
TEST_F(NATTraversalRelayTest, SuccessfulHolePunchingBetweenConeNATs) {
    // ARRANGE
    const std::string peer_id = "12D3KooWPeerBehindNAT";
    const std::string remote_addr = "203.0.113.2:4001";
    
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Cone, NATType::Cone))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_hole_punching_, attemptHolePunch(peer_id, remote_addr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_);
    
    // ACT
    auto future = nat_traversal->establishConnection(peer_id, remote_addr, NATType::Cone, NATType::Cone);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
}

/**
 * Test: Hole punching timeout and fallback to relay
 * Verifies that failed hole punching attempts fall back to relay connections
 */
TEST_F(NATTraversalRelayTest, HolePunchingTimeoutFallsBackToRelay) {
    // ARRANGE
    const std::string peer_id = "12D3KooWTimeoutPeer";
    const std::string remote_addr = "203.0.113.3:4001";
    const std::string relay_addr = "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1";
    
    // Hole punching attempt times out
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Cone, NATType::Cone))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_hole_punching_, attemptHolePunch(peer_id, remote_addr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::ConnectionTimeout);
            return promise.get_future();
        });
    
    // Fallback to relay succeeds
    EXPECT_CALL(*mock_circuit_relay_, getRelayAddresses())
        .WillOnce(Return(std::vector<std::string>{relay_addr}));
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(peer_id, relay_addr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_, mock_circuit_relay_);
    
    // ACT
    auto future = nat_traversal->establishConnectionWithFallback(peer_id, remote_addr, NATType::Cone, NATType::Cone);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(nat_traversal->getLastUsedStrategy(peer_id), NATTraversalStrategy::Relay);
}

/**
 * Test: Symmetric NAT detection triggers immediate relay usage
 * Verifies that symmetric NAT peers immediately use relay connections
 */
TEST_F(NATTraversalRelayTest, SymmetricNATUsesRelayImmediately) {
    // ARRANGE
    const std::string peer_id = "12D3KooWSymmetricPeer";
    const std::string relay_addr = "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1";
    
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Symmetric, NATType::Cone))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_circuit_relay_, getRelayAddresses())
        .WillOnce(Return(std::vector<std::string>{relay_addr}));
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(peer_id, relay_addr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_, mock_circuit_relay_);
    
    // ACT
    auto future = nat_traversal->establishConnection(peer_id, "203.0.113.4:4001", NATType::Symmetric, NATType::Cone);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(nat_traversal->getLastUsedStrategy(peer_id), NATTraversalStrategy::Relay);
}

/**
 * Test: Relay server selection and failover
 * Verifies that relay server selection works and can failover to backup relays
 */
TEST_F(NATTraversalRelayTest, RelayServerSelectionAndFailover) {
    // ARRANGE
    const std::string peer_id = "12D3KooWRelayTest";
    const std::string relay1 = "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1";
    const std::string relay2 = "/ip4/relay2.sudachi.org/tcp/4002/p2p/12D3KooWRelay2";
    
    EXPECT_CALL(*mock_circuit_relay_, getRelayAddresses())
        .WillRepeatedly(Return(std::vector<std::string>{relay1, relay2}));
    
    // First relay fails
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(peer_id, relay1))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::ConnectionFailed);
            return promise.get_future();
        });
    
    // Second relay succeeds
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(peer_id, relay2))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto relay_client = std::make_unique<RelayClient>(config_.relay_servers, mock_circuit_relay_);
    
    // ACT
    auto future = relay_client->connectToPeerViaRelay(peer_id);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(relay_client->getCurrentRelay(peer_id), relay2);
}

/**
 * Test: Multiple relay servers configuration
 * Verifies that multiple relay servers can be configured and used
 */
TEST_F(NATTraversalRelayTest, ConfiguresMultipleRelayServers) {
    // ARRANGE
    const std::vector<std::string> relay_servers = {
        "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1",
        "/ip4/relay2.sudachi.org/tcp/4002/p2p/12D3KooWRelay2",
        "/ip4/relay3.sudachi.org/tcp/4002/p2p/12D3KooWRelay3"
    };
    
    EXPECT_CALL(*mock_circuit_relay_, addRelayServer(_))
        .Times(3); // One call for each relay server
    EXPECT_CALL(*mock_circuit_relay_, getRelayAddresses())
        .WillOnce(Return(relay_servers));
    
    // ACT
    auto relay_client = std::make_unique<RelayClient>(relay_servers, mock_circuit_relay_);
    
    // ASSERT
    auto configured_relays = relay_client->getConfiguredRelays();
    EXPECT_EQ(configured_relays.size(), 3);
    EXPECT_THAT(configured_relays, Contains(relay_servers[0]));
    EXPECT_THAT(configured_relays, Contains(relay_servers[1]));
    EXPECT_THAT(configured_relays, Contains(relay_servers[2]));
}

/**
 * Test: NAT traversal with connection preferences
 * Verifies that connection preferences (direct vs relay) are respected
 */
TEST_F(NATTraversalRelayTest, RespectsConnectionPreferences) {
    // ARRANGE
    const std::string peer_id = "12D3KooWPreferencePeer";
    P2PConnectionOptions options;
    options.prefer_direct_connection = true;
    options.max_hole_punch_attempts = 3;
    options.hole_punch_timeout_ms = 2000;
    
    EXPECT_CALL(*mock_nat_coordinator_, setPreferDirectConnection(true));
    EXPECT_CALL(*mock_nat_coordinator_, establishConnection(peer_id, options))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_, mock_circuit_relay_, mock_nat_coordinator_);
    
    // ACT
    nat_traversal->setConnectionPreferences(true, false); // Prefer direct, don't fallback to relay
    auto future = nat_traversal->establishConnectionWithOptions(peer_id, options);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
}

/**
 * Test: Relay connection bandwidth and latency monitoring
 * Verifies that relay connections are monitored for performance metrics
 */
TEST_F(NATTraversalRelayTest, MonitorsRelayConnectionPerformance) {
    // ARRANGE
    const std::string peer_id = "12D3KooWMonitorPeer";
    const std::string relay_addr = "/ip4/relay1.sudachi.org/tcp/4002/p2p/12D3KooWRelay1";
    
    std::function<void(const std::string&, bool)> relay_callback;
    
    EXPECT_CALL(*mock_circuit_relay_, setOnRelayConnectionCallback(_))
        .WillOnce([&relay_callback](std::function<void(const std::string&, bool)> callback) {
            relay_callback = callback;
        });
    EXPECT_CALL(*mock_circuit_relay_, connectViaRelay(peer_id, relay_addr))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto relay_client = std::make_unique<RelayClient>(config_.relay_servers, mock_circuit_relay_);
    
    // ACT
    auto future = relay_client->connectToPeerViaRelay(peer_id);
    auto result = future.get();
    
    // Simulate relay connection established callback
    ASSERT_NE(relay_callback, nullptr);
    relay_callback(peer_id, true);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(relay_client->isConnectedViaRelay(peer_id));
    
    // Test performance metrics
    auto metrics = relay_client->getConnectionMetrics(peer_id);
    EXPECT_GE(metrics.connection_time_ms, 0);
    EXPECT_EQ(metrics.connection_type, ConnectionType::Relay);
}

/**
 * Test: Concurrent NAT traversal attempts
 * Verifies that multiple NAT traversal attempts can be handled concurrently
 */
TEST_F(NATTraversalRelayTest, HandlesConcurrentNATTraversalAttempts) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {
        "12D3KooWConcurrent1", "12D3KooWConcurrent2", "12D3KooWConcurrent3"
    };
    
    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Cone, NATType::Cone))
            .WillOnce(Return(true));
        EXPECT_CALL(*mock_hole_punching_, attemptHolePunch(peer_id, _))
            .WillOnce([]() {
                std::promise<ErrorCode> promise;
                promise.set_value(ErrorCode::Success);
                return promise.get_future();
            });
    }
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_);
    
    // ACT
    std::vector<std::future<ErrorCode>> futures;
    for (const auto& peer_id : peer_ids) {
        futures.push_back(nat_traversal->establishConnection(peer_id, "203.0.113.100:4001", NATType::Cone, NATType::Cone));
    }
    
    // Wait for all attempts to complete
    std::vector<ErrorCode> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    // ASSERT
    for (const auto& result : results) {
        EXPECT_EQ(result, ErrorCode::Success);
    }
}

/**
 * Test: NAT traversal statistics and reporting
 * Verifies that NAT traversal attempts are properly tracked and reported
 */
TEST_F(NATTraversalRelayTest, TracksNATTraversalStatistics) {
    // ARRANGE
    const std::string peer_id = "12D3KooWStatsPeer";
    
    // Successful hole punch
    EXPECT_CALL(*mock_hole_punching_, isHolePunchingSupported(NATType::Cone, NATType::Cone))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_hole_punching_, attemptHolePunch(peer_id, _))
        .WillOnce([]() {
            std::promise<ErrorCode> promise;
            promise.set_value(ErrorCode::Success);
            return promise.get_future();
        });
    
    auto nat_traversal = std::make_unique<NATTraversal>(config_, mock_autonat_, mock_hole_punching_);
    
    // ACT
    auto future = nat_traversal->establishConnection(peer_id, "203.0.113.200:4001", NATType::Cone, NATType::Cone);
    auto result = future.get();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    
    auto stats = nat_traversal->getStatistics();
    EXPECT_EQ(stats.total_attempts, 1);
    EXPECT_EQ(stats.successful_hole_punches, 1);
    EXPECT_EQ(stats.failed_hole_punches, 0);
    EXPECT_EQ(stats.relay_connections, 0);
    EXPECT_GT(stats.average_connection_time_ms, 0);
}
