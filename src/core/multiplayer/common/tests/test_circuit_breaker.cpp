// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

#include "core/multiplayer/common/circuit_breaker.h"
#include "mocks/mock_service_operation.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace std::chrono_literals;

// Basic state machine behaviour ------------------------------------------------

TEST(CircuitBreakerTest, ClosedToOpenAfterFailures) {
    CircuitBreakerConfig config;
    config.failure_threshold = 2;
    CircuitBreaker breaker(config);

    MockServiceOperation op;
    EXPECT_CALL(op, Execute())
        .Times(2)
        .WillRepeatedly(Return(ErrorCode::NetworkTimeout));

    breaker.Execute([&]() { return op.Execute(); });
    breaker.Execute([&]() { return op.Execute(); });

    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::Open);
}

TEST(CircuitBreakerTest, OpenToHalfOpenAfterTimeout) {
    CircuitBreakerConfig config;
    config.failure_threshold = 1;
    config.timeout_duration_ms = 50;
    config.success_threshold_for_close = 2; // stay half-open after first success
    CircuitBreaker breaker(config);

    MockServiceOperation op;
    EXPECT_CALL(op, Execute())
        .WillOnce(Return(ErrorCode::NetworkTimeout))
        .WillOnce(Return(ErrorCode::Success));

    breaker.Execute([&]() { return op.Execute(); }); // open breaker
    std::this_thread::sleep_for(60ms);
    breaker.Execute([&]() { return op.Execute(); }); // allowed half-open call

    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::HalfOpen);
}

TEST(CircuitBreakerTest, HalfOpenToClosedOnSuccess) {
    CircuitBreakerConfig config;
    config.failure_threshold = 1;
    config.timeout_duration_ms = 50;
    config.success_threshold_for_close = 1;
    CircuitBreaker breaker(config);

    MockServiceOperation op;
    EXPECT_CALL(op, Execute())
        .WillOnce(Return(ErrorCode::NetworkTimeout))
        .WillOnce(Return(ErrorCode::Success));

    breaker.Execute([&]() { return op.Execute(); }); // open
    std::this_thread::sleep_for(60ms);
    breaker.Execute([&]() { return op.Execute(); }); // success closes

    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::Closed);
}

TEST(CircuitBreakerTest, HalfOpenToOpenOnFailure) {
    CircuitBreakerConfig config;
    config.failure_threshold = 1;
    config.timeout_duration_ms = 50;
    CircuitBreaker breaker(config);

    MockServiceOperation op;
    EXPECT_CALL(op, Execute())
        .WillOnce(Return(ErrorCode::NetworkTimeout))
        .WillOnce(Return(ErrorCode::NetworkTimeout));

    breaker.Execute([&]() { return op.Execute(); }); // open
    std::this_thread::sleep_for(60ms);
    breaker.Execute([&]() { return op.Execute(); }); // failure in half-open

    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::Open);
}

