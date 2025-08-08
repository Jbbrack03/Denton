// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>

#include "mocks/mock_jni_env.h"
#include "core/multiplayer/model_b/platform/android/wifi_direct_wrapper.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelB::Testing;
using namespace Core::Multiplayer::ModelB::Android;

namespace {

class WiFiDirectWrapperTest : public Test {
protected:
    void SetUp() override {
        mock_env = std::make_unique<MockJNIEnv>();
        mock_context = std::make_unique<MockAndroidContext>();
        ON_CALL(*mock_context, GetSystemService("wifip2p"))
            .WillByDefault(Return(reinterpret_cast<jobject>(0x1)));
        ON_CALL(*mock_context, GetSDKVersion()).WillByDefault(Return(33));
        ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(true));
    }

    void CreateWrapper() { wrapper = std::make_unique<WiFiDirectWrapper>(); }

    std::unique_ptr<MockJNIEnv> mock_env;
    std::unique_ptr<MockAndroidContext> mock_context;
    std::unique_ptr<WiFiDirectWrapper> wrapper;
};

TEST_F(WiFiDirectWrapperTest, InitializeAndShutdown) {
    CreateWrapper();
    EXPECT_EQ(wrapper->Initialize(mock_env.get(), mock_context.get()), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Initialized);
    wrapper->Shutdown();
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Uninitialized);
}

TEST_F(WiFiDirectWrapperTest, InitializeFailsWithoutPermissions) {
    CreateWrapper();
    EXPECT_CALL(*mock_context, CheckSelfPermission("android.permission.NEARBY_WIFI_DEVICES"))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_context, CheckSelfPermission("android.permission.ACCESS_FINE_LOCATION"))
        .WillOnce(Return(false));
    EXPECT_EQ(wrapper->Initialize(mock_env.get(), mock_context.get()), ErrorCode::PermissionDenied);
}

TEST_F(WiFiDirectWrapperTest, StartAndStopDiscovery) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->StartDiscovery(nullptr), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Discovering);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(wrapper->StopDiscovery(), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Initialized);
}

TEST_F(WiFiDirectWrapperTest, GetDiscoveredPeersInitiallyEmpty) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    auto peers = wrapper->GetDiscoveredPeers();
    EXPECT_TRUE(peers.empty());
}

TEST_F(WiFiDirectWrapperTest, ConnectAndDisconnectPeer) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->ConnectToPeer("11:22:33:44:55:66"), ErrorCode::Success);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Connected);
    EXPECT_EQ(wrapper->DisconnectFromPeer(), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Initialized);
}

TEST_F(WiFiDirectWrapperTest, CancelConnectionBeforeCompletion) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->ConnectToPeer("11:22:33:44:55:66"), ErrorCode::Success);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(wrapper->CancelConnection(), ErrorCode::Success);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Initialized);
}

TEST_F(WiFiDirectWrapperTest, ConnectToPeerValidatesAddress) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->ConnectToPeer("invalid"), ErrorCode::InvalidParameter);
}

TEST_F(WiFiDirectWrapperTest, CreateAndRemoveGroup) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->CreateGroup(), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::GroupOwner);
    auto info = wrapper->GetGroupInfo();
    EXPECT_TRUE(info.is_group_owner);
    EXPECT_FALSE(info.network_name.empty());
    EXPECT_EQ(wrapper->RemoveGroup(), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::Initialized);
}

TEST_F(WiFiDirectWrapperTest, JoinGroupSucceeds) {
    CreateWrapper();
    wrapper->Initialize(mock_env.get(), mock_context.get());
    EXPECT_EQ(wrapper->JoinGroup("AA:BB:CC:DD:EE:FF"), ErrorCode::Success);
    EXPECT_EQ(wrapper->GetState(), WifiDirectState::GroupClient);
}

} // namespace

