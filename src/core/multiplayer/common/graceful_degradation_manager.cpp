// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graceful_degradation_manager.h"
#include <thread>
#include <algorithm>
#include <iostream>
#include <atomic>

namespace Core::Multiplayer {

// Implementation class using PIMPL pattern
class GracefulDegradationManager::Impl {
public:
    explicit Impl(const DegradationConfig& config)
        : config_(config), current_mode_(MultiplayerMode::Offline), 
          original_mode_(MultiplayerMode::Offline), state_(DegradationState::Normal),
          fallback_attempts_(0), is_degraded_(false), internet_backend_(nullptr),
          adhoc_backend_(nullptr), listener_(nullptr), shutdown_requested_(false) {
        
        degradation_start_time_ = std::chrono::steady_clock::now();
        
        // Initialize health metrics
        InitializeHealthMetrics();
    }

    ~Impl() {
        Shutdown();
    }

    ErrorCode Initialize(MultiplayerMode primary_mode) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        current_mode_ = primary_mode;
        original_mode_ = primary_mode;
        state_ = DegradationState::Normal;
        fallback_attempts_ = 0;
        is_degraded_ = false;
        
        // Check if the primary backend is available
        if (!IsBackendAvailableLocked(primary_mode)) {
            std::cout << "Primary backend " << ModeToString(primary_mode) 
                      << " not available, attempting fallback" << std::endl;
            
            // Try to find an available fallback
            auto fallback_modes = GetSupportedFallbackModes(primary_mode);
            for (auto fallback_mode : fallback_modes) {
                if (IsBackendAvailableLocked(fallback_mode)) {
                    current_mode_ = fallback_mode;
                    state_ = DegradationState::Degraded;
                    is_degraded_ = true;
                    degradation_start_time_ = std::chrono::steady_clock::now();
                    
                    if (listener_) {
                        // For minimal implementation, just log the mode switch
                        // In real implementation, this would call listener methods
                        std::cout << "Mode switch started: " << ModeToString(primary_mode) 
                                  << " -> " << ModeToString(fallback_mode) << std::endl;
                        std::cout << "Mode switch completed: " << ModeToString(fallback_mode) << std::endl;
                    }
                    
                    std::cout << "Successfully fell back to " << ModeToString(fallback_mode) 
                              << " mode" << std::endl;
                    break;
                }
            }
            
            if (current_mode_ == primary_mode) {
                state_ = DegradationState::Failed;
                if (listener_) {
                    // For minimal implementation, just log the failure
                    std::cout << "All backends failed" << std::endl;
                }
                return ErrorCode::ServiceUnavailable;
            }
        }
        
        // Start health monitoring if enabled
        if (config_.enable_auto_recovery) {
            StartHealthMonitoring();
        }
        
        std::cout << "Graceful degradation manager initialized with mode: " 
                  << ModeToString(current_mode_) << std::endl;
        return ErrorCode::Success;
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_requested_ = true;
        }
        
