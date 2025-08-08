// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "../relay_client.h"
#include "mock_relay_connection.h"
#include "../../common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;
using namespace Core::Multiplayer::ModelA::Test;

namespace {

class RelayClientTest : public Test {
protected:
    void SetUp() override {
        auto connection = std::make_unique<StrictMock<MockRelayConnection>>();
        mock_connection = connection.get();
        auto p2p = std::make_unique<StrictMock<MockP2PConnection>>();
        mock_p2p = p2p.get();
        auto limiter = std::make_unique<StrictMock<MockBandwidthLimiter>>();
        mock_bandwidth_limiter = limiter.get();
        auto selector = std::make_unique<StrictMock<MockRelayServerSelector>>();
        mock_server_selector = selector.get();

        relay_client = std::make_unique<RelayClient>(
            std::move(connection), std::move(p2p),
            std::move(limiter), std::move(selector));
    }

    void TearDown() override {
        relay_client.reset();
    }

    StrictMock<MockRelayConnection>* mock_connection{};
    StrictMock<MockP2PConnection>* mock_p2p{};
    StrictMock<MockBandwidthLimiter>* mock_bandwidth_limiter{};
    StrictMock<MockRelayServerSelector>* mock_server_selector{};
    std::unique_ptr<RelayClient> relay_client;

    static constexpr uint32_t TEST_SESSION_TOKEN = 0x12345678;
    static constexpr const char* TEST_SERVER_HOST = "relay.sudachi.org";
    static constexpr uint16_t TEST_SERVER_PORT = 8443;
    static constexpr const char* TEST_JWT_TOKEN =
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.test";
};

// Verify default construction state
TEST_F(RelayClientTest, ConstructionAndInitialization) {
    EXPECT_NE(relay_client, nullptr);
    EXPECT_FALSE(relay_client->IsConnected());
    EXPECT_EQ(relay_client->GetCurrentSession(), 0u);
    EXPECT_EQ(relay_client->GetConnectionState(),
              RelayClient::ConnectionState::Disconnected);
}

// Exercise successful connection flow
TEST_F(RelayClientTest, SuccessfulConnection) {
    EXPECT_CALL(*mock_server_selector, SelectBestServer())
        .WillOnce(Return(std::string(TEST_SERVER_HOST)));
    EXPECT_CALL(*mock_connection,
                Connect(TEST_SERVER_HOST, TEST_SERVER_PORT))
        .Times(1);
    EXPECT_CALL(*mock_connection, SetAuthToken(TEST_JWT_TOKEN)).Times(1);
    EXPECT_CALL(*mock_connection, Authenticate()).WillOnce(Return(true));

    bool connection_result = false;
    relay_client->ConnectAsync(TEST_JWT_TOKEN,
                               [&](bool success) { connection_result = success; });

    EXPECT_TRUE(connection_result);
    EXPECT_TRUE(relay_client->IsConnected());
    EXPECT_EQ(relay_client->GetConnectionState(),
              RelayClient::ConnectionState::Connected);
    EXPECT_EQ(relay_client->GetConnectionType(), "relay");
}

// Exercise connection failure path
TEST_F(RelayClientTest, ConnectionFailure) {
    EXPECT_CALL(*mock_server_selector, SelectBestServer())
        .WillOnce(Return(std::string(TEST_SERVER_HOST)));
    EXPECT_CALL(*mock_connection,
                Connect(TEST_SERVER_HOST, TEST_SERVER_PORT))
        .Times(1);
    EXPECT_CALL(*mock_connection, SetAuthToken(TEST_JWT_TOKEN)).Times(1);
    EXPECT_CALL(*mock_connection, Authenticate()).WillOnce(Return(false));

    std::string error;
    relay_client->SetErrorCallback([&](const std::string& e) { error = e; });

    bool connection_result = true;
    relay_client->ConnectAsync(TEST_JWT_TOKEN,
                               [&](bool success) { connection_result = success; });

    EXPECT_FALSE(connection_result);
    EXPECT_FALSE(relay_client->IsConnected());
    EXPECT_EQ(relay_client->GetConnectionState(),
              RelayClient::ConnectionState::Error);
    EXPECT_EQ(error, "Authentication failed");
}

// Session creation currently fails; verify error path
TEST_F(RelayClientTest, SessionCreationFails) {
    bool result = true;
    uint32_t session_id = 0;
    relay_client->CreateSessionAsync(
        TEST_SESSION_TOKEN,
        [&](bool success, uint32_t id) {
            result = success;
            session_id = id;
        });

    EXPECT_FALSE(result);
    EXPECT_EQ(session_id, 0u);
    EXPECT_EQ(relay_client->GetCurrentSession(), 0u);
}

// Session joining also fails in current implementation
TEST_F(RelayClientTest, SessionJoiningFails) {
    bool result = true;
    uint32_t session_id = 0;
    relay_client->JoinSessionAsync(
        TEST_SESSION_TOKEN,
        [&](bool success, uint32_t id) {
            result = success;
            session_id = id;
        });

    EXPECT_FALSE(result);
    EXPECT_EQ(session_id, 0u);
    EXPECT_EQ(relay_client->GetCurrentSession(), 0u);
}

} // namespace
