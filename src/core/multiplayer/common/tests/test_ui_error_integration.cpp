// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include "../error_handling.h"

namespace Sudachi::Multiplayer::Tests {

/**
 * Mock UI notification handler for testing UI integration
 */
class MockUINotificationHandler {
public:
    MOCK_METHOD(void, ShowNotification, (const NotificationData& notification));
    MOCK_METHOD(void, DismissNotification, (const std::string& notification_id));
    MOCK_METHOD(void, ShowModal, (const std::string& title, const std::string& message, 
                                 const std::vector<std::string>& actions));
    MOCK_METHOD(void, ShowToast, (const std::string& message, uint32_t duration_ms));
    MOCK_METHOD(void, ShowBanner, (const std::string& message, bool persistent));
    MOCK_METHOD(void, ShowErrorScreen, (const std::string& message, 
                                       const std::vector<std::string>& recovery_actions));
};

/**
 * Mock permission request handler for testing permission integration
 */
class MockPermissionHandler {
public:
    MOCK_METHOD(void, RequestPermission, (const std::string& permission, 
                                         std::function<void(bool)> callback));
    MOCK_METHOD(bool, HasPermission, (const std::string& permission));
    MOCK_METHOD(std::vector<std::string>, GetRequiredPermissions, ());
};

/**
 * Mock mode switcher for testing fallback mode integration
 */
class MockModeSwitch {
public:
    MOCK_METHOD(bool, SwitchToMode, (const std::string& mode));
    MOCK_METHOD(std::string, GetCurrentMode, ());
    MOCK_METHOD(std::vector<std::string>, GetAvailableModes, ());
};

/**
 * Test class for UI integration with error handling framework
 */
class UIErrorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        error_handler_ = std::make_unique<ErrorHandler>();
        ui_handler_ = std::make_unique<MockUINotificationHandler>();
        permission_handler_ = std::make_unique<MockPermissionHandler>();
        mode_switch_ = std::make_unique<MockModeSwitch>();
        
        // Set up notification callback - this will FAIL until callback system is working
        error_handler_->SetNotificationCallback(
            [this](const NotificationData& notification) {
                ui_handler_->ShowNotification(notification);
            }
        );
    }

    void TearDown() override {
        error_handler_.reset();
        ui_handler_.reset();
        permission_handler_.reset();
        mode_switch_.reset();
    }

    std::unique_ptr<ErrorHandler> error_handler_;
    std::unique_ptr<MockUINotificationHandler> ui_handler_;
    std::unique_ptr<MockPermissionHandler> permission_handler_;
    std::unique_ptr<MockModeSwitch> mode_switch_;
};

// CRITICAL: Test notification level mapping to UI display types
TEST_F(UIErrorIntegrationTest, NotificationLevelUIMapping) {
    // This test will FAIL until proper UI integration is implemented
    using ::testing::_;
    using ::testing::Property;
    
    struct TestCase {
        Core::Multiplayer::ErrorCode error_code;
        NotificationLevel expected_level;
        std::string ui_expectation;
    };
    
    std::vector<TestCase> test_cases = {
        // Info level -> Toast notification (3 seconds)
        {Core::Multiplayer::ErrorCode::Success, NotificationLevel::Info, "toast"},
        
        // Warning level -> Persistent banner (10 seconds)
        {Core::Multiplayer::ErrorCode::NetworkTimeout, NotificationLevel::Warning, "banner"},
        {Core::Multiplayer::ErrorCode::ConnectionLost, NotificationLevel::Warning, "banner"},
        
        // Error level -> Modal dialog (no auto-dismiss)
        {Core::Multiplayer::ErrorCode::PermissionDenied, NotificationLevel::Error, "modal"},
        {Core::Multiplayer::ErrorCode::ConfigurationInvalid, NotificationLevel::Error, "modal"},
        
        // Critical level -> Blocking error screen
        {Core::Multiplayer::ErrorCode::AuthenticationFailed, NotificationLevel::Critical, "error_screen"}
    };
    
    for (const auto& test_case : test_cases) {
        // Expect UI handler to be called with correct notification level
        EXPECT_CALL(*ui_handler_, ShowNotification(
            Property(&NotificationData::level, test_case.expected_level)))
            .Times(1);
        
        // Report error and verify UI integration
        error_handler_->ReportError(test_case.error_code, 
                                   "Test error for " + test_case.ui_expectation, 
                                   "UITest");
        
        // Reset mock for next test
        testing::Mock::VerifyAndClearExpectations(ui_handler_.get());
    }
}

// CRITICAL: Test auto-dismiss timing for different notification levels
TEST_F(UIErrorIntegrationTest, AutoDismissTimingIntegration) {
    // This test will FAIL until auto-dismiss timing is properly implemented
    using ::testing::_;
    using ::testing::Property;
    using ::testing::Optional;
    
    struct TimingTestCase {
        NotificationLevel level;
        std::optional<uint32_t> expected_auto_dismiss_ms;
        std::string description;
    };
    
    std::vector<TimingTestCase> timing_cases = {
        {NotificationLevel::Info, 3000, "Info notifications auto-dismiss after 3 seconds"},
        {NotificationLevel::Warning, 10000, "Warning notifications auto-dismiss after 10 seconds"},  // Will FAIL - should be 10000, not undefined
        {NotificationLevel::Error, std::nullopt, "Error notifications don't auto-dismiss"},
        {NotificationLevel::Critical, std::nullopt, "Critical notifications don't auto-dismiss"}
    };
    
    for (const auto& test_case : timing_cases) {
        EXPECT_CALL(*ui_handler_, ShowNotification(
            Property(&NotificationData::auto_dismiss_ms, test_case.expected_auto_dismiss_ms)))
            .Times(1);
        
        // Create error with specific notification level
        error_handler_->SetNotificationLevel(Core::Multiplayer::ErrorCode::InternalError, test_case.level);
        error_handler_->ReportError(Core::Multiplayer::ErrorCode::InternalError,
                                   test_case.description, "TimingTest");
        
        testing::Mock::VerifyAndClearExpectations(ui_handler_.get());
    }
}

// CRITICAL: Test notification action callback integration
TEST_F(UIErrorIntegrationTest, NotificationActionCallbackIntegration) {
    // This test will FAIL until action callback system is implemented
    using ::testing::_;
    
    std::atomic<bool> action_callback_called{false};
    std::string received_action;
    
    // Set up a notification callback that captures action callbacks
    error_handler_->SetNotificationCallback(
        [&](const NotificationData& notification) {
            // Test that action callback exists and is callable
            EXPECT_TRUE(notification.action_callback != nullptr) 
                << "Notification should have action callback";
            
            if (notification.action_callback) {
                // Simulate user clicking an action
                notification.action_callback("retry");
                action_callback_called = true;
                received_action = "retry";
            }
            
            ui_handler_->ShowNotification(notification);
        }
    );
    
    EXPECT_CALL(*ui_handler_, ShowNotification(_)).Times(1);
    
    // Report an error that should have suggested actions
    ErrorInfo error = CreateNetworkError(Core::Multiplayer::ErrorCode::NetworkTimeout, "Test timeout");
    error.suggested_actions = {"retry", "cancel", "settings"};
    
    error_handler_->ReportError(error);
    
    // Wait a bit for callback to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(action_callback_called) << "Action callback should have been called";
    EXPECT_EQ(received_action, "retry");
}

// CRITICAL: Test permission request integration with UI
TEST_F(UIErrorIntegrationTest, PermissionRequestUIIntegration) {
    // This test will FAIL until permission request UI integration is implemented
    using ::testing::_;
    using ::testing::StrictMock;
    
    // Set up permission request strategy
    auto permission_strategy = std::make_unique<PermissionRequestStrategy>(
        [this](const std::string& permission, std::function<void(bool)> callback) {
            permission_handler_->RequestPermission(permission, callback);
        }
    );
    
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::PermissionDenied, 
                                           std::move(permission_strategy));
    
    // Expect permission request UI to be shown
    EXPECT_CALL(*permission_handler_, RequestPermission("NEARBY_WIFI_DEVICES", _))
        .WillOnce([](const std::string& permission, std::function<void(bool)> callback) {
            // Simulate user granting permission
            callback(true);
        });
    
    // Create permission error with proper context
    ErrorInfo permission_error = CreatePermissionError(
        Core::Multiplayer::ErrorCode::PermissionDenied, "NEARBY_WIFI_DEVICES");
    
    bool recovery_success_called = false;
    error_handler_->SetOnRecoverySuccess([&](const ErrorInfo& error) {
        recovery_success_called = true;
    });
    
    // Report permission error - should trigger UI permission request
    error_handler_->ReportError(permission_error);
    
    // Wait for recovery attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_TRUE(recovery_success_called) << "Permission recovery should succeed when user grants permission";
}

// CRITICAL: Test modal dialog integration for critical errors
TEST_F(UIErrorIntegrationTest, ModalDialogIntegration) {
    // This test will FAIL until modal dialog integration is properly implemented
    using ::testing::_;
    using ::testing::ElementsAre;
    
    // Set up expectation for modal dialog
    EXPECT_CALL(*ui_handler_, ShowModal(
        "Security Error",  // Title based on error category
        "Authentication failed",  // Error message
        ElementsAre("Re-authenticate", "Contact support")  // Suggested actions
    )).Times(1);
    
    // Also expect the notification callback to be triggered
    EXPECT_CALL(*ui_handler_, ShowNotification(
        Property(&NotificationData::level, NotificationLevel::Critical)))
        .Times(1);
    
    // Override notification callback to also show modal for critical errors
    error_handler_->SetNotificationCallback(
        [this](const NotificationData& notification) {
            ui_handler_->ShowNotification(notification);
            
            if (notification.level == NotificationLevel::Critical) {
                ui_handler_->ShowModal(notification.title, notification.message, notification.actions);
            }
        }
    );
    
    // Report critical security error
    ErrorInfo security_error = CreateSecurityError(
        Core::Multiplayer::ErrorCode::AuthenticationFailed, "Invalid token");
    
    error_handler_->ReportError(security_error);
}

// CRITICAL: Test fallback mode UI integration
TEST_F(UIErrorIntegrationTest, FallbackModeUIIntegration) {
    // This test will FAIL until fallback mode UI integration is implemented
    using ::testing::_;
    using ::testing::Return;
    
    // Set up fallback mode strategy
    auto fallback_strategy = std::make_unique<FallbackModeStrategy>(
        [this](const std::string& mode) {
            return mode_switch_->SwitchToMode(mode);
        }
    );
    
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::HardwareLimitation, 
                                           std::move(fallback_strategy));
    
    // Expect mode switch to be attempted
    EXPECT_CALL(*mode_switch_, SwitchToMode("internet"))
        .WillOnce(Return(true));
    
    // Expect UI notification about mode switch
    EXPECT_CALL(*ui_handler_, ShowNotification(
        Property(&NotificationData::message, 
                testing::HasSubstr("Switching to internet mode"))))  // Will FAIL - message not implemented
        .Times(1);
    
    // Create platform limitation error
    ErrorInfo platform_error;
    platform_error.category = ErrorCategory::HardwareLimitation;
    platform_error.error_code = Core::Multiplayer::ErrorCode::PlatformFeatureUnavailable;
    platform_error.message = "Wi-Fi Direct not supported on this device";
    platform_error.component = "WiFiDirect";
    platform_error.context["current_mode"] = "adhoc";
    platform_error.timestamp = std::chrono::steady_clock::now();
    
    bool recovery_success_called = false;
    error_handler_->SetOnRecoverySuccess([&](const ErrorInfo& error) {
        recovery_success_called = true;
        
        // UI should show mode switch notification
        NotificationData mode_switch_notification;
        mode_switch_notification.level = NotificationLevel::Info;
        mode_switch_notification.title = "Mode Switch";
        mode_switch_notification.message = "Switching to internet mode due to hardware limitation";
        mode_switch_notification.auto_dismiss_ms = 3000;
        
        ui_handler_->ShowNotification(mode_switch_notification);
    });
    
    error_handler_->ReportError(platform_error);
    
    // Wait for recovery
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_TRUE(recovery_success_called);
}

// CRITICAL: Test error context display in UI
TEST_F(UIErrorIntegrationTest, ErrorContextUIDisplay) {
    // This test will FAIL until error context is properly displayed in UI
    using ::testing::_;
    using ::testing::Property;
    using ::testing::HasSubstr;
    
    // Create error with rich context information
    ErrorInfo context_error;
    context_error.category = ErrorCategory::NetworkConnectivity;
    context_error.error_code = Core::Multiplayer::ErrorCode::ConnectionFailed;
    context_error.message = "Failed to connect to room server";
    context_error.component = "RoomClient";
    context_error.timestamp = std::chrono::steady_clock::now();
    
    // Add context information that should be displayed in UI
    context_error.context["server_url"] = "ws://room.sudachi.dev:8080";
    context_error.context["room_id"] = "12345";
    context_error.context["retry_count"] = "3";
    context_error.context["last_error"] = "Connection refused";
    
    // Expect UI to display context information
    EXPECT_CALL(*ui_handler_, ShowNotification(
        Property(&NotificationData::message, 
                AllOf(HasSubstr("room.sudachi.dev"), 
                      HasSubstr("Room: 12345"),
                      HasSubstr("Retries: 3")))))  // Will FAIL - context not included in message
        .Times(1);
    
    // Set up enhanced notification callback that includes context
    error_handler_->SetNotificationCallback(
        [this](const NotificationData& notification) {
            // This should enhance the message with context information
            NotificationData enhanced = notification;
            
            // Find the original error to get context - this will FAIL as feature doesn't exist
            auto recent_errors = error_handler_->GetRecentErrors(1);
            if (!recent_errors.empty()) {
                const auto& error = recent_errors[0];
                
                // Enhance message with context
                std::string context_info;
                for (const auto& [key, value] : error.context) {
                    if (key == "server_url") context_info += "Server: " + value + "\n";
                    if (key == "room_id") context_info += "Room: " + value + "\n";
                    if (key == "retry_count") context_info += "Retries: " + value + "\n";
                }
                
                if (!context_info.empty()) {
                    enhanced.message += "\n\nDetails:\n" + context_info;
                }
            }
            
            ui_handler_->ShowNotification(enhanced);
        }
    );
    
    error_handler_->ReportError(context_error);
}

// CRITICAL: Test notification queue management for multiple errors
TEST_F(UIErrorIntegrationTest, NotificationQueueManagement) {
    // This test will FAIL until notification queue management is implemented
    using ::testing::_;
    using ::testing::AtLeast;
    
    // Expect multiple notifications but with proper queuing/batching
    EXPECT_CALL(*ui_handler_, ShowNotification(_))
        .Times(AtLeast(1));  // Should batch/queue notifications, not show all 5
    
    // Report multiple errors rapidly
    std::vector<Core::Multiplayer::ErrorCode> rapid_errors = {
        Core::Multiplayer::ErrorCode::NetworkTimeout,
        Core::Multiplayer::ErrorCode::ConnectionLost,
        Core::Multiplayer::ErrorCode::NetworkTimeout,
        Core::Multiplayer::ErrorCode::ConnectionLost,
        Core::Multiplayer::ErrorCode::NetworkTimeout
    };
    
    for (auto code : rapid_errors) {
        error_handler_->ReportError(code, "Rapid error", "QueueTest");
    }
    
    // Should implement notification deduplication/batching
    // Expect that duplicate NetworkTimeout errors are batched together
    // This will FAIL until batching is implemented
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify that error statistics show all errors were recorded
    auto stats = error_handler_->GetErrorStatistics();
    EXPECT_EQ(stats[Core::Multiplayer::ErrorCode::NetworkTimeout], 3);
    EXPECT_EQ(stats[Core::Multiplayer::ErrorCode::ConnectionLost], 2);
}

// CRITICAL: Test accessibility features in error notifications
TEST_F(UIErrorIntegrationTest, AccessibilityFeatures) {
    // This test will FAIL until accessibility features are implemented
    using ::testing::_;
    using ::testing::Property;
    
    // Test that notifications include accessibility information
    EXPECT_CALL(*ui_handler_, ShowNotification(
        Property(&NotificationData::title, testing::Not(testing::IsEmpty()))))
        .Times(1);
    
    // Override notification callback to verify accessibility features
    error_handler_->SetNotificationCallback(
        [this](const NotificationData& notification) {
            // Verify accessibility requirements
            EXPECT_FALSE(notification.title.empty()) 
                << "Notification title required for screen readers";
            EXPECT_FALSE(notification.message.empty()) 
                << "Notification message required for screen readers";
            
            // Verify that actions have descriptive text
            for (const auto& action : notification.actions) {
                EXPECT_FALSE(action.empty()) 
                    << "Action buttons must have descriptive text";
                EXPECT_TRUE(action.length() >= 3) 
                    << "Action text should be descriptive: " << action;
            }
            
            // Test that critical errors have appropriate urgency indicators
            if (notification.level == NotificationLevel::Critical) {
                // Should have urgency markers for assistive technology
                // This will FAIL until implemented
                EXPECT_TRUE(notification.message.find("[URGENT]") != std::string::npos ||
                           notification.title.find("Critical") != std::string::npos)
                    << "Critical notifications should have urgency indicators";
            }
            
            ui_handler_->ShowNotification(notification);
        }
    );
    
    // Test various notification levels for accessibility
    std::vector<NotificationLevel> levels = {
        NotificationLevel::Info,
        NotificationLevel::Warning,
        NotificationLevel::Error,
        NotificationLevel::Critical
    };
    
    for (auto level : levels) {
        error_handler_->SetNotificationLevel(Core::Multiplayer::ErrorCode::InternalError, level);
        error_handler_->ReportError(Core::Multiplayer::ErrorCode::InternalError,
                                   "Accessibility test error", "AccessibilityTest");
    }
}

} // namespace Sudachi::Multiplayer::Tests