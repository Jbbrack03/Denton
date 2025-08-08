// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "core/multiplayer/model_b/platform/windows/mobile_hotspot_manager.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelB::Windows;

namespace {

class MobileHotspotManagerTest : public Test {
protected:
    void CreateManager() { manager = std::make_unique<MobileHotspotManager>(); }
    std::unique_ptr<MobileHotspotManager> manager;
};

TEST_F(MobileHotspotManagerTest, InitializeSetsState) {
    CreateManager();
    EXPECT_EQ(manager->Initialize(), ErrorCode::Success);
    EXPECT_EQ(manager->GetState(), HotspotState::Initialized);
}

TEST_F(MobileHotspotManagerTest, ConfigureHotspotValidatesInput) {
    CreateManager();
    manager->Initialize();
    HotspotConfiguration cfg{"Sudachi", "password", 4, WiFiBand::TwoPointFourGHz, 6};
    EXPECT_EQ(manager->ConfigureHotspot(cfg), ErrorCode::Success);

    cfg.passphrase = "short";
    EXPECT_EQ(manager->ConfigureHotspot(cfg), ErrorCode::ConfigurationInvalid);
}

TEST_F(MobileHotspotManagerTest, StartAndStopHotspotTransitionsState) {
    CreateManager();
    manager->Initialize();
    HotspotConfiguration cfg{"Sudachi", "password", 4, WiFiBand::TwoPointFourGHz, 6};
    manager->ConfigureHotspot(cfg);
    EXPECT_EQ(manager->StartHotspot(), ErrorCode::Success);
    EXPECT_EQ(manager->GetState(), HotspotState::Active);
    EXPECT_EQ(manager->StopHotspot(), ErrorCode::Success);
    EXPECT_EQ(manager->GetState(), HotspotState::Initialized);
}

TEST_F(MobileHotspotManagerTest, ClientConnectionsUpdateStatus) {
    CreateManager();
    manager->Initialize();
    HotspotConfiguration cfg{"Sudachi", "password", 4, WiFiBand::TwoPointFourGHz, 6};
    manager->ConfigureHotspot(cfg);
    manager->StartHotspot();

    ClientInfo client{"AA:BB:CC:DD:EE:FF", "192.168.0.2", "Device"};
    manager->OnClientConnected(client);
    auto status = manager->GetHotspotStatus();
    EXPECT_EQ(status.connected_clients, 1);
    manager->OnClientDisconnected(client.mac_address);
    status = manager->GetHotspotStatus();
    EXPECT_EQ(status.connected_clients, 0);
}

TEST_F(MobileHotspotManagerTest, FallbackModeSelectionWorks) {
    CreateManager();
    EXPECT_EQ(manager->GetRecommendedFallbackMode(), FallbackMode::WiFiDirect);
    EXPECT_EQ(manager->InitializeFallbackMode(FallbackMode::InternetMode), ErrorCode::Success);
    EXPECT_EQ(manager->GetCurrentMode(), OperationMode::InternetFallback);
}

} // namespace

