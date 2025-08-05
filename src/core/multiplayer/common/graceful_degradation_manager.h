// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>

#include "error_handling.h"

namespace Core::Multiplayer {

// Forward declarations
class MockMultiplayerBackend;
class MockModeSwitchListener;

/**
 * Multiplayer mode enumeration
 */
enum class MultiplayerMode {
    Internet,
    Adhoc,
    Offline
};

/**
 * Backend health status enumeration
 */
enum class BackendHealthStatus {
    Healthy,
    Degraded,
    Unhealthy,
    Unknown
};

/**
 * Degradation state enumeration
 */
enum class DegradationState {
    Normal,     // All backends operating normally
    Degraded,   // Operating on fallback backend
    Failed,     // All backends failed, offline mode
    Recovering  // Attempting to recover to primary backend
};

/**
 * Graceful degradation configuration
 */
struct DegradationConfig {
    bool enable_auto_fallback = true;
    bool enable_auto_recovery = true;
    uint32_t fallback_timeout_ms = 10000;
    uint32_t recovery_check_interval_ms = 30000;
    size_t max_fallback_attempts = 2;
    uint32_t health_check_interval_ms = 10000;
};

/**
 * Degradation status information
 */
struct DegradationStatus {
    MultiplayerMode current_mode;
    MultiplayerMode original_mode;
    DegradationState state;
    size_t fallback_attempts;
    bool is_degraded;
    std::chrono::steady_clock::time_point degradation_start_time;
    std::chrono::milliseconds time_in_degraded_state{0};
};

/**
 * Health metrics for backends
 */
struct HealthMetrics {
    double uptime_percentage;
    size_t total_health_checks;
    size_t failed_health_checks;
    double average_response_time_ms;
    std::chrono::steady_clock::time_point last_check_time;
};

/**
 * Graceful Degradation Manager
 * 
 * Manages automatic fallback between Internet and Ad-hoc multiplayer modes
 * when backend services become unavailable. Provides seamless user experience
 * by maintaining service continuity through backend transitions.
 * 
 * Key Features:
 * - Automatic fallback between multiplayer modes
 * - Health monitoring of backend services
 * - Automatic recovery when primary backends recover
 * - Service continuity during transitions
 * - User notification and progress feedback
 * - Configurable fallback policies
 * 
 * Usage:
 *   DegradationConfig config;
 *   config.enable_auto_fallback = true;
 *   config.fallback_timeout_ms = 10000;
 *   
 *   auto manager = std::make_unique<GracefulDegradationManager>(config);
 *   manager->SetInternetBackend(internet_backend);
 *   manager->SetAdhocBackend(adhoc_backend);
 *   manager->SetModeSwitchListener(listener);
 *   
 *   manager->Initialize(MultiplayerMode::Internet);
 *   
 *   // Error handling triggers automatic fallback
 *   auto error = CreateNetworkError(ErrorCode::NetworkTimeout, "Connection failed");
 *   manager->HandleError(error);
 */
class GracefulDegradationManager {
public:
    explicit GracefulDegradationManager(const DegradationConfig& config);
    ~GracefulDegradationManager();

    // Initialization and lifecycle
    ErrorCode Initialize(MultiplayerMode primary_mode);
    void Shutdown();

    // Mode management
    MultiplayerMode GetCurrentMode() const;
    DegradationState GetState() const;
    DegradationStatus GetDegradationStatus() const;

    // Backend management
    void SetInternetBackend(MockMultiplayerBackend* backend);
    void SetAdhocBackend(MockMultiplayerBackend* backend);
    bool IsBackendAvailable(MultiplayerMode mode) const;
    std::vector<MultiplayerMode> GetSupportedFallbackModes(MultiplayerMode from_mode) const;

    // Error handling and fallback
    void HandleError(const ErrorInfo& error);
    ErrorCode AttemptRecovery(MultiplayerMode target_mode);

    // Health monitoring
    void CheckBackendHealth(MultiplayerMode mode);
    HealthMetrics GetHealthMetrics(MultiplayerMode mode) const;

    // Service availability
    bool IsServiceAvailable() const;

    // Configuration and listeners
    void SetModeSwitchListener(MockModeSwitchListener* listener);
    void UpdateConfig(const DegradationConfig& config);
    DegradationConfig GetConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer