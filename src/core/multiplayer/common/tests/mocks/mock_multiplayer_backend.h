// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <vector>
#include <string>
#include "core/multiplayer/common/graceful_degradation_manager.h"

namespace Core::Multiplayer {

/**
 * Mock multiplayer backend for testing graceful degradation
 */
class MockMultiplayerBackend {
public:
    MOCK_METHOD(bool, IsAvailable, (), (const));
    MOCK_METHOD(BackendHealthStatus, GetHealthStatus, (), (const));
    MOCK_METHOD(ErrorCode, Initialize, ());
    MOCK_METHOD(ErrorCode, Shutdown, ());
    MOCK_METHOD(ErrorCode, CreateSession, (const std::string& session_name));
    MOCK_METHOD(ErrorCode, JoinSession, (const std::string& session_id));
    MOCK_METHOD(ErrorCode, LeaveSession, ());
    MOCK_METHOD(std::vector<std::string>, GetAvailableSessions, (), (const));
    MOCK_METHOD(std::string, GetBackendName, (), (const));
    MOCK_METHOD(HealthMetrics, GetHealthMetrics, (), (const));
};

/**
 * Mock mode switch listener for testing degradation events
 */
class MockModeSwitchListener {
public:
    MOCK_METHOD(void, OnModeSwitchStarted, (MultiplayerMode from_mode, MultiplayerMode to_mode));
    MOCK_METHOD(void, OnModeSwitchCompleted, (MultiplayerMode new_mode));
    MOCK_METHOD(void, OnModeSwitchFailed, (MultiplayerMode target_mode, const ErrorInfo& error));
    MOCK_METHOD(void, OnAllBackendsFailed, ());
    MOCK_METHOD(void, OnMaxFallbackAttemptsReached, ());
    MOCK_METHOD(void, OnFallbackTimeout, (MultiplayerMode target_mode));
    MOCK_METHOD(void, OnModeRecoveryStarted, (MultiplayerMode recovery_mode));
    MOCK_METHOD(void, OnModeRecoveryCompleted, (MultiplayerMode recovered_mode));
    MOCK_METHOD(void, OnModeRecoveryFailed, (MultiplayerMode recovery_mode, const ErrorInfo& error));
    MOCK_METHOD(void, OnBackendDegradationDetected, (MultiplayerMode mode, BackendHealthStatus status));
    MOCK_METHOD(void, OnBackendInitializationFailed, (MultiplayerMode mode, const ErrorInfo& error));
    MOCK_METHOD(void, OnUserNotification, (NotificationLevel level, const std::string& message, const std::vector<std::string>& actions));
    MOCK_METHOD(void, OnFallbackProgress, (MultiplayerMode target_mode, int progress_percent));
};

} // namespace Core::Multiplayer
