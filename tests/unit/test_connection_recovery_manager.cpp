// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <gtest/gtest.h>
#include <optional>
#include <thread>
#include <unordered_map>

#include "core/multiplayer/common/connection_recovery_manager.h"

namespace Core::Multiplayer {
// Minimal connection implementation for testing
class MockNetworkConnection {};
} // namespace Core::Multiplayer

using namespace Core::Multiplayer;

TEST(ConnectionRecoveryManagerTest, InstancesDoNotShareAttemptCount) {
    RecoveryConfig config;
    config.max_retries = 3;
    config.initial_delay_ms = 1;
    config.max_delay_ms = 1;
    config.jitter_enabled = false;

    MockNetworkConnection connection;

    ConnectionRecoveryManager manager1(config, &connection);
    ConnectionRecoveryManager manager2(config, &connection);

    ErrorInfo error{ErrorCategory::NetworkConnectivity,
                    ErrorCode::NetworkTimeout,
                    "test",
                    std::nullopt,
                    {},
                    std::chrono::steady_clock::now(),
                    "test",
                    {}};

    ASSERT_EQ(ErrorCode::Success, manager1.StartRecovery(error));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(RecoveryState::Succeeded, manager1.GetState());

    ASSERT_EQ(ErrorCode::Success, manager2.StartRecovery(error));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(RecoveryState::Succeeded, manager2.GetState());

    manager1.Shutdown();
    manager2.Shutdown();
}

