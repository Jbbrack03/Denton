// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "circuit_breaker.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <spdlog/spdlog.h>

namespace Core::Multiplayer {

// Implementation class using PIMPL pattern
class CircuitBreaker::Impl {
public:
    explicit Impl(const CircuitBreakerConfig& config)
        : config_(config), state_(CircuitBreakerState::Closed), 
          consecutive_failures_(0), listener_(nullptr) {
        
        last_state_change_ = std::chrono::steady_clock::now();
        
        // Initialize metrics
        metrics_.total_requests = 0;
        metrics_.successful_requests = 0;
        metrics_.failed_requests = 0;
        metrics_.rejected_requests = 0;
        metrics_.consecutive_failures = 0;
        metrics_.success_rate = 0.0;
        metrics_.failure_rate = 0.0;
        metrics_.average_response_time_ms = 0.0;
        metrics_.min_response_time_ms = 0.0;
        metrics_.max_response_time_ms = 0.0;
        metrics_.last_state_change = last_state_change_;
        metrics_.time_in_current_state = std::chrono::milliseconds(0);
    }

    ~Impl() = default;

    ErrorCode ExecuteInternal(std::function<ErrorCode()> operation) {
        auto start_time = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lock(mutex_);

        // Check if circuit is open
        if (state_ == CircuitBreakerState::Open) {
            // Check if enough time has passed to transition to half-open
            auto time_since_open = std::chrono::duration_cast<std::chrono::milliseconds>(
                start_time - last_state_change_);

            if (time_since_open.count() >= config_.timeout_duration_ms) {
                TransitionToHalfOpen();
            } else {
                metrics_.rejected_requests++;
                return ErrorCode::ServiceUnavailable;
            }
        }

        // If half-open, only allow limited concurrent calls
        if (state_ == CircuitBreakerState::HalfOpen) {
            if (half_open_calls_ >= config_.max_concurrent_half_open_calls) {
                metrics_.rejected_requests++;
                return ErrorCode::ServiceUnavailable;
            }
            half_open_calls_++;
        }

        metrics_.total_requests++;

        // Release the lock while executing the user operation
        lock.unlock();

        ErrorCode result;
        try {
            result = operation();
        } catch (...) {
            result = ErrorCode::InternalError;
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Reacquire lock to update metrics and state
        lock.lock();

        // Update response time metrics
        UpdateResponseTimeMetrics(duration.count());

        // Handle result
        if (result == ErrorCode::Success) {
            OnSuccess();
        } else {
            OnFailure(result);
        }

        if (state_ == CircuitBreakerState::HalfOpen) {
            half_open_calls_--;
        }

        return result;
    }

    CircuitBreakerState GetState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    void ForceOpen() {
        std::lock_guard<std::mutex> lock(mutex_);
        TransitionTo(CircuitBreakerState::Open);
    }

    void ForceHalfOpen() {
        std::lock_guard<std::mutex> lock(mutex_);
        TransitionToHalfOpen();
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        consecutive_failures_ = 0;
        metrics_.consecutive_failures = 0;
        TransitionTo(CircuitBreakerState::Closed);
    }

    CircuitBreakerMetrics GetMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        CircuitBreakerMetrics current_metrics = metrics_;
        
        // Update time in current state
        auto now = std::chrono::steady_clock::now();
        current_metrics.time_in_current_state = 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_state_change_);
        
        // Calculate rates
        if (current_metrics.total_requests > 0) {
            current_metrics.success_rate = 
                static_cast<double>(current_metrics.successful_requests) / current_metrics.total_requests;
            current_metrics.failure_rate = 
                static_cast<double>(current_metrics.failed_requests) / current_metrics.total_requests;
        }
        
        return current_metrics;
    }

    std::string ExportMetricsAsJSON() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ostringstream json;
        json << "{\n";
        json << "  \"state\": \"" << StateToString(state_) << "\",\n";
        json << "  \"total_requests\": " << metrics_.total_requests << ",\n";
        json << "  \"successful_requests\": " << metrics_.successful_requests << ",\n";
        json << "  \"failed_requests\": " << metrics_.failed_requests << ",\n";
        json << "  \"rejected_requests\": " << metrics_.rejected_requests << ",\n";
        json << "  \"consecutive_failures\": " << metrics_.consecutive_failures << ",\n";
        json << "  \"success_rate\": " << metrics_.success_rate << ",\n";
        json << "  \"failure_rate\": " << metrics_.failure_rate << ",\n";
        json << "  \"average_response_time_ms\": " << metrics_.average_response_time_ms << ",\n";
        json << "  \"min_response_time_ms\": " << metrics_.min_response_time_ms << ",\n";
        json << "  \"max_response_time_ms\": " << metrics_.max_response_time_ms << "\n";
        json << "}";
        
        return json.str();
    }

    void SetListener(MockCircuitBreakerListener* listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listener_ = listener;
    }

    void UpdateConfig(const CircuitBreakerConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    CircuitBreakerConfig GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

private:
    // These functions expect mutex_ to be locked by the caller
    void OnSuccess() {
        metrics_.successful_requests++;
        consecutive_failures_ = 0;
        metrics_.consecutive_failures = 0;
        
        if (listener_) {
            if (spdlog::should_log(spdlog::level::info)) {
                spdlog::info("Circuit breaker: Success recorded");
            }
        }
        
        // If in half-open state and we've had enough successes, close the circuit
        if (state_ == CircuitBreakerState::HalfOpen && 
            metrics_.successful_requests >= config_.success_threshold_for_close) {
            TransitionTo(CircuitBreakerState::Closed);
        }
    }

    void OnFailure(ErrorCode error) {
        metrics_.failed_requests++;
        consecutive_failures_++;
        metrics_.consecutive_failures = consecutive_failures_;
        
        if (listener_) {
            if (spdlog::should_log(spdlog::level::warn)) {
                spdlog::warn("Circuit breaker: Failure recorded, error {}", static_cast<int>(error));
            }
        }
        
        // Check if we should open the circuit
        if ((state_ == CircuitBreakerState::Closed || state_ == CircuitBreakerState::HalfOpen) &&
            consecutive_failures_ >= config_.failure_threshold) {
            TransitionTo(CircuitBreakerState::Open);
        }
    }

    void TransitionTo(CircuitBreakerState new_state) {
        CircuitBreakerState old_state = state_;
        state_ = new_state;
        last_state_change_ = std::chrono::steady_clock::now();
        metrics_.last_state_change = last_state_change_;
        
        if (new_state == CircuitBreakerState::Open) {
            if (listener_) {
                if (spdlog::should_log(spdlog::level::warn)) {
                    spdlog::warn("Circuit breaker: Circuit opened after {} failures", consecutive_failures_);
                }
            }
        } else if (new_state == CircuitBreakerState::Closed) {
            if (listener_) {
                if (spdlog::should_log(spdlog::level::info)) {
                    spdlog::info("Circuit breaker: Circuit closed");
                }
            }
        } else if (new_state == CircuitBreakerState::HalfOpen) {
            half_open_calls_ = 0;
            if (listener_) {
                if (spdlog::should_log(spdlog::level::info)) {
                    spdlog::info("Circuit breaker: Half-open test started");
                }
            }
        }

        if (listener_) {
            if (spdlog::should_log(spdlog::level::info)) {
                spdlog::info("Circuit breaker: State changed from {} to {}",
                             StateToString(old_state), StateToString(new_state));
            }
        }

        if (spdlog::should_log(spdlog::level::info)) {
            spdlog::info("Circuit breaker state changed from {} to {}",
                         StateToString(old_state), StateToString(new_state));
        }
    }

    void TransitionToHalfOpen() {
        TransitionTo(CircuitBreakerState::HalfOpen);
    }

    void UpdateResponseTimeMetrics(double response_time_ms) {
        if (metrics_.total_requests == 1) {
            metrics_.min_response_time_ms = response_time_ms;
            metrics_.max_response_time_ms = response_time_ms;
            metrics_.average_response_time_ms = response_time_ms;
        } else {
            metrics_.min_response_time_ms = std::min(metrics_.min_response_time_ms, response_time_ms);
            metrics_.max_response_time_ms = std::max(metrics_.max_response_time_ms, response_time_ms);
            
            // Update running average
            double total_time = metrics_.average_response_time_ms * (metrics_.total_requests - 1);
            metrics_.average_response_time_ms = (total_time + response_time_ms) / metrics_.total_requests;
        }
    }

    std::string StateToString(CircuitBreakerState state) const {
        switch (state) {
            case CircuitBreakerState::Closed: return "Closed";
            case CircuitBreakerState::Open: return "Open";
            case CircuitBreakerState::HalfOpen: return "HalfOpen";
            default: return "Unknown";
        }
    }

    mutable std::mutex mutex_;
    CircuitBreakerConfig config_;
    CircuitBreakerState state_;
    size_t consecutive_failures_;
    std::chrono::steady_clock::time_point last_state_change_;
    CircuitBreakerMetrics metrics_;
    MockCircuitBreakerListener* listener_;
    size_t half_open_calls_ = 0;
};

// CircuitBreaker implementation
CircuitBreaker::CircuitBreaker(const CircuitBreakerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

CircuitBreaker::~CircuitBreaker() = default;

ErrorCode CircuitBreaker::ExecuteInternal(std::function<ErrorCode()> operation) {
    return impl_->ExecuteInternal(operation);
}

CircuitBreakerState CircuitBreaker::GetState() const {
    return impl_->GetState();
}

void CircuitBreaker::ForceOpen() {
    impl_->ForceOpen();
}

void CircuitBreaker::ForceHalfOpen() {
    impl_->ForceHalfOpen();
}

void CircuitBreaker::Reset() {
    impl_->Reset();
}

CircuitBreakerMetrics CircuitBreaker::GetMetrics() const {
    return impl_->GetMetrics();
}

std::string CircuitBreaker::ExportMetricsAsJSON() const {
    return impl_->ExportMetricsAsJSON();
}

void CircuitBreaker::SetListener(MockCircuitBreakerListener* listener) {
    impl_->SetListener(listener);
}

void CircuitBreaker::UpdateConfig(const CircuitBreakerConfig& config) {
    impl_->UpdateConfig(config);
}

CircuitBreakerConfig CircuitBreaker::GetConfig() const {
    return impl_->GetConfig();
}

} // namespace Core::Multiplayer
