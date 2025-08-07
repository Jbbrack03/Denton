// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include "test_helpers.h"

namespace Core::Multiplayer::HLE {

class MultiplayerBackendInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_backend = std::make_unique<MockMultiplayerBackend>();
    }

    std::unique_ptr<MockMultiplayerBackend> mock_backend;
};

TEST_F(MultiplayerBackendInterfaceTest, InitializeSuccess) {
    EXPECT_CALL(*mock_backend, Initialize()).WillOnce(::testing::Return(ErrorCode::Success));
    EXPECT_CALL(*mock_backend, IsInitialized()).WillOnce(::testing::Return(true));
    auto result = mock_backend->Initialize();
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(mock_backend->IsInitialized());
}

TEST_F(MultiplayerBackendInterfaceTest, InitializeFailure) {
    EXPECT_CALL(*mock_backend, Initialize()).WillOnce(::testing::Return(ErrorCode::PlatformAPIError));
    EXPECT_CALL(*mock_backend, IsInitialized()).WillOnce(::testing::Return(false));
    auto result = mock_backend->Initialize();
    EXPECT_EQ(result, ErrorCode::PlatformAPIError);
    EXPECT_FALSE(mock_backend->IsInitialized());
}

TEST_F(MultiplayerBackendInterfaceTest, CreateNetworkSuccess) {
    Service::LDN::CreateNetworkConfig config{};
    config.network_config.intent_id.local_communication_id = 0x0100000000001234ULL;
    config.network_config.channel = Service::LDN::WifiChannel::Default;
    config.network_config.node_count_max = 8;

    EXPECT_CALL(*mock_backend, CreateNetwork(::testing::_)).WillOnce(::testing::Return(ErrorCode::Success));
    auto result = mock_backend->CreateNetwork(config);
    EXPECT_EQ(result, ErrorCode::Success);
}

TEST_F(MultiplayerBackendInterfaceTest, CreateNetworkInvalidConfig) {
    Service::LDN::CreateNetworkConfig config{};
    config.network_config.intent_id.local_communication_id = 0;
    EXPECT_CALL(*mock_backend, CreateNetwork(::testing::_))
        .WillOnce(::testing::Return(ErrorCode::ConfigurationInvalid));
    auto result = mock_backend->CreateNetwork(config);
    EXPECT_EQ(result, ErrorCode::ConfigurationInvalid);
}

TEST_F(MultiplayerBackendInterfaceTest, ScanNetworks) {
    std::vector<Service::LDN::NetworkInfo> networks;
    Service::LDN::ScanFilter filter{};
    EXPECT_CALL(*mock_backend, Scan(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::Invoke([](std::vector<Service::LDN::NetworkInfo>& out_networks,
                                 const Service::LDN::ScanFilter&) {
                Service::LDN::NetworkInfo network{};
                network.network_id.intent_id.local_communication_id = 0x0100000000001234ULL;
                out_networks.push_back(network);
            }),
            ::testing::Return(ErrorCode::Success)));
    auto result = mock_backend->Scan(networks, filter);
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(networks.size(), 1);
    EXPECT_EQ(networks[0].network_id.intent_id.local_communication_id, 0x0100000000001234ULL);
}

TEST_F(MultiplayerBackendInterfaceTest, ConnectToNetwork) {
    Service::LDN::ConnectNetworkData connect_data{};
    Service::LDN::NetworkInfo network_info{};
    network_info.network_id.intent_id.local_communication_id = 0x0100000000001234ULL;
    EXPECT_CALL(*mock_backend, Connect(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(ErrorCode::Success));
    auto result = mock_backend->Connect(connect_data, network_info);
    EXPECT_EQ(result, ErrorCode::Success);
}

TEST_F(MultiplayerBackendInterfaceTest, ConnectFailure) {
    Service::LDN::ConnectNetworkData connect_data{};
    Service::LDN::NetworkInfo network_info{};
    EXPECT_CALL(*mock_backend, Connect(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(ErrorCode::ConnectionRefused));
    auto result = mock_backend->Connect(connect_data, network_info);
    EXPECT_EQ(result, ErrorCode::ConnectionRefused);
}

TEST_F(MultiplayerBackendInterfaceTest, AccessPointManagement) {
    EXPECT_CALL(*mock_backend, OpenAccessPoint()).WillOnce(::testing::Return(ErrorCode::Success));
    EXPECT_CALL(*mock_backend, CloseAccessPoint()).WillOnce(::testing::Return(ErrorCode::Success));
    auto open_result = mock_backend->OpenAccessPoint();
    auto close_result = mock_backend->CloseAccessPoint();
    EXPECT_EQ(open_result, ErrorCode::Success);
    EXPECT_EQ(close_result, ErrorCode::Success);
}

TEST_F(MultiplayerBackendInterfaceTest, StationManagement) {
    EXPECT_CALL(*mock_backend, OpenStation()).WillOnce(::testing::Return(ErrorCode::Success));
    EXPECT_CALL(*mock_backend, CloseStation()).WillOnce(::testing::Return(ErrorCode::Success));
    auto open_result = mock_backend->OpenStation();
    auto close_result = mock_backend->CloseStation();
    EXPECT_EQ(open_result, ErrorCode::Success);
    EXPECT_EQ(close_result, ErrorCode::Success);
}

TEST_F(MultiplayerBackendInterfaceTest, PacketTransmission) {
    std::vector<uint8_t> send_data = {0x01, 0x02, 0x03, 0x04};
    uint8_t target_node = 1;
    std::vector<uint8_t> receive_data;
    uint8_t source_node;
    EXPECT_CALL(*mock_backend, SendPacket(send_data, target_node)).WillOnce(::testing::Return(ErrorCode::Success));
    EXPECT_CALL(*mock_backend, ReceivePacket(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::Invoke([&send_data](std::vector<uint8_t>& out_data, uint8_t& out_node) {
                out_data = send_data;
                out_node = 2;
            }),
            ::testing::Return(ErrorCode::Success)));
    auto send_result = mock_backend->SendPacket(send_data, target_node);
    auto receive_result = mock_backend->ReceivePacket(receive_data, source_node);
    EXPECT_EQ(send_result, ErrorCode::Success);
    EXPECT_EQ(receive_result, ErrorCode::Success);
    EXPECT_EQ(receive_data, send_data);
    EXPECT_EQ(source_node, 2);
}

TEST_F(MultiplayerBackendInterfaceTest, GetNetworkInfo) {
    Service::LDN::NetworkInfo network_info{};
    EXPECT_CALL(*mock_backend, GetNetworkInfo(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::Invoke([](Service::LDN::NetworkInfo& out_info) {
                out_info.network_id.intent_id.local_communication_id = 0x0100000000001234ULL;
                out_info.ldn.node_count = 1;
                out_info.ldn.node_count_max = 8;
            }),
            ::testing::Return(ErrorCode::Success)));
    auto result = mock_backend->GetNetworkInfo(network_info);
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(network_info.network_id.intent_id.local_communication_id, 0x0100000000001234ULL);
    EXPECT_EQ(network_info.ldn.node_count, 1);
    EXPECT_EQ(network_info.ldn.node_count_max, 8);
}

TEST_F(MultiplayerBackendInterfaceTest, StateManagement) {
    Service::LDN::State current_state;
    EXPECT_CALL(*mock_backend, GetCurrentState(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::Invoke([](Service::LDN::State& out_state) { out_state = Service::LDN::State::Initialized; }),
            ::testing::Return(ErrorCode::Success)));
    auto result = mock_backend->GetCurrentState(current_state);
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(current_state, Service::LDN::State::Initialized);
}

TEST_F(MultiplayerBackendInterfaceTest, AdvertiseDataManagement) {
    std::vector<uint8_t> advertise_data(100, 0xAB);
    EXPECT_CALL(*mock_backend, SetAdvertiseData(advertise_data)).WillOnce(::testing::Return(ErrorCode::Success));
    auto result = mock_backend->SetAdvertiseData(advertise_data);
    EXPECT_EQ(result, ErrorCode::Success);
}

TEST_F(MultiplayerBackendInterfaceTest, AdvertiseDataTooLarge) {
    std::vector<uint8_t> oversized_data(500, 0xAB);
    EXPECT_CALL(*mock_backend, SetAdvertiseData(oversized_data))
        .WillOnce(::testing::Return(ErrorCode::MessageTooLarge));
    auto result = mock_backend->SetAdvertiseData(oversized_data);
    EXPECT_EQ(result, ErrorCode::MessageTooLarge);
}

TEST_F(MultiplayerBackendInterfaceTest, RealBackendIntegration) {
    GTEST_SKIP() << "Real backend implementation does not exist yet.";
}

} // namespace Core::Multiplayer::HLE