        if (health_monitor_thread_.joinable()) {
            health_monitor_thread_.join();
        }
    }

    MultiplayerMode GetCurrentMode() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_mode_;
    }

    DegradationState GetState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    DegradationStatus GetDegradationStatus() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        DegradationStatus status;
        status.current_mode = current_mode_;
        status.original_mode = original_mode_;
        status.state = state_;
        status.fallback_attempts = fallback_attempts_;
        status.is_degraded = is_degraded_;
        status.degradation_start_time = degradation_start_time_;
        
        auto now = std::chrono::steady_clock::now();
        status.time_in_degraded_state = 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - degradation_start_time_);
        
        return status;
    }

    void SetInternetBackend(MockMultiplayerBackend* backend) {
        std::lock_guard<std::mutex> lock(mutex_);
        internet_backend_ = backend;
    }

    void SetAdhocBackend(MockMultiplayerBackend* backend) {
        std::lock_guard<std::mutex> lock(mutex_);
        adhoc_backend_ = backend;
    }

    bool IsBackendAvailable(MultiplayerMode mode) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return IsBackendAvailableLocked(mode);
    }

    bool IsBackendAvailableLocked(MultiplayerMode mode) const {
        MockMultiplayerBackend* backend = GetBackendForMode(mode);
        // For minimal implementation, simulate backend availability
        // In real implementation, this would call backend->IsAvailable()
        if (!backend) {
            return mode == MultiplayerMode::Offline; // Offline mode is always available
        }

        // Simulate: Internet is available 80% of the time, Adhoc 90% of the time
        static std::atomic<int> check_count{0};
        int current_count = ++check_count;

        switch (mode) {
            case MultiplayerMode::Internet:
                return (current_count % 5) != 0; // 80% availability
            case MultiplayerMode::Adhoc:
                return (current_count % 10) != 0; // 90% availability
            case MultiplayerMode::Offline:
                return true;
            default:
                return false;
        }
    }

    std::vector<MultiplayerMode> GetSupportedFallbackModes(MultiplayerMode from_mode) const {
        std::vector<MultiplayerMode> fallback_modes;
        
        switch (from_mode) {
            case MultiplayerMode::Internet:
                fallback_modes.push_back(MultiplayerMode::Adhoc);
                fallback_modes.push_back(MultiplayerMode::Offline);
                break;
            case MultiplayerMode::Adhoc:
                fallback_modes.push_back(MultiplayerMode::Internet);
                fallback_modes.push_back(MultiplayerMode::Offline);
                break;
            case MultiplayerMode::Offline:
                // No fallback from offline mode
                break;
        }
        
        return fallback_modes;
    }

    void HandleError(const ErrorInfo& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!config_.enable_auto_fallback) {
            return;
        }
        
        // Check if error warrants fallback
        if (!ShouldTriggerFallback(error)) {
            return;
        }
        
        if (fallback_attempts_ >= config_.max_fallback_attempts) {
            if (listener_) {
                // For minimal implementation, just log the max attempts
                std::cout << "Max fallback attempts reached" << std::endl;
            }
            return;
        }
        
        // Attempt fallback to another mode
        auto fallback_modes = GetSupportedFallbackModes(current_mode_);
        for (auto fallback_mode : fallback_modes) {
            if (IsBackendAvailableLocked(fallback_mode)) {
                AttemptFallback(fallback_mode, error);
                break;
            }
        }
    }

    ErrorCode AttemptRecovery(MultiplayerMode target_mode) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!IsBackendAvailableLocked(target_mode)) {
            return ErrorCode::ServiceUnavailable;
        }
        
        MultiplayerMode old_mode = current_mode_;
        current_mode_ = target_mode;
        
        // Check if we're recovering to the original mode
        if (target_mode == original_mode_) {
            state_ = DegradationState::Normal;
            is_degraded_ = false;
            fallback_attempts_ = 0;
        } else {
            state_ = DegradationState::Degraded;
        }
        
        if (listener_) {
            // For minimal implementation, just log the recovery
            std::cout << "Mode recovery started: " << ModeToString(target_mode) << std::endl;
            std::cout << "Mode recovery completed: " << ModeToString(target_mode) << std::endl;
        }
        
        std::cout << "Successfully recovered to " << ModeToString(target_mode) 
                  << " mode" << std::endl;
        return ErrorCode::Success;
    }

    void CheckBackendHealth(MultiplayerMode mode) {
        std::lock_guard<std::mutex> lock(mutex_);
        MockMultiplayerBackend* backend = GetBackendForMode(mode);
        if (!backend) {
            return;
        }

        auto& metrics = health_metrics_[mode];
        metrics.total_health_checks++;
        metrics.last_check_time = std::chrono::steady_clock::now();

        // For minimal implementation, simulate health check
        // In real implementation, this would call backend->IsAvailable()
        try {
            auto start_time = std::chrono::steady_clock::now();
            bool is_available = IsBackendAvailableLocked(mode);
            auto end_time = std::chrono::steady_clock::now();

            auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();

            if (is_available) {
                // Successful health check
                UpdateResponseTimeMetrics(mode, response_time);
            } else {
                metrics.failed_health_checks++;
            }
        } catch (...) {
            metrics.failed_health_checks++;
        }

        // Update uptime percentage
        if (metrics.total_health_checks > 0) {
            metrics.uptime_percentage =
                static_cast<double>(metrics.total_health_checks - metrics.failed_health_checks) /
                metrics.total_health_checks * 100.0;
        }
    }

    HealthMetrics GetHealthMetrics(MultiplayerMode mode) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = health_metrics_.find(mode);
        return (it != health_metrics_.end()) ? it->second : HealthMetrics{};
    }

    bool IsServiceAvailable() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return IsBackendAvailableLocked(current_mode_);
    }

    void SetModeSwitchListener(MockModeSwitchListener* listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listener_ = listener;
    }

    void UpdateConfig(const DegradationConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    DegradationConfig GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

private:
    void InitializeHealthMetrics() {
        for (auto mode : {MultiplayerMode::Internet, MultiplayerMode::Adhoc, MultiplayerMode::Offline}) {
            HealthMetrics metrics{};
            metrics.uptime_percentage = 100.0;
            metrics.total_health_checks = 0;
            metrics.failed_health_checks = 0;
            metrics.average_response_time_ms = 0.0;
            metrics.last_check_time = std::chrono::steady_clock::now();
            health_metrics_[mode] = metrics;
        }
    }

    void StartHealthMonitoring() {
        health_monitor_thread_ = std::thread([this]() {
            while (true) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (shutdown_requested_) {
                        break;
                    }
                }
                
                // Check health of all backends
                CheckBackendHealth(MultiplayerMode::Internet);
                CheckBackendHealth(MultiplayerMode::Adhoc);
                
                // Sleep for the configured interval
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config_.health_check_interval_ms));
            }
        });
    }

    MockMultiplayerBackend* GetBackendForMode(MultiplayerMode mode) const {
        switch (mode) {
            case MultiplayerMode::Internet:
                return internet_backend_;
            case MultiplayerMode::Adhoc:
                return adhoc_backend_;
            case MultiplayerMode::Offline:
                return nullptr; // Offline mode doesn't need a backend
            default:
                return nullptr;
        }
    }

    bool ShouldTriggerFallback(const ErrorInfo& error) const {
        // Determine if error should trigger fallback
        switch (error.error_code) {
            case ErrorCode::NetworkTimeout:
            case ErrorCode::ConnectionLost:
            case ErrorCode::HostUnreachable:
            case ErrorCode::ServiceUnavailable:
                return true;
            default:
                return false;
        }
    }

    void AttemptFallback(MultiplayerMode target_mode, const ErrorInfo& error) {
        MultiplayerMode old_mode = current_mode_;
        current_mode_ = target_mode;
        state_ = DegradationState::Degraded;
        is_degraded_ = true;
        fallback_attempts_++;
        degradation_start_time_ = std::chrono::steady_clock::now();
        
        if (listener_) {
            // For minimal implementation, just log the switch
            std::cout << "Mode switch started: " << ModeToString(old_mode) 
                      << " -> " << ModeToString(target_mode) << std::endl;
            std::cout << "Mode switch completed: " << ModeToString(target_mode) << std::endl;
        }
        
        std::cout << "Fell back from " << ModeToString(old_mode) << " to " 
                  << ModeToString(target_mode) << " due to error: " 
                  << static_cast<int>(error.error_code) << std::endl;
    }

    void UpdateResponseTimeMetrics(MultiplayerMode mode, double response_time_ms) {
        auto& metrics = health_metrics_[mode];
        
        if (metrics.total_health_checks == 1) {
            metrics.average_response_time_ms = response_time_ms;
        } else {
            // Update running average
            double total_time = metrics.average_response_time_ms * (metrics.total_health_checks - 1);
            metrics.average_response_time_ms = (total_time + response_time_ms) / metrics.total_health_checks;
        }
    }

    std::string ModeToString(MultiplayerMode mode) const {
        switch (mode) {
            case MultiplayerMode::Internet: return "Internet";
            case MultiplayerMode::Adhoc: return "Adhoc";
            case MultiplayerMode::Offline: return "Offline";
            default: return "Unknown";
        }
    }

    mutable std::mutex mutex_;
    DegradationConfig config_;
    MultiplayerMode current_mode_;
    MultiplayerMode original_mode_;
    DegradationState state_;
    size_t fallback_attempts_;
    bool is_degraded_;
    std::chrono::steady_clock::time_point degradation_start_time_;
    
    MockMultiplayerBackend* internet_backend_;
    MockMultiplayerBackend* adhoc_backend_;
    MockModeSwitchListener* listener_;
    
    std::unordered_map<MultiplayerMode, HealthMetrics> health_metrics_;
    std::thread health_monitor_thread_;
    bool shutdown_requested_;
};

// GracefulDegradationManager implementation
GracefulDegradationManager::GracefulDegradationManager(const DegradationConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

GracefulDegradationManager::~GracefulDegradationManager() = default;

ErrorCode GracefulDegradationManager::Initialize(MultiplayerMode primary_mode) {
    return impl_->Initialize(primary_mode);
}

void GracefulDegradationManager::Shutdown() {
    impl_->Shutdown();
}

MultiplayerMode GracefulDegradationManager::GetCurrentMode() const {
    return impl_->GetCurrentMode();
}

DegradationState GracefulDegradationManager::GetState() const {
    return impl_->GetState();
}

DegradationStatus GracefulDegradationManager::GetDegradationStatus() const {
    return impl_->GetDegradationStatus();
}

void GracefulDegradationManager::SetInternetBackend(MockMultiplayerBackend* backend) {
    impl_->SetInternetBackend(backend);
}

void GracefulDegradationManager::SetAdhocBackend(MockMultiplayerBackend* backend) {
    impl_->SetAdhocBackend(backend);
}

bool GracefulDegradationManager::IsBackendAvailable(MultiplayerMode mode) const {
    return impl_->IsBackendAvailable(mode);
}

std::vector<MultiplayerMode> GracefulDegradationManager::GetSupportedFallbackModes(MultiplayerMode from_mode) const {
    return impl_->GetSupportedFallbackModes(from_mode);
}

void GracefulDegradationManager::HandleError(const ErrorInfo& error) {
    impl_->HandleError(error);
}

ErrorCode GracefulDegradationManager::AttemptRecovery(MultiplayerMode target_mode) {
    return impl_->AttemptRecovery(target_mode);
}

void GracefulDegradationManager::CheckBackendHealth(MultiplayerMode mode) {
    impl_->CheckBackendHealth(mode);
}

HealthMetrics GracefulDegradationManager::GetHealthMetrics(MultiplayerMode mode) const {
    return impl_->GetHealthMetrics(mode);
}

bool GracefulDegradationManager::IsServiceAvailable() const {
    return impl_->IsServiceAvailable();
}

void GracefulDegradationManager::SetModeSwitchListener(MockModeSwitchListener* listener) {
    impl_->SetModeSwitchListener(listener);
}

void GracefulDegradationManager::UpdateConfig(const DegradationConfig& config) {
    impl_->UpdateConfig(config);
}

DegradationConfig GracefulDegradationManager::GetConfig() const {
    return impl_->GetConfig();
}

} // namespace Core::Multiplayer