// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connection_recovery_manager.h"
#include <thread>
#include <random>
#include <algorithm>
#include <iostream>
#include <atomic>

namespace Core::Multiplayer {

// Implementation class using PIMPL pattern
class ConnectionRecoveryManager::Impl {
public:
    Impl(const RecoveryConfig& config, MockNetworkConnection* connection)
        : config_(config), connection_(connection), state_(RecoveryState::Idle),
          current_attempt_(0), listener_(nullptr),
          rng_(std::random_device{}()), attempt_count_(0) {
        
        start_time_ = std::chrono::steady_clock::now();
        
        // Initialize default strategies for different error types
        InitializeDefaultStrategies();
    }

    ~Impl() {
        Shutdown();
    }

    ErrorCode StartRecovery(const ErrorInfo& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ == RecoveryState::InProgress) {
            return ErrorCode::InvalidState;
        }
        
        // Check if error is retryable
        if (!CanRetryError(error)) {
            return ErrorCode::NotSupported;
        }
        
        current_error_ = error;
        state_ = RecoveryState::InProgress;
        current_attempt_ = 0;
        start_time_ = std::chrono::steady_clock::now();
        
        // Start recovery process in background thread
        recovery_thread_ = std::thread(&Impl::PerformRecovery, this);
        
        // Log recovery start (placeholder for actual logging)
        std::cout << "Started recovery for error: " << static_cast<int>(error.error_code) << std::endl;
        return ErrorCode::Success;
    }

    ErrorCode StopRecovery() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ != RecoveryState::InProgress) {
            return ErrorCode::InvalidState;
        }
        
        state_ = RecoveryState::Aborted;
        
        if (recovery_thread_.joinable()) {
            recovery_thread_.join();
        }
        
        std::cout << "Recovery stopped" << std::endl;
        return ErrorCode::Success;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ == RecoveryState::InProgress) {
            state_ = RecoveryState::Aborted;
        }
        
        if (recovery_thread_.joinable()) {
            recovery_thread_.join();
        }
    }

    RecoveryState GetState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    RecoveryStatus GetRecoveryStatus() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        RecoveryStatus status;
        status.state = state_;
        status.current_attempt = current_attempt_;
        status.max_attempts = config_.max_retries;
        status.start_time = start_time_;
        
        auto now = std::chrono::steady_clock::now();
        status.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
        
        if (current_error_.has_value()) {
            status.last_error = current_error_;
            status.current_strategy = GetRecoveryStrategy(current_error_.value());
        } else {
            status.current_strategy = RecoveryStrategy::ExponentialBackoff;
        }
        
        return status;
    }

    bool CanRetryError(const ErrorInfo& error) const {
        // Non-retryable errors
        switch (error.error_code) {
            case ErrorCode::AuthenticationFailed:
            case ErrorCode::NotSupported:
            case ErrorCode::PermissionDenied:
            case ErrorCode::InvalidParameter:
                return false;
            default:
                return true;
        }
    }

    RecoveryStrategy GetRecoveryStrategy(const ErrorInfo& error) const {
        // Determine strategy based on error type
        switch (error.error_code) {
            case ErrorCode::NetworkTimeout:
            case ErrorCode::ConnectionLost:
                return RecoveryStrategy::ExponentialBackoff;
            case ErrorCode::ConnectionRefused:
                return RecoveryStrategy::ImmediateRetry;
            case ErrorCode::SSLError:
                return RecoveryStrategy::CertificateRetry;
            default:
                return RecoveryStrategy::ExponentialBackoff;
        }
    }

    void RegisterStrategy(ErrorCode error_code, MockRecoveryStrategy* strategy) {
        std::lock_guard<std::mutex> lock(mutex_);
        // For minimal implementation, just store the raw pointer
        custom_strategies_[error_code] = strategy;
    }

    void SetRecoveryListener(MockRecoveryListener* listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listener_ = listener;
    }

    void UpdateConfig(const RecoveryConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    RecoveryConfig GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

private:
    void InitializeDefaultStrategies() {
        // Initialize any default recovery strategies
        // For minimal implementation, we'll use built-in logic
    }

    void PerformRecovery() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (state_ == RecoveryState::Aborted) {
                    return;
                }
                
                current_attempt_++;
                
                // Check if exceeded max attempts
                if (current_attempt_ > config_.max_retries) {
                    state_ = RecoveryState::Failed;
                    NotifyRecoveryFailed();
                    return;
                }
                
                NotifyRecoveryAttempt();
            }
            
            // Calculate delay with exponential backoff and jitter
            uint32_t delay_ms = CalculateDelay();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            
            // Attempt to recover connection
            bool success = AttemptConnection();
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (success) {
                    state_ = RecoveryState::Succeeded;
                    NotifyRecoverySuccess();
                    return;
                }
                
                // Continue retry loop if not successful
            }
        }
    }

    uint32_t CalculateDelay() {
        uint32_t base_delay = config_.initial_delay_ms;
        
        // Apply exponential backoff
        for (size_t i = 1; i < current_attempt_; ++i) {
            base_delay = static_cast<uint32_t>(base_delay * config_.backoff_multiplier);
        }
        
        // Cap at maximum delay
        base_delay = std::min(base_delay, config_.max_delay_ms);
        
        // Apply jitter if enabled
        if (config_.jitter_enabled) {
            uint32_t jitter_range = (base_delay * config_.jitter_range_percent) / 100;
            std::uniform_int_distribution<uint32_t> dist(0, jitter_range);
            base_delay += dist(rng_);
        }
        
        return base_delay;
    }

    bool AttemptConnection() {
        if (!connection_) {
            return false;
        }
        
        // For minimal implementation, simulate connection attempts
        // In real implementation, this would call connection_->Connect()
        // For now, simulate 50% success rate for recovery attempts
        int attempt = ++attempt_count_;
        return (attempt % 2) == 0;
    }

    void NotifyRecoveryAttempt() {
        // For minimal implementation, just log the attempt
        // In real implementation, this would call listener_->OnRecoveryAttempt()
        if (listener_ && current_error_.has_value()) {
            std::cout << "Recovery attempt " << current_attempt_ << " for error " 
                      << static_cast<int>(current_error_.value().error_code) << std::endl;
        }
    }

    void NotifyRecoverySuccess() {
        // For minimal implementation, just log the success
        // In real implementation, this would call listener_->OnRecoverySucceeded()
        if (listener_ && current_error_.has_value()) {
            std::cout << "Recovery succeeded for error " 
                      << static_cast<int>(current_error_.value().error_code) << std::endl;
        }
    }

    void NotifyRecoveryFailed() {
        // For minimal implementation, just log the failure
        // In real implementation, this would call listener_->OnRecoveryFailed()
        if (listener_ && current_error_.has_value()) {
            std::cout << "Recovery failed for error " 
                      << static_cast<int>(current_error_.value().error_code) 
                      << " after " << current_attempt_ << " attempts" << std::endl;
        }
    }

    mutable std::mutex mutex_;
    RecoveryConfig config_;
    MockNetworkConnection* connection_;
    RecoveryState state_;
    size_t current_attempt_;
    std::chrono::steady_clock::time_point start_time_;
    std::optional<ErrorInfo> current_error_;
    MockRecoveryListener* listener_;
    std::thread recovery_thread_;
    std::unordered_map<ErrorCode, MockRecoveryStrategy*> custom_strategies_;
    std::mt19937 rng_;
    std::atomic<int> attempt_count_;
};

// ConnectionRecoveryManager implementation
ConnectionRecoveryManager::ConnectionRecoveryManager(const RecoveryConfig& config, 
                                                   MockNetworkConnection* connection)
    : impl_(std::make_unique<Impl>(config, connection)) {}

ConnectionRecoveryManager::~ConnectionRecoveryManager() = default;

ErrorCode ConnectionRecoveryManager::StartRecovery(const ErrorInfo& error) {
    return impl_->StartRecovery(error);
}

ErrorCode ConnectionRecoveryManager::StopRecovery() {
    return impl_->StopRecovery();
}

void ConnectionRecoveryManager::Shutdown() {
    impl_->Shutdown();
}

RecoveryState ConnectionRecoveryManager::GetState() const {
    return impl_->GetState();
}

RecoveryStatus ConnectionRecoveryManager::GetRecoveryStatus() const {
    return impl_->GetRecoveryStatus();
}

bool ConnectionRecoveryManager::CanRetryError(const ErrorInfo& error) const {
    return impl_->CanRetryError(error);
}

RecoveryStrategy ConnectionRecoveryManager::GetRecoveryStrategy(const ErrorInfo& error) const {
    return impl_->GetRecoveryStrategy(error);
}

void ConnectionRecoveryManager::RegisterStrategy(ErrorCode error_code, 
                                               MockRecoveryStrategy* strategy) {
    impl_->RegisterStrategy(error_code, strategy);
}

void ConnectionRecoveryManager::SetRecoveryListener(MockRecoveryListener* listener) {
    impl_->SetRecoveryListener(listener);
}

void ConnectionRecoveryManager::UpdateConfig(const RecoveryConfig& config) {
    impl_->UpdateConfig(config);
}

RecoveryConfig ConnectionRecoveryManager::GetConfig() const {
    return impl_->GetConfig();
}

} // namespace Core::Multiplayer