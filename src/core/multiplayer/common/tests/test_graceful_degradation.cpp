// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "core/multiplayer/common/graceful_degradation_manager.h"
#include "mocks/mock_multiplayer_backend.h"

using namespace testing;
using namespace Core::Multiplayer;

// Verify that a network error triggers fallback to the Adhoc backend

TEST(GracefulDegradationTest, FallbackToAdhocOnNetworkError) {
    DegradationConfig config;
    GracefulDegradationManager manager(config);

    MockMultiplayerBackend internet_backend;
    MockMultiplayerBackend adhoc_backend;
    MockModeSwitchListener listener;

    manager.SetInternetBackend(&internet_backend);
    manager.SetAdhocBackend(&adhoc_backend);
    manager.SetModeSwitchListener(&listener);

    ASSERT_EQ(manager.Initialize(MultiplayerMode::Internet), ErrorCode::Success);

    auto error = CreateNetworkError(ErrorCode::NetworkTimeout, "net fail");
    manager.HandleError(error);

    EXPECT_EQ(manager.GetCurrentMode(), MultiplayerMode::Adhoc);
    EXPECT_EQ(manager.GetState(), DegradationState::Degraded);
}

