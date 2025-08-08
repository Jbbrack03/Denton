// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "core/multiplayer/common/error_handling.h"

using namespace Sudachi::Multiplayer;
using namespace testing;
using namespace std::chrono_literals;

/**
 * Test fixture for error handling framework
 */
class ErrorHandlingTest : public Test {
protected:
    void SetUp() override {
        // Create a fresh error handler for each test
        error_handler_ = std::make_unique<ErrorHandler>();
        
        // Clear any existing errors
        error_handler_->ClearErrorHistory();
        
        // Reset counters
        notification_count_ = 0;
        error_count_ = 0;
        recovery_success_count_ = 0;
        recovery_failure_count_ = 0;
    }

    void TearDown() override {
        error_handler_.reset();
    }

    // Helper to track notifications
    void SetupNotificationTracking() {
        error_handler_->SetNotificationCallback(
            [this](const NotificationData& notification) {
                std::lock_guard<std::mutex> lock(notification_mutex_);
                notifications_.push_back(notification);
                notification_count_++;
            });
    }

    // Helper to track errors
    void SetupErrorTracking() {
        error_handler_->SetOnError(
            [this](const ErrorInfo& error) {
                std::lock_guard<std::mutex> lock(error_mutex_);
                reported_errors_.push_back(error);
                error_count_++;
            });
    }

    // Helper to track recovery
    void SetupRecoveryTracking() {
        error_handler_->SetOnRecoverySuccess(
            [this](const ErrorInfo& error) {
                recovery_success_count_++;
            });
        
        error_handler_->SetOnRecoveryFailure(
            [this](const ErrorInfo& error) {
                recovery_failure_count_++;
            });
    }

protected:
    std::unique_ptr<ErrorHandler> error_handler_;
    
    // Tracking data
    std::vector<NotificationData> notifications_;
    std::vector<ErrorInfo> reported_errors_;
    std::atomic<int> notification_count_{0};
    std::atomic<int> error_count_{0};
    std::atomic<int> recovery_success_count_{0};
    std::atomic<int> recovery_failure_count_{0};
    
    std::mutex notification_mutex_;
    std::mutex error_mutex_;
};

/**
 * Test basic error reporting
 */
TEST_F(ErrorHandlingTest, BasicErrorReporting) {
    SetupErrorTracking();
    
    // Report a network error
    error_handler_->ReportError(ErrorCode::NetworkTimeout, 
                               "Connection timed out", 
                               "NetworkManager");
    
    // Wait for async processing
    std::this_thread::sleep_for(10ms);
    
    EXPECT_EQ(1, error_count_);
    ASSERT_EQ(1u, reported_errors_.size());
    
    const auto& error = reported_errors_[0];
    EXPECT_EQ(ErrorCode::NetworkTimeout, error.error_code);
    EXPECT_EQ("Connection timed out", error.message);
    EXPECT_EQ("NetworkManager", error.component);
    EXPECT_EQ(ErrorCategory::NetworkConnectivity, error.category);
}

/**
 * Test error categorization
 */
TEST_F(ErrorHandlingTest, ErrorCategorization) {
    struct TestCase {
        ErrorCode code;
        ErrorCategory expected_category;
    };
    
    std::vector<TestCase> test_cases = {
        {ErrorCode::NetworkTimeout, ErrorCategory::NetworkConnectivity},
        {ErrorCode::ConnectionRefused, ErrorCategory::NetworkConnectivity},
        {ErrorCode::PermissionDenied, ErrorCategory::PermissionDenied},
        {ErrorCode::ConfigurationInvalid, ErrorCategory::ConfigurationError},
        {ErrorCode::ProtocolError, ErrorCategory::ProtocolMismatch},
        {ErrorCode::ResourceExhausted, ErrorCategory::ResourceExhausted},
        {ErrorCode::AuthenticationFailed, ErrorCategory::SecurityViolation},
        {ErrorCode::PlatformAPIError, ErrorCategory::HardwareLimitation},
    };
    
    SetupErrorTracking();
    
    for (const auto& test : test_cases) {
        error_handler_->ReportError(test.code, "Test error");
    }
    
    std::this_thread::sleep_for(50ms);
    
    ASSERT_EQ(test_cases.size(), reported_errors_.size());
    
    for (size_t i = 0; i < test_cases.size(); ++i) {
        EXPECT_EQ(test_cases[i].expected_category, reported_errors_[i].category)
            << "Failed for error code: " << static_cast<int>(test_cases[i].code);
    }
}

/**
 * Test notification levels
 */
TEST_F(ErrorHandlingTest, NotificationLevels) {
    SetupNotificationTracking();
    
    // Set custom notification levels
    error_handler_->SetNotificationLevel(ErrorCode::NetworkTimeout, NotificationLevel::Info);
    error_handler_->SetNotificationLevel(ErrorCode::PermissionDenied, NotificationLevel::Error);
    error_handler_->SetNotificationLevel(ErrorCode::AuthenticationFailed, NotificationLevel::Critical);
    
    // Report errors
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Timeout");
    error_handler_->ReportError(ErrorCode::PermissionDenied, "No permission");
    error_handler_->ReportError(ErrorCode::AuthenticationFailed, "Auth failed");
    
    std::this_thread::sleep_for(50ms);
    
    ASSERT_EQ(3u, notifications_.size());
    
    EXPECT_EQ(NotificationLevel::Info, notifications_[0].level);
    EXPECT_EQ(NotificationLevel::Error, notifications_[1].level);
    EXPECT_EQ(NotificationLevel::Critical, notifications_[2].level);
    
    // Check auto-dismiss times
    EXPECT_TRUE(notifications_[0].auto_dismiss_ms.has_value());
    EXPECT_EQ(3000u, notifications_[0].auto_dismiss_ms.value());
    
    EXPECT_FALSE(notifications_[1].auto_dismiss_ms.has_value()); // Error doesn't auto-dismiss
    EXPECT_FALSE(notifications_[2].auto_dismiss_ms.has_value()); // Critical doesn't auto-dismiss
}

/**
 * Test error history
 */
TEST_F(ErrorHandlingTest, ErrorHistory) {
    // Set small history size
    error_handler_->SetMaxErrorHistorySize(5);
    
    // Report 10 errors
    for (int i = 0; i < 10; ++i) {
        error_handler_->ReportError(ErrorCode::NetworkTimeout, 
                                   "Error " + std::to_string(i));
        std::this_thread::sleep_for(5ms);
    }
    
    // Get recent errors
    auto recent = error_handler_->GetRecentErrors(10);
    
    // Should only have last 5
    ASSERT_EQ(5u, recent.size());
    
    // Verify we have the last 5 errors
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ("Error " + std::to_string(i + 5), recent[i].message);
    }
    
    // Test clear history
    error_handler_->ClearErrorHistory();
    recent = error_handler_->GetRecentErrors(10);
    EXPECT_EQ(0u, recent.size());
}

/**
 * Test error statistics
 */
TEST_F(ErrorHandlingTest, ErrorStatistics) {
    // Report various errors
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Timeout 1");
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Timeout 2");
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Timeout 3");
    error_handler_->ReportError(ErrorCode::ConnectionRefused, "Refused");
    error_handler_->ReportError(ErrorCode::PermissionDenied, "Permission");
    
    std::this_thread::sleep_for(50ms);
    
    auto stats = error_handler_->GetErrorStatistics();
    
    EXPECT_EQ(3u, stats[ErrorCode::NetworkTimeout]);
    EXPECT_EQ(1u, stats[ErrorCode::ConnectionRefused]);
    EXPECT_EQ(1u, stats[ErrorCode::PermissionDenied]);
}

/**
 * Test network retry recovery strategy
 */
TEST_F(ErrorHandlingTest, NetworkRetryStrategy) {
    SetupRecoveryTracking();
    
    // Register retry strategy
    auto retry_strategy = std::make_unique<NetworkRetryStrategy>(3, 10);
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::NetworkConnectivity, 
                                            std::move(retry_strategy));
    
    // Report network error
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Connection failed");
    
    // Wait for recovery attempt
    std::this_thread::sleep_for(100ms);
    
    // Should have attempted recovery (success is random)
    EXPECT_GT(recovery_success_count_ + recovery_failure_count_, 0);
}

/**
 * Test permission request strategy
 */
TEST_F(ErrorHandlingTest, PermissionRequestStrategy) {
    SetupRecoveryTracking();
    
    bool permission_requested = false;
    std::string requested_permission;
    
    // Create permission strategy with mock request function
    auto permission_strategy = std::make_unique<PermissionRequestStrategy>(
        [&permission_requested, &requested_permission](const std::string& permission, 
                                                      std::function<void(bool)> callback) {
            permission_requested = true;
            requested_permission = permission;
            callback(true); // Simulate permission granted
        });
    
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::PermissionDenied, 
                                            std::move(permission_strategy));
    
    // Report permission error
    auto error = CreatePermissionError(ErrorCode::PermissionDenied, "WIFI");
    error_handler_->ReportError(error);
    
    std::this_thread::sleep_for(50ms);
    
    EXPECT_TRUE(permission_requested);
    EXPECT_EQ("WIFI", requested_permission);
    EXPECT_EQ(1, recovery_success_count_);
}

/**
 * Test fallback mode strategy
 */
TEST_F(ErrorHandlingTest, FallbackModeStrategy) {
    SetupRecoveryTracking();
    
    bool mode_switched = false;
    std::string target_mode;
    
    // Create fallback strategy
    auto fallback_strategy = std::make_unique<FallbackModeStrategy>(
        [&mode_switched, &target_mode](const std::string& mode) {
            mode_switched = true;
            target_mode = mode;
            return true;
        });
    
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::HardwareLimitation, 
                                            std::move(fallback_strategy));
    
    // Report hardware limitation error
    ErrorInfo error;
    error.category = ErrorCategory::HardwareLimitation;
    error.error_code = ErrorCode::PlatformFeatureUnavailable;
    error.message = "WiFi Direct not supported";
    error.context["current_mode"] = "adhoc";
    error.timestamp = std::chrono::steady_clock::now();
    
    error_handler_->ReportError(error);
    
    std::this_thread::sleep_for(50ms);
    
    EXPECT_TRUE(mode_switched);
    EXPECT_EQ("internet", target_mode);
    EXPECT_EQ(1, recovery_success_count_);
}

/**
 * Test auto-recovery disable
 */
TEST_F(ErrorHandlingTest, AutoRecoveryDisabled) {
    SetupRecoveryTracking();
    
    // Disable auto-recovery
    error_handler_->SetAutoRecoveryEnabled(false);
    
    // Register recovery strategy
    auto retry_strategy = std::make_unique<NetworkRetryStrategy>(3, 10);
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::NetworkConnectivity, 
                                            std::move(retry_strategy));
    
    // Report error
    error_handler_->ReportError(ErrorCode::NetworkTimeout, "Connection failed");
    
    std::this_thread::sleep_for(50ms);
    
    // Should not have attempted recovery
    EXPECT_EQ(0, recovery_success_count_);
    EXPECT_EQ(0, recovery_failure_count_);
}

/**
 * Test error helper functions
 */
TEST_F(ErrorHandlingTest, ErrorHelperFunctions) {
    // Test network error helper
    auto network_error = CreateNetworkError(ErrorCode::NetworkTimeout, "Custom network error");
    EXPECT_EQ(ErrorCategory::NetworkConnectivity, network_error.category);
    EXPECT_EQ(ErrorCode::NetworkTimeout, network_error.error_code);
    EXPECT_EQ("Custom network error", network_error.message);
    EXPECT_TRUE(network_error.retry_after_seconds.has_value());
    EXPECT_EQ(5u, network_error.retry_after_seconds.value());
    EXPECT_FALSE(network_error.suggested_actions.empty());
    
    // Test permission error helper
    auto permission_error = CreatePermissionError(ErrorCode::PermissionDenied, "LOCATION");
    EXPECT_EQ(ErrorCategory::PermissionDenied, permission_error.category);
    EXPECT_EQ("Permission required: LOCATION", permission_error.message);
    EXPECT_EQ("LOCATION", permission_error.context.at("permission"));
    
    // Test configuration error helper
    auto config_error = CreateConfigurationError(ErrorCode::ConfigurationInvalid, "max_players");
    EXPECT_EQ(ErrorCategory::ConfigurationError, config_error.category);
    EXPECT_EQ("Invalid configuration: max_players", config_error.message);
    EXPECT_EQ("max_players", config_error.context.at("setting"));
    
    // Test security error helper
    auto security_error = CreateSecurityError(ErrorCode::AuthenticationFailed, "Invalid token");
    EXPECT_EQ(ErrorCategory::SecurityViolation, security_error.category);
    EXPECT_EQ("Security violation: Invalid token", security_error.message);
}

/**
 * Test concurrent error reporting
 */
TEST_F(ErrorHandlingTest, ConcurrentErrorReporting) {
    SetupErrorTracking();
    
    const int num_threads = 10;
    const int errors_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    // Start threads that report errors concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, errors_per_thread]() {
            for (int j = 0; j < errors_per_thread; ++j) {
                error_handler_->ReportError(
                    ErrorCode::NetworkTimeout,
                    "Thread " + std::to_string(i) + " Error " + std::to_string(j)
                );
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Give time for async processing
    std::this_thread::sleep_for(100ms);
    
    // Verify all errors were reported
    EXPECT_EQ(num_threads * errors_per_thread, error_count_);
    
    // Verify statistics
    auto stats = error_handler_->GetErrorStatistics();
    EXPECT_EQ(static_cast<size_t>(num_threads * errors_per_thread), 
              stats[ErrorCode::NetworkTimeout]);
}

/**
 * Test global error handler instance
 */
TEST_F(ErrorHandlingTest, GlobalErrorHandlerInstance) {
    auto& global1 = GetGlobalErrorHandler();
    auto& global2 = GetGlobalErrorHandler();
    
    // Should be the same instance
    EXPECT_EQ(&global1, &global2);
    
    // Test it works
    global1.ClearErrorHistory();
    global1.ReportError(ErrorCode::NetworkTimeout, "Global error");
    
    auto errors = global1.GetRecentErrors(1);
    ASSERT_EQ(1u, errors.size());
    EXPECT_EQ("Global error", errors[0].message);
}
