// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include "core/multiplayer/common/error_codes.h"

namespace Core::Multiplayer {

/**
 * Mock service operation for testing circuit breaker
 */
class MockServiceOperation {
public:
    MOCK_METHOD(ErrorCode, Execute, ());
    MOCK_METHOD(ErrorCode, ExecuteWithTimeout, (uint32_t timeout_ms));
    MOCK_METHOD(void, Cancel, ());
    MOCK_METHOD(bool, IsComplete, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetExecutionTime, (), (const));
};

/**
 * Mock circuit breaker listener for testing events
 */
class MockCircuitBreakerListener {
public:
    MOCK_METHOD(void, OnCircuitOpened, (const ErrorInfo& trigger_error));
    MOCK_METHOD(void, OnCircuitClosed, ());
    MOCK_METHOD(void, OnCircuitHalfOpened, ());
    MOCK_METHOD(void, OnOperationRejected, (const std::string& reason));
    MOCK_METHOD(void, OnThresholdExceeded, (size_t failure_count, size_t threshold));
    MOCK_METHOD(void, OnMetricsUpdated, (const std::string& metrics_json));
};

} // namespace Core::Multiplayer
