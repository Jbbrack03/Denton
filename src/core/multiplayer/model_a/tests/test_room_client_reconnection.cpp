// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#include "core/multiplayer/model_a/room_client.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock WebSocket connection for reconnection testing
 */
class MockWebSocketConnection {
public:
    MOCK_METHOD(void, Connect, (const std::string& uri), ());
    MOCK_METHOD(void, Disconnect, (const std::string& reason), ());
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(void, SendMessage, (const std::string& message), ());
    MOCK_METHOD(void, SetOnConnectCallback, (std::function<void()> callback), ());
    MOCK_METHOD(void, SetOnDisconnectCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnErrorCallback, (std::function<void(const std::string&)> callback), ());
    
    // Store callbacks for test simulation
    void SetCallbacks(std::function<void()> on_connect,
                     std::function<void(const std::string&)> on_disconnect,
                     std::function<void(const std::string&)> on_error) {
        connect_callback_ = on_connect;
        disconnect_callback_ = on_disconnect;
        error_callback_ = on_error;
    }
    
    // Simulate connection events
    void SimulateConnect() {
        if (connect_callback_) connect_callback_();
    }
    
    void SimulateDisconnect(const std::string& reason) {
        if (disconnect_callback_) disconnect_callback_(reason);
    }
    
    void SimulateError(const std::string& error) {
        if (error_callback_) error_callback_(error);
    }
    
private:
    std::function<void()> connect_callback_;
    std::function<void(const std::string&)> disconnect_callback_;
    std::function<void(const std::string&)> error_callback_;
};

/**
 * Mock configuration with reconnection settings
 */
class MockReconnectionConfig {
public:
    MOCK_METHOD(bool, IsAutoReconnectEnabled, (), (const));
    MOCK_METHOD(int, GetMaxReconnectAttempts, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetReconnectBaseDelay, (), (const));
    MOCK_METHOD(double, GetReconnectBackoffMultiplier, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetMaxReconnectDelay, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetConnectionTimeout, (), (const));
    MOCK_METHOD(bool, ShouldReconnectOnError, (const std::string& error_type), (const));
};

/**
 * Mock reconnection event listener
 */
class MockReconnectionListener {
public:
    MOCK_METHOD(void, OnReconnectionStarted, (int attempt), ());
    MOCK_METHOD(void, OnReconnectionFailed, (int attempt, const std::string& error), ());
    MOCK_METHOD(void, OnReconnectionSucceeded, (int attempt), ());
    MOCK_METHOD(void, OnReconnectionGivenUp, (int total_attempts), ());
    MOCK_METHOD(void, OnConnectionLost, (const std::string& reason), ());
};

} // anonymous namespace

/**
 * Test fixture for RoomClient reconnection logic tests
 * Tests automatic reconnection with exponential backoff
 */
class RoomClientReconnectionTest : public Test {
protected:
    void SetUp() override {
        mock_connection_ = std::make_shared<MockWebSocketConnection>();
        mock_config_ = std::make_shared<MockReconnectionConfig>();
        mock_listener_ = std::make_shared<MockReconnectionListener>();
        
        // Set up default reconnection configuration
        ON_CALL(*mock_config_, IsAutoReconnectEnabled()).WillByDefault(Return(true));
        ON_CALL(*mock_config_, GetMaxReconnectAttempts()).WillByDefault(Return(3));
        ON_CALL(*mock_config_, GetReconnectBaseDelay())
            .WillByDefault(Return(std::chrono::milliseconds(1000)));
        ON_CALL(*mock_config_, GetReconnectBackoffMultiplier()).WillByDefault(Return(2.0));
        ON_CALL(*mock_config_, GetMaxReconnectDelay())
            .WillByDefault(Return(std::chrono::milliseconds(30000)));
        ON_CALL(*mock_config_, GetConnectionTimeout())
            .WillByDefault(Return(std::chrono::milliseconds(5000)));
        ON_CALL(*mock_config_, ShouldReconnectOnError(_)).WillByDefault(Return(true));
    }

    std::shared_ptr<MockWebSocketConnection> mock_connection_;
    std::shared_ptr<MockReconnectionConfig> mock_config_;
    std::shared_ptr<MockReconnectionListener> mock_listener_;
};

/**
 * Test: Automatic reconnection triggers on connection loss
 * Verifies that reconnection starts automatically when connection is lost
 */
TEST_F(RoomClientReconnectionTest, AutoReconnectionTriggersOnConnectionLoss) {
    // This test will fail because RoomClient reconnection logic doesn't exist yet
    
    // ARRANGE
    EXPECT_CALL(*mock_listener_, OnConnectionLost("Network error"));
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(1));
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(AtLeast(1));
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // Simulate initial connection
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(true));
    client->Connect();
    
    // ACT - Simulate connection loss
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    mock_connection_->SimulateDisconnect("Network error");
    
    // Wait for reconnection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // ASSERT - Mock expectations verify reconnection started
}

/**
 * Test: Exponential backoff calculation
 * Verifies that reconnection delays follow exponential backoff pattern
 */
TEST_F(RoomClientReconnectionTest, ExponentialBackoffCalculation) {
    // This test will fail because backoff calculation doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // ACT & ASSERT - Test backoff delay calculation
    EXPECT_EQ(client->CalculateReconnectDelay(1), std::chrono::milliseconds(1000));
    EXPECT_EQ(client->CalculateReconnectDelay(2), std::chrono::milliseconds(2000));
    EXPECT_EQ(client->CalculateReconnectDelay(3), std::chrono::milliseconds(4000));
    EXPECT_EQ(client->CalculateReconnectDelay(4), std::chrono::milliseconds(8000));
    EXPECT_EQ(client->CalculateReconnectDelay(5), std::chrono::milliseconds(16000));
    
    // Test maximum delay cap
    EXPECT_EQ(client->CalculateReconnectDelay(10), std::chrono::milliseconds(30000));
}

/**
 * Test: Successful reconnection after temporary network failure
 * Verifies that client can successfully reconnect after network issues
 */
TEST_F(RoomClientReconnectionTest, SuccessfulReconnectionAfterNetworkFailure) {
    // This test will fail because reconnection success handling doesn't exist yet
    
    // ARRANGE
    InSequence seq;
    EXPECT_CALL(*mock_listener_, OnConnectionLost("Temporary network failure"));
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(1));
    EXPECT_CALL(*mock_listener_, OnReconnectionSucceeded(1));
    
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(1);
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // Simulate initial connection
    client->Connect();
    
    // ACT - Simulate temporary network failure and recovery
    ON_CALL(*mock_connection_, IsConnected())
        .WillByDefault(Return(false)); // Initially disconnected
    mock_connection_->SimulateDisconnect("Temporary network failure");
    
    // Wait for reconnection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Simulate successful reconnection
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(true));
    mock_connection_->SimulateConnect();
    
    // ASSERT
    EXPECT_TRUE(client->IsConnected());
    EXPECT_EQ(client->GetReconnectionAttempts(), 1);
}

/**
 * Test: Reconnection gives up after max attempts exceeded
 * Verifies that reconnection stops after configured maximum attempts
 */
TEST_F(RoomClientReconnectionTest, ReconnectionGivesUpAfterMaxAttempts) {
    // This test will fail because max attempts handling doesn't exist yet
    
    // ARRANGE
    ON_CALL(*mock_config_, GetMaxReconnectAttempts()).WillByDefault(Return(2));
    
    InSequence seq;
    EXPECT_CALL(*mock_listener_, OnConnectionLost(_));
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(1));
    EXPECT_CALL(*mock_listener_, OnReconnectionFailed(1, _));
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(2));
    EXPECT_CALL(*mock_listener_, OnReconnectionFailed(2, _));
    EXPECT_CALL(*mock_listener_, OnReconnectionGivenUp(2));
    
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(2);
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // ACT - Simulate connection loss and failed reconnection attempts
    mock_connection_->SimulateDisconnect("Persistent network error");
    
    // Wait for all reconnection attempts to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // ASSERT
    EXPECT_FALSE(client->IsConnected());
    EXPECT_FALSE(client->IsReconnecting());
    EXPECT_EQ(client->GetReconnectionAttempts(), 2);
}

/**
 * Test: Manual reconnection while auto-reconnection is disabled
 * Verifies that manual reconnection works when auto-reconnection is disabled
 */
TEST_F(RoomClientReconnectionTest, ManualReconnectionWhenAutoDisabled) {
    // This test will fail because manual reconnection doesn't exist yet
    
    // ARRANGE
    ON_CALL(*mock_config_, IsAutoReconnectEnabled()).WillByDefault(Return(false));
    
    // Should not trigger automatic reconnection
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(_)).Times(0);
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(1); // Only manual call
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // Simulate connection loss
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    mock_connection_->SimulateDisconnect("Connection lost");
    
    // Wait to ensure no auto-reconnection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // ACT - Manual reconnection
    auto result = client->Reconnect();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
}

/**
 * Test: Reconnection state tracking
 * Verifies that reconnection state is properly tracked and reported
 */
TEST_F(RoomClientReconnectionTest, ReconnectionStateTracking) {
    // This test will fail because reconnection state tracking doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Initially not reconnecting
    EXPECT_FALSE(client->IsReconnecting());
    EXPECT_EQ(client->GetReconnectionAttempts(), 0);
    EXPECT_EQ(client->GetConnectionState(), ConnectionState::Disconnected);
    
    // ACT - Trigger reconnection
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    mock_connection_->SimulateDisconnect("Test disconnect");
    
    // ASSERT - During reconnection
    EXPECT_TRUE(client->IsReconnecting());
    EXPECT_GT(client->GetReconnectionAttempts(), 0);
    EXPECT_EQ(client->GetConnectionState(), ConnectionState::Reconnecting);
    
    // Simulate successful reconnection
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(true));
    mock_connection_->SimulateConnect();
    
    // ASSERT - After successful reconnection
    EXPECT_FALSE(client->IsReconnecting());
    EXPECT_EQ(client->GetConnectionState(), ConnectionState::Connected);
}

/**
 * Test: Connection timeout during reconnection attempt
 * Verifies that individual reconnection attempts respect timeout settings
 */
TEST_F(RoomClientReconnectionTest, ConnectionTimeoutDuringReconnection) {
    // This test will fail because reconnection timeout handling doesn't exist yet
    
    // ARRANGE
    ON_CALL(*mock_config_, GetConnectionTimeout())
        .WillByDefault(Return(std::chrono::milliseconds(100))); // Short timeout for test
    
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(1));
    EXPECT_CALL(*mock_listener_, OnReconnectionFailed(1, HasSubstr("timeout")));
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(1);
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // ACT - Trigger reconnection that will timeout
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    mock_connection_->SimulateDisconnect("Connection lost");
    
    // Wait longer than timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // ASSERT - Should have failed due to timeout
    EXPECT_FALSE(client->IsConnected());
}

/**
 * Test: Selective reconnection based on error type
 * Verifies that reconnection only occurs for appropriate error types
 */
TEST_F(RoomClientReconnectionTest, SelectiveReconnectionBasedOnErrorType) {
    // This test will fail because selective reconnection doesn't exist yet
    
    // ARRANGE
    ON_CALL(*mock_config_, ShouldReconnectOnError("AUTH_FAILED")).WillByDefault(Return(false));
    ON_CALL(*mock_config_, ShouldReconnectOnError("NETWORK_ERROR")).WillByDefault(Return(true));
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // ACT & ASSERT - Auth failure should not trigger reconnection
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(_)).Times(0);
    mock_connection_->SimulateError("AUTH_FAILED");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Reset mock
    Mock::VerifyAndClearExpectations(mock_listener_.get());
    
    // ACT & ASSERT - Network error should trigger reconnection
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(1));
    mock_connection_->SimulateError("NETWORK_ERROR");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

/**
 * Test: Reconnection preserves session state
 * Verifies that room membership and other state is preserved across reconnections
 */
TEST_F(RoomClientReconnectionTest, ReconnectionPreservesSessionState) {
    // This test will fail because session state preservation doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Simulate being in a room before disconnection
    client->SetCurrentRoomId("room_12345");
    client->SetPlayerSlot(2);
    
    EXPECT_EQ(client->GetCurrentRoomId(), "room_12345");
    EXPECT_EQ(client->GetPlayerSlot(), 2);
    
    // ACT - Trigger reconnection
    ON_CALL(*mock_connection_, IsConnected())
        .WillByDefault(Return(false))
        .WillOnce(Return(true)); // Successful reconnection
    
    mock_connection_->SimulateDisconnect("Temporary disconnection");
    
    // Wait for reconnection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mock_connection_->SimulateConnect();
    
    // ASSERT - Session state should be preserved
    EXPECT_EQ(client->GetCurrentRoomId(), "room_12345");
    EXPECT_EQ(client->GetPlayerSlot(), 2);
    
    // Should attempt to rejoin room automatically
    EXPECT_CALL(*mock_connection_, SendMessage(HasSubstr("rejoin_room")));
}

/**
 * Test: Concurrent reconnection attempts are serialized
 * Verifies that multiple simultaneous reconnection triggers don't cause issues
 */
TEST_F(RoomClientReconnectionTest, ConcurrentReconnectionAttemptsSeralized) {
    // This test will fail because concurrent reconnection handling doesn't exist yet
    
    // ARRANGE
    EXPECT_CALL(*mock_listener_, OnReconnectionStarted(_)).Times(1); // Should only start once
    EXPECT_CALL(*mock_connection_, Connect(_)).Times(1); // Should only connect once
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    client->SetReconnectionListener(mock_listener_);
    
    // ACT - Trigger multiple concurrent reconnection attempts
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            mock_connection_->SimulateDisconnect("Concurrent test");
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for reconnection logic to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // ASSERT - Mock expectations verify single reconnection attempt
}

/**
 * Test: Reconnection statistics tracking
 * Verifies that reconnection statistics are properly tracked and reported
 */
TEST_F(RoomClientReconnectionTest, ReconnectionStatisticsTracking) {
    // This test will fail because statistics tracking doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Initial state
    auto stats = client->GetReconnectionStatistics();
    EXPECT_EQ(stats.total_attempts, 0);
    EXPECT_EQ(stats.successful_reconnections, 0);
    EXPECT_EQ(stats.failed_reconnections, 0);
    
    // ACT - Simulate some reconnection attempts
    ON_CALL(*mock_connection_, IsConnected())
        .WillByDefault(Return(false))   // First attempt fails
        .WillOnce(Return(true));        // Second attempt succeeds
    
    // First failed attempt
    mock_connection_->SimulateDisconnect("Test 1");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Second successful attempt
    mock_connection_->SimulateConnect();
    
    // ASSERT
    stats = client->GetReconnectionStatistics();
    EXPECT_EQ(stats.total_attempts, 2);
    EXPECT_EQ(stats.successful_reconnections, 1);
    EXPECT_EQ(stats.failed_reconnections, 1);
    EXPECT_GT(stats.total_downtime_ms, 0);
    EXPECT_GT(stats.average_reconnect_time_ms, 0);
}

/**
 * Test: Graceful shutdown during reconnection
 * Verifies that client can be properly shutdown even during active reconnection
 */
TEST_F(RoomClientReconnectionTest, GracefulShutdownDuringReconnection) {
    // This test will fail because graceful shutdown during reconnection doesn't exist yet
    
    // ARRANGE
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Start reconnection process
    ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(false));
    mock_connection_->SimulateDisconnect("Test disconnect");
    
    // Verify reconnection started
    EXPECT_TRUE(client->IsReconnecting());
    
    // ACT - Shutdown during reconnection
    auto shutdown_start = std::chrono::steady_clock::now();
    client->Shutdown();
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;
    
    // ASSERT
    EXPECT_FALSE(client->IsReconnecting());
    EXPECT_LT(shutdown_duration, std::chrono::milliseconds(1000)); // Should shutdown quickly
    EXPECT_EQ(client->GetConnectionState(), ConnectionState::Disconnected);
}