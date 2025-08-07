// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "error_handling.h"
#include <algorithm>
#include <thread>
#include <iostream>

namespace Core::Multiplayer {

// Error code to category mapping
static ErrorCategory GetErrorCategory(ErrorCode code) {
    // Network errors (1000-1999)
    if (code >= ErrorCode::NetworkTimeout && code <= ErrorCode::SSLError) {
        return ErrorCategory::NetworkConnectivity;
    }
    
    // Permission errors (2000-2099)
    if (code >= ErrorCode::PermissionDenied && code <= ErrorCode::PermissionDenied) {
        return ErrorCategory::PermissionDenied;
    }
    
    // Configuration errors (2100-2199)
    if (code >= ErrorCode::ConfigurationInvalid && code <= ErrorCode::ConfigurationMissing) {
        return ErrorCategory::ConfigurationError;
    }
    
    // Protocol errors (2200-2299)
    if (code >= ErrorCode::ProtocolError && code <= ErrorCode::ProtocolError) {
        return ErrorCategory::ProtocolMismatch;
    }
    
    // Resource errors (2300-2399)
    if (code >= ErrorCode::ResourceExhausted && code <= ErrorCode::ResourceExhausted) {
        return ErrorCategory::ResourceExhausted;
    }
    
    // Security errors (2400-2499)
    if (code >= ErrorCode::AuthenticationFailed && code <= ErrorCode::AuthenticationFailed) {
        return ErrorCategory::SecurityViolation;
    }
    
    // Platform errors (4000-4999)
    if (code >= ErrorCode::PlatformAPIError && code <= ErrorCode::PlatformPermissionDenied) {
        return ErrorCategory::HardwareLimitation;
    }
    
    return ErrorCategory::Unknown;
}

// Default error messages
static std::string GetDefaultErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::NetworkTimeout:
            return "Unable to connect to multiplayer service";
        case ErrorCode::ConnectionRefused:
            return "Connection was refused by the server";
        case ErrorCode::HostUnreachable:
            return "Cannot reach the host";
        case ErrorCode::ConnectionLost:
            return "Lost connection to the server";
        case ErrorCode::InvalidResponse:
            return "Received invalid response from server";
        case ErrorCode::SSLError:
            return "Secure connection failed";
        case ErrorCode::PermissionDenied:
            return "Permission required for this operation";
        case ErrorCode::InvalidParameter:
            return "Invalid parameter provided";
        case ErrorCode::InvalidState:
            return "Operation not allowed in current state";
        case ErrorCode::NotSupported:
            return "This feature is not supported";
        case ErrorCode::ResourceExhausted:
            return "Resource limit reached";
        case ErrorCode::AuthenticationFailed:
            return "Authentication failed";
        case ErrorCode::PlatformAPIError:
            return "Platform API error occurred";
        case ErrorCode::ConfigurationInvalid:
            return "Invalid configuration";
        case ErrorCode::ConfigurationMissing:
            return "Required configuration missing";
        default:
            return "An error occurred";
    }
}

// Implementation class
class ErrorHandler::Impl {
public:
    Impl() : max_history_size_(100), auto_recovery_enabled_(true) {
        InitializeDefaultNotificationLevels();
    }

    void ReportError(const ErrorInfo& error) {
        ErrorCallback on_error_copy;
        bool auto_recovery_copy;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            // Log the error
            std::cerr << "[" << error.component << "] Error "
                      << static_cast<int>(error.error_code) << ": " << error.message << std::endl;

            // Add to history
            error_history_.push_back(error);
            if (error_history_.size() > max_history_size_) {
                error_history_.erase(error_history_.begin());
            }

            // Update statistics
            error_stats_[error.error_code]++;

            // Copy state needed after releasing the lock
            on_error_copy = on_error_;
            auto_recovery_copy = auto_recovery_enabled_;
        }

        // Trigger callbacks outside the lock
        if (on_error_copy) {
            on_error_copy(error);
        }

        // Show notification outside the lock
        ShowNotification(error);

        // Attempt auto-recovery outside the lock if enabled
        if (auto_recovery_copy) {
            AttemptRecovery(error);
        }
    }

    void ShowNotification(const ErrorInfo& error) {
        if (!notification_callback_) {
            return;
        }
        
        auto level_it = notification_levels_.find(error.error_code);
        NotificationLevel level = (level_it != notification_levels_.end()) 
            ? level_it->second 
            : GetDefaultNotificationLevel(error.category);
        
        NotificationData notification;
        notification.level = level;
        notification.title = GetErrorCategoryTitle(error.category);
        notification.message = error.message;
        notification.actions = error.suggested_actions;
        
        // Set auto-dismiss time based on level
        switch (level) {
            case NotificationLevel::Info:
                notification.auto_dismiss_ms = 3000;
                break;
            case NotificationLevel::Warning:
                notification.auto_dismiss_ms = 10000;
                break;
            default:
                // Error and Critical don't auto-dismiss
                break;
        }
        
        notification_callback_(notification);
    }

    bool AttemptRecovery(const ErrorInfo& error) {
        auto it = recovery_strategies_.find(error.category);
        if (it == recovery_strategies_.end()) {
            return false;
        }
        
        for (const auto& strategy : it->second) {
            if (strategy->CanRecover(error)) {
                strategy->AttemptRecovery(error, 
                    [this, error](bool success) {
                        if (success && on_recovery_success_) {
                            on_recovery_success_(error);
                        } else if (!success && on_recovery_failure_) {
                            on_recovery_failure_(error);
                        }
                    });
                return true;
            }
        }
        
        return false;
    }

    void RegisterRecoveryStrategy(ErrorCategory category, 
                                 std::unique_ptr<IErrorRecoveryStrategy> strategy) {
        std::lock_guard<std::mutex> lock(mutex_);
        recovery_strategies_[category].push_back(std::move(strategy));
    }

    std::vector<ErrorInfo> GetRecentErrors(size_t count) const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t start = error_history_.size() > count 
            ? error_history_.size() - count 
            : 0;
        return std::vector<ErrorInfo>(
            error_history_.begin() + start, 
            error_history_.end()
        );
    }

    std::unordered_map<ErrorCode, size_t> GetErrorStatistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return error_stats_;
    }

    void ClearErrorHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_history_.clear();
        error_stats_.clear();
    }

    // Setters
    void SetMaxErrorHistorySize(size_t size) { max_history_size_ = size; }
    void SetAutoRecoveryEnabled(bool enabled) { auto_recovery_enabled_ = enabled; }
    void SetNotificationLevel(ErrorCode code, NotificationLevel level) {
        notification_levels_[code] = level;
    }
    void SetNotificationCallback(NotificationCallback callback) {
        notification_callback_ = callback;
    }
    void SetOnError(ErrorCallback callback) { on_error_ = callback; }
    void SetOnRecoverySuccess(ErrorCallback callback) { on_recovery_success_ = callback; }
    void SetOnRecoveryFailure(ErrorCallback callback) { on_recovery_failure_ = callback; }
    
    // Context management functions
    void SetErrorContext(ErrorCode code, const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_contexts_[code][key] = value;
    }
    
    void SetRetryDelay(ErrorCode code, uint32_t seconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        retry_delays_[code] = seconds;
    }
    
    void AddSuggestedAction(ErrorCode code, const std::string& action) {
        std::lock_guard<std::mutex> lock(mutex_);
        suggested_actions_[code].push_back(action);
    }
    
    void ApplyStoredContext(ErrorInfo& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Apply stored context
        auto context_it = error_contexts_.find(error.error_code);
        if (context_it != error_contexts_.end()) {
            for (const auto& [key, value] : context_it->second) {
                error.context[key] = value;
            }
        }
        
        // Apply retry delay
        auto retry_it = retry_delays_.find(error.error_code);
        if (retry_it != retry_delays_.end()) {
            error.retry_after_seconds = retry_it->second;
        }
        
        // Apply suggested actions
        auto actions_it = suggested_actions_.find(error.error_code);
        if (actions_it != suggested_actions_.end()) {
            error.suggested_actions.insert(error.suggested_actions.end(),
                                         actions_it->second.begin(),
                                         actions_it->second.end());
        }
    }

private:
    void InitializeDefaultNotificationLevels() {
        // Network errors - usually warnings
        notification_levels_[ErrorCode::NetworkTimeout] = NotificationLevel::Warning;
        notification_levels_[ErrorCode::ConnectionLost] = NotificationLevel::Warning;
        
        // Permission errors - require user action
        notification_levels_[ErrorCode::PermissionDenied] = NotificationLevel::Error;
        
        // Security errors - critical
        notification_levels_[ErrorCode::AuthenticationFailed] = NotificationLevel::Critical;
        
        // Configuration errors - errors
        notification_levels_[ErrorCode::ConfigurationInvalid] = NotificationLevel::Error;
    }

    NotificationLevel GetDefaultNotificationLevel(ErrorCategory category) {
        switch (category) {
            case ErrorCategory::NetworkConnectivity:
                return NotificationLevel::Warning;
            case ErrorCategory::PermissionDenied:
            case ErrorCategory::ConfigurationError:
                return NotificationLevel::Error;
            case ErrorCategory::SecurityViolation:
                return NotificationLevel::Critical;
            default:
                return NotificationLevel::Info;
        }
    }

    std::string GetErrorCategoryTitle(ErrorCategory category) {
        switch (category) {
            case ErrorCategory::NetworkConnectivity:
                return "Network Error";
            case ErrorCategory::PermissionDenied:
                return "Permission Required";
            case ErrorCategory::ConfigurationError:
                return "Configuration Error";
            case ErrorCategory::ProtocolMismatch:
                return "Version Mismatch";
            case ErrorCategory::ResourceExhausted:
                return "Resource Limit";
            case ErrorCategory::SecurityViolation:
                return "Security Error";
            case ErrorCategory::HardwareLimitation:
                return "Hardware Limitation";
            default:
                return "Error";
        }
    }

    mutable std::mutex mutex_;
    std::vector<ErrorInfo> error_history_;
    std::unordered_map<ErrorCode, size_t> error_stats_;
    std::unordered_map<ErrorCategory, std::vector<std::unique_ptr<IErrorRecoveryStrategy>>> recovery_strategies_;
    std::unordered_map<ErrorCode, NotificationLevel> notification_levels_;
    
    // Context management storage
    std::unordered_map<ErrorCode, std::unordered_map<std::string, std::string>> error_contexts_;
    std::unordered_map<ErrorCode, uint32_t> retry_delays_;
    std::unordered_map<ErrorCode, std::vector<std::string>> suggested_actions_;
    
    size_t max_history_size_;
    bool auto_recovery_enabled_;
    
    NotificationCallback notification_callback_;
    ErrorCallback on_error_;
    ErrorCallback on_recovery_success_;
    ErrorCallback on_recovery_failure_;
};

// ErrorHandler implementation
ErrorHandler::ErrorHandler() : impl_(std::make_unique<Impl>()) {}
ErrorHandler::~ErrorHandler() = default;

void ErrorHandler::ReportError(ErrorCode code, const std::string& message, 
                              const std::string& component) {
    ErrorInfo error;
    error.category = GetErrorCategory(code);
    error.error_code = code;
    error.message = message.empty() ? GetDefaultErrorMessage(code) : message;
    error.component = component;
    error.timestamp = std::chrono::steady_clock::now();
    
    // Apply stored context
    impl_->ApplyStoredContext(error);
    
    impl_->ReportError(error);
}

void ErrorHandler::ReportError(const ErrorInfo& error) {
    impl_->ReportError(error);
}

void ErrorHandler::SetErrorContext(ErrorCode code, const std::string& key, 
                                  const std::string& value) {
    impl_->SetErrorContext(code, key, value);
}

void ErrorHandler::SetRetryDelay(ErrorCode code, uint32_t seconds) {
    impl_->SetRetryDelay(code, seconds);
}

void ErrorHandler::AddSuggestedAction(ErrorCode code, const std::string& action) {
    impl_->AddSuggestedAction(code, action);
}

void ErrorHandler::RegisterRecoveryStrategy(ErrorCategory category, 
                                           std::unique_ptr<IErrorRecoveryStrategy> strategy) {
    impl_->RegisterRecoveryStrategy(category, std::move(strategy));
}

bool ErrorHandler::AttemptRecovery(const ErrorInfo& error) {
    return impl_->AttemptRecovery(error);
}

void ErrorHandler::SetNotificationCallback(NotificationCallback callback) {
    impl_->SetNotificationCallback(callback);
}

void ErrorHandler::ShowNotification(const ErrorInfo& error) {
    impl_->ShowNotification(error);
}

void ErrorHandler::DismissNotification(const std::string& notification_id) {
    // For minimal implementation, we don't track notification IDs
    // This would be implemented when UI integration is added
}

std::vector<ErrorInfo> ErrorHandler::GetRecentErrors(size_t count) const {
    return impl_->GetRecentErrors(count);
}

std::unordered_map<ErrorCode, size_t> ErrorHandler::GetErrorStatistics() const {
    return impl_->GetErrorStatistics();
}

void ErrorHandler::ClearErrorHistory() {
    impl_->ClearErrorHistory();
}

void ErrorHandler::SetMaxErrorHistorySize(size_t size) {
    impl_->SetMaxErrorHistorySize(size);
}

void ErrorHandler::SetAutoRecoveryEnabled(bool enabled) {
    impl_->SetAutoRecoveryEnabled(enabled);
}

void ErrorHandler::SetNotificationLevel(ErrorCode code, NotificationLevel level) {
    impl_->SetNotificationLevel(code, level);
}

void ErrorHandler::SetOnError(ErrorCallback callback) {
    impl_->SetOnError(callback);
}

void ErrorHandler::SetOnRecoverySuccess(ErrorCallback callback) {
    impl_->SetOnRecoverySuccess(callback);
}

void ErrorHandler::SetOnRecoveryFailure(ErrorCallback callback) {
    impl_->SetOnRecoveryFailure(callback);
}

// NetworkRetryStrategy implementation
NetworkRetryStrategy::NetworkRetryStrategy(uint32_t max_retries, uint32_t initial_delay_ms)
    : max_retries_(max_retries), initial_delay_ms_(initial_delay_ms) {}

bool NetworkRetryStrategy::CanRecover(const ErrorInfo& error) const {
    if (error.category != ErrorCategory::NetworkConnectivity) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = retry_counts_.find(error.error_code);
    uint32_t current_retries = (it != retry_counts_.end()) ? it->second : 0;
    
    return current_retries < max_retries_;
}

void NetworkRetryStrategy::AttemptRecovery(const ErrorInfo& error, 
                                          std::function<void(bool)> callback) {
    std::thread([this, error, callback]() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            retry_counts_[error.error_code]++;
        }
        
        // Calculate exponential backoff
        uint32_t retry_count = retry_counts_[error.error_code];
        uint32_t delay_ms = initial_delay_ms_ * (1 << (retry_count - 1));
        
        // Honor retry_after_seconds if specified
        if (error.retry_after_seconds.has_value()) {
            delay_ms = error.retry_after_seconds.value() * 1000;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        
        // In a real implementation, this would retry the network operation
        // For now, we'll simulate a 50% success rate
        bool success = (std::rand() % 2) == 0;
        
        if (success) {
            std::lock_guard<std::mutex> lock(mutex_);
            retry_counts_.erase(error.error_code);
        }
        
        callback(success);
    }).detach();
}

// PermissionRequestStrategy implementation
PermissionRequestStrategy::PermissionRequestStrategy(PermissionRequestFunc request_func)
    : request_permission_(request_func) {}

bool PermissionRequestStrategy::CanRecover(const ErrorInfo& error) const {
    return error.category == ErrorCategory::PermissionDenied && request_permission_;
}

void PermissionRequestStrategy::AttemptRecovery(const ErrorInfo& error, 
                                               std::function<void(bool)> callback) {
    // Extract permission name from error context
    std::string permission_name = "unknown";
    auto it = error.context.find("permission");
    if (it != error.context.end()) {
        permission_name = it->second;
    }
    
    request_permission_(permission_name, callback);
}

// FallbackModeStrategy implementation
FallbackModeStrategy::FallbackModeStrategy(ModeSwitchFunc switch_func)
    : switch_mode_(switch_func) {}

bool FallbackModeStrategy::CanRecover(const ErrorInfo& error) const {
    return (error.category == ErrorCategory::HardwareLimitation ||
            error.category == ErrorCategory::NetworkConnectivity) && 
           switch_mode_;
}

void FallbackModeStrategy::AttemptRecovery(const ErrorInfo& error, 
                                          std::function<void(bool)> callback) {
    // Determine fallback mode based on error
    std::string fallback_mode = "internet"; // Default fallback
    
    if (error.error_code == ErrorCode::PlatformFeatureUnavailable) {
        auto it = error.context.find("current_mode");
        if (it != error.context.end() && it->second == "adhoc") {
            fallback_mode = "internet";
        }
    }
    
    bool success = switch_mode_(fallback_mode);
    callback(success);
}

// Helper functions
ErrorInfo CreateNetworkError(ErrorCode code, const std::string& message) {
    ErrorInfo error;
    error.category = ErrorCategory::NetworkConnectivity;
    error.error_code = code;
    error.message = message;
    error.timestamp = std::chrono::steady_clock::now();
    error.component = "Network";
    error.retry_after_seconds = 5; // Default retry after 5 seconds
    error.suggested_actions = {"Check your internet connection", "Try again later"};
    return error;
}

ErrorInfo CreatePermissionError(ErrorCode code, const std::string& permission) {
    ErrorInfo error;
    error.category = ErrorCategory::PermissionDenied;
    error.error_code = code;
    error.message = "Permission required: " + permission;
    error.timestamp = std::chrono::steady_clock::now();
    error.component = "Permissions";
    error.context["permission"] = permission;
    error.suggested_actions = {"Grant permission in settings", "Check app permissions"};
    return error;
}

ErrorInfo CreateConfigurationError(ErrorCode code, const std::string& setting) {
    ErrorInfo error;
    error.category = ErrorCategory::ConfigurationError;
    error.error_code = code;
    error.message = "Invalid configuration: " + setting;
    error.timestamp = std::chrono::steady_clock::now();
    error.component = "Configuration";
    error.context["setting"] = setting;
    error.suggested_actions = {"Check multiplayer settings", "Reset to defaults"};
    return error;
}

ErrorInfo CreateSecurityError(ErrorCode code, const std::string& violation) {
    ErrorInfo error;
    error.category = ErrorCategory::SecurityViolation;
    error.error_code = code;
    error.message = "Security violation: " + violation;
    error.timestamp = std::chrono::steady_clock::now();
    error.component = "Security";
    error.suggested_actions = {"Re-authenticate", "Contact support"};
    return error;
}

// Global instance
ErrorHandler& GetGlobalErrorHandler() {
    static ErrorHandler instance;
    return instance;
}

} // namespace Core::Multiplayer