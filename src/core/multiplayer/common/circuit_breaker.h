// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <string>
#include <unordered_map>

#include "error_handling.h"

namespace Core::Multiplayer {

// Forward declarations
class MockCircuitBreakerListener;

/**
 * Circuit breaker configuration settings
 */
struct CircuitBreakerConfig {
    size_t failure_threshold = 5;
    uint32_t timeout_duration_ms = 60000;
    size_t success_threshold_for_close = 3;
    size_t sliding_window_size = 10;
    size_t minimum_throughput = 10;
    bool enable_automatic_half_open = true;
    size_t max_concurrent_half_open_calls = 1;
};

/**
 * Circuit breaker state enumeration
 */
enum class CircuitBreakerState {
    Closed,   // Normal operation, all calls allowed
    Open,     // Failure state, all calls rejected
    HalfOpen  // Testing state, limited calls allowed
};

/**
 * Circuit breaker metrics
 */
struct CircuitBreakerMetrics {
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    size_t rejected_requests = 0;
    size_t consecutive_failures = 0;
    double success_rate = 0.0;
    double failure_rate = 0.0;
    
    // Response time metrics
    double average_response_time_ms = 0.0;
    double min_response_time_ms = 0.0;
    double max_response_time_ms = 0.0;
    
    // State timing
    std::chrono::steady_clock::time_point last_state_change;
    std::chrono::milliseconds time_in_current_state{0};
};

/**
 * Circuit Breaker Implementation
 * 
 * Implements the circuit breaker pattern to prevent cascading failures
 * by temporarily disabling failing operations. The circuit breaker has
 * three states: Closed (normal), Open (failing), and Half-Open (testing).
 * 
 * Key Features:
 * - Automatic failure detection and circuit opening
 * - Configurable failure thresholds and timeouts
 * - Half-open state for gradual recovery testing
 * - Sliding window for failure rate calculation
 * - Comprehensive metrics and monitoring
 * - Thread-safe operations
 * 
 * Usage:
 *   CircuitBreakerConfig config;
 *   config.failure_threshold = 5;
 *   config.timeout_duration_ms = 60000;
 *   
 *   auto breaker = std::make_unique<CircuitBreaker>(config);
 *   breaker->SetListener(listener);
 *   
 *   auto result = breaker->Execute([&]() {
 *       return some_network_operation();
 *   });
 */
class CircuitBreaker {
public:
    explicit CircuitBreaker(const CircuitBreakerConfig& config);
    ~CircuitBreaker();

    // Core circuit breaker operations
    template<typename F>
    ErrorCode Execute(F&& operation);
    
    // State management
    CircuitBreakerState GetState() const;
    void ForceOpen();
    void ForceHalfOpen();
    void Reset();

    // Metrics and monitoring
    CircuitBreakerMetrics GetMetrics() const;
    std::string ExportMetricsAsJSON() const;

    // Configuration
    void SetListener(MockCircuitBreakerListener* listener);
    void UpdateConfig(const CircuitBreakerConfig& config);
    CircuitBreakerConfig GetConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal helper for template implementation
    ErrorCode ExecuteInternal(std::function<ErrorCode()> operation);
};

// Template implementation
template<typename F>
ErrorCode CircuitBreaker::Execute(F&& operation) {
    return ExecuteInternal(std::forward<F>(operation));
}

} // namespace Core::Multiplayer
