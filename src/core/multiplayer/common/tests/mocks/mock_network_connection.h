// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include "core/multiplayer/common/error_codes.h"

namespace Core::Multiplayer {

/**
 * Mock network connection for testing connection recovery
 */
class MockNetworkConnection {
public:
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(ErrorCode, Connect, ());
    MOCK_METHOD(ErrorCode, Disconnect, ());
    MOCK_METHOD(ErrorCode, SendData, (const std::vector<uint8_t>& data));
    MOCK_METHOD(ErrorCode, ReceiveData, (std::vector<uint8_t>& data));
    MOCK_METHOD(void, SetTimeout, (uint32_t timeout_ms));
    MOCK_METHOD(uint32_t, GetTimeout, (), (const));
    MOCK_METHOD(std::string, GetEndpoint, (), (const));
    MOCK_METHOD(void, Reset, ());
};

/**
 * Mock recovery listener for testing recovery events
 */
class MockRecoveryListener {
public:
    MOCK_METHOD(void, OnRecoveryStarted, (const ErrorInfo& error));
    MOCK_METHOD(void, OnRecoveryAttempt, (const ErrorInfo& error, size_t attempt_number));
    MOCK_METHOD(void, OnRecoverySucceeded, (const ErrorInfo& error));
    MOCK_METHOD(void, OnRecoveryFailed, (const ErrorInfo& error, size_t final_attempt));
    MOCK_METHOD(void, OnRecoveryAborted, (const ErrorInfo& error));
};

/**
 * Mock recovery strategy for testing custom strategies
 */
class MockRecoveryStrategy {
public:
    MOCK_METHOD(bool, CanHandle, (ErrorCode error_code), (const));
    MOCK_METHOD(uint32_t, GetDelayMs, (const ErrorInfo& error, size_t attempt_number), (const));
    MOCK_METHOD(bool, ShouldRetry, (const ErrorInfo& error, size_t attempt_number), (const));
    MOCK_METHOD(std::string, GetName, (), (const));
};

} // namespace Core::Multiplayer
