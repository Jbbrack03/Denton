// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>

#include "error_handling.h"

namespace Core::Multiplayer {

// Forward declarations
class MockNetworkConnection;
class MockRecoveryListener;
class MockRecoveryStrategy;

/**
 * Recovery configuration settings
 */
struct RecoveryConfig {
    uint32_t max_retries = 3;
    uint32_t initial_delay_ms = 1000;
    uint32_t max_delay_ms = 30000;
    double backoff_multiplier = 2.0;
    bool jitter_enabled = true;
    uint32_t jitter_range_percent = 25;
    uint32_t connection_timeout_ms = 10000;
};

/**
 * Recovery state enumeration
 */
enum class RecoveryState {
    Idle,
    InProgress,
    Succeeded,
    Failed,
    Aborted
};

/**
 * Recovery strategy enumeration
 */
enum class RecoveryStrategy {
    ExponentialBackoff,
    ImmediateRetry,
    CertificateRetry,
    Custom
};

/**
 * Recovery status information
 */
struct RecoveryStatus {
    RecoveryState state;
    size_t current_attempt;
    size_t max_attempts;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::milliseconds elapsed_time;
    std::optional<ErrorInfo> last_error;
    RecoveryStrategy current_strategy;
};

/**
 * Connection Recovery Manager
 * 
 * Handles automatic connection recovery with exponential backoff, jitter,
 * and configurable retry strategies. Integrates with the existing error
 * handling framework to provide robust connection resilience.
 * 
 * Key Features:
 * - Exponential backoff with jitter
 * - Multiple retry strategies
 * - Error categorization and filtering
 * - Thread-safe operations
 * - Integration with circuit breaker patterns
 * 
 * Usage:
 *   RecoveryConfig config;
 *   config.max_retries = 3;
 *   config.initial_delay_ms = 1000;
 *   
 *   auto manager = std::make_unique<ConnectionRecoveryManager>(config, connection);
 *   manager->SetRecoveryListener(listener);
 *   
 *   auto error = CreateNetworkError(ErrorCode::ConnectionTimeout, "Connection failed");
 *   manager->StartRecovery(error);
 */
class ConnectionRecoveryManager {
public:
    explicit ConnectionRecoveryManager(const RecoveryConfig& config, 
                                     MockNetworkConnection* connection);
    ~ConnectionRecoveryManager();

    // Core recovery operations
    ErrorCode StartRecovery(const ErrorInfo& error);
    ErrorCode StopRecovery();
    void Shutdown();

    // State and status
    RecoveryState GetState() const;
    RecoveryStatus GetRecoveryStatus() const;

    // Error handling and strategy
    bool CanRetryError(const ErrorInfo& error) const;
    RecoveryStrategy GetRecoveryStrategy(const ErrorInfo& error) const;
    void RegisterStrategy(ErrorCode error_code, MockRecoveryStrategy* strategy);

    // Configuration
    void SetRecoveryListener(MockRecoveryListener* listener);
    void UpdateConfig(const RecoveryConfig& config);
    RecoveryConfig GetConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer
