// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

#include "core/multiplayer/common/circuit_breaker.h"
#include "core/multiplayer/common/graceful_degradation_manager.h"
#include "mocks/mock_service_operation.h"
#include "mocks/mock_multiplayer_backend.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace std::chrono_literals;

// Verify that circuit breaker failures lead to graceful fallback and recovery

TEST(ResilienceIntegrationTest, CircuitBreakersTriggerFallbackAndRecovery) {
    // Set up degradation manager with both backends
    DegradationConfig deg_config;
    GracefulDegradationManager degradation_manager(deg_config);
    MockMultiplayerBackend internet_backend;
    MockMultiplayerBackend adhoc_backend;
    degradation_manager.SetInternetBackend(&internet_backend);
    degradation_manager.SetAdhocBackend(&adhoc_backend);
    ASSERT_EQ(degradation_manager.Initialize(MultiplayerMode::Internet), ErrorCode::Success);

    // Circuit breaker that opens after a single failure
    CircuitBreakerConfig cb_config;
    cb_config.failure_threshold = 1;
    cb_config.timeout_duration_ms = 50;
    cb_config.success_threshold_for_close = 1;
    CircuitBreaker breaker(cb_config);

    MockServiceOperation op;
    EXPECT_CALL(op, Execute())
        .WillOnce(Return(ErrorCode::NetworkTimeout))
        .WillOnce(Return(ErrorCode::Success));

    // First call fails and opens breaker
    breaker.Execute([&]() { return op.Execute(); });
    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::Open);

    // Handle failure through degradation manager -> should fallback to Adhoc
    auto error = CreateNetworkError(ErrorCode::NetworkTimeout, "network failure");
    degradation_manager.HandleError(error);
    EXPECT_EQ(degradation_manager.GetCurrentMode(), MultiplayerMode::Adhoc);

    // After timeout, breaker allows a success which closes circuit
    std::this_thread::sleep_for(60ms);
    breaker.Execute([&]() { return op.Execute(); });
    EXPECT_EQ(breaker.GetState(), CircuitBreakerState::Closed);
    EXPECT_EQ(degradation_manager.GetCurrentMode(), MultiplayerMode::Adhoc);
    EXPECT_EQ(degradation_manager.GetState(), DegradationState::Degraded);
}

