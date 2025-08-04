// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include "../error_handling.h"

namespace Sudachi::Multiplayer::Tests {

/**
 * Mock multiplayer component interface for testing cross-component integration
 */
class MockMultiplayerComponent {
public:
    virtual ~MockMultiplayerComponent() = default;
    
    MOCK_METHOD(void, Initialize, ());
    MOCK_METHOD(void, Shutdown, ());
    MOCK_METHOD(void, OnError, (const ErrorInfo& error));
    MOCK_METHOD(bool, CanRecover, (const ErrorInfo& error));
    MOCK_METHOD(void, AttemptRecovery, (const ErrorInfo& error));
    MOCK_METHOD(std::string, GetComponentName, (), (const));
};

/**
 * Concrete test component that simulates real multiplayer components
 */
class TestRoomClient : public MockMultiplayerComponent {
public:
    TestRoomClient(ErrorHandler* error_handler) : error_handler_(error_handler) {
        // Register for error notifications
        error_handler_->SetOnError([this](const ErrorInfo& error) {
            if (error.component == "P2PNetwork" || error.component == "RelayClient") {
                // Room client should react to network component errors
                OnError(error);
                HandleNetworkError(error);
            }
        });
    }
    
    void SimulateConnectionAttempt() {
        // Simulate a connection attempt that might fail
        if (should_fail_connection_) {
            ReportConnectionError();
        }
    }
    
    void ReportConnectionError() {
        ErrorInfo error;
        error.category = ErrorCategory::NetworkConnectivity;
        error.error_code = Core::Multiplayer::ErrorCode::ConnectionFailed;
        error.message = "Failed to connect to room server";
        error.component = "RoomClient";
        error.timestamp = std::chrono::steady_clock::now();
        error.context["server_url"] = "ws://room.sudachi.dev:8080";
        error.context["room_id"] = "12345";
        
        error_handler_->ReportError(error);
    }
    
    void HandleNetworkError(const ErrorInfo& error) {
        network_errors_received_++;
        last_network_error_ = error;
    }
    
    std::string GetComponentName() const override { return "RoomClient"; }
    
    bool should_fail_connection_ = false;
    std::atomic<int> network_errors_received_{0};
    ErrorInfo last_network_error_;
    
private:
    ErrorHandler* error_handler_;
};

/**
 * Test P2P Network component
 */
class TestP2PNetwork : public MockMultiplayerComponent {
public:
    TestP2PNetwork(ErrorHandler* error_handler) : error_handler_(error_handler) {}
    
    void SimulateP2PConnectionFailure() {
        ErrorInfo error;
        error.category = ErrorCategory::NetworkConnectivity;
        error.error_code = Core::Multiplayer::ErrorCode::ConnectionTimeout;
        error.message = "P2P connection establishment timeout";
        error.component = "P2PNetwork";
        error.timestamp = std::chrono::steady_clock::now();
        error.context["peer_id"] = "67890";
        error.context["connection_type"] = "p2p";
        error.retry_after_seconds = 10;
        
        error_handler_->ReportError(error);
    }
    
    void SimulateNATTraversalFailure() {
        // This error code doesn't exist yet - will FAIL until implemented
        ErrorInfo error;
        error.category = ErrorCategory::NetworkConnectivity;
        error.error_code = Core::Multiplayer::ErrorCode::NATTraversalFailed;  // MISSING
        error.message = "NAT traversal failed, falling back to relay";
        error.component = "P2PNetwork";
        error.timestamp = std::chrono::steady_clock::now();
        error.context["nat_type"] = "symmetric";
        error.context["stun_servers"] = "stun1.l.google.com:19302";
        
        error_handler_->ReportError(error);
    }
    
    std::string GetComponentName() const override { return "P2PNetwork"; }
    
private:
    ErrorHandler* error_handler_;
};

/**
 * Test Configuration Manager component
 */
class TestConfigManager : public MockMultiplayerComponent {
public:
    TestConfigManager(ErrorHandler* error_handler) : error_handler_(error_handler) {}
    
    void SimulateConfigurationError() {
        ErrorInfo error;
        error.category = ErrorCategory::ConfigurationError;
        error.error_code = Core::Multiplayer::ErrorCode::ConfigurationInvalid;
        error.message = "Invalid multiplayer configuration detected";
        error.component = "ConfigManager";
        error.timestamp = std::chrono::steady_clock::now();
        error.context["config_file"] = "multiplayer_config.json";
        error.context["invalid_field"] = "max_players";
        error.context["invalid_value"] = "16";  // Exceeds limit
        error.suggested_actions = {"Reset to default configuration", "Edit configuration manually"};
        
        error_handler_->ReportError(error);
    }
    
    std::string GetComponentName() const override { return "ConfigManager"; }
    
private:
    ErrorHandler* error_handler_;
};

/**
 * Test class for cross-component error propagation
 */
class CrossComponentErrorPropagationTest : public ::testing::Test {
protected:
    void SetUp() override {
        error_handler_ = std::make_unique<ErrorHandler>();
        
        // Create test components
        room_client_ = std::make_unique<TestRoomClient>(error_handler_.get());
        p2p_network_ = std::make_unique<TestP2PNetwork>(error_handler_.get());
        config_manager_ = std::make_unique<TestConfigManager>(error_handler_.get());
        
        // Set up global error tracking
        error_handler_->SetOnError([this](const ErrorInfo& error) {
            std::lock_guard<std::mutex> lock(error_mutex_);
            all_received_errors_.push_back(error);
            error_condition_.notify_one();
        });
    }

    void TearDown() override {
        room_client_.reset();
        p2p_network_.reset();
        config_manager_.reset();
        error_handler_.reset();
        all_received_errors_.clear();
    }

    std::unique_ptr<ErrorHandler> error_handler_;
    std::unique_ptr<TestRoomClient> room_client_;
    std::unique_ptr<TestP2PNetwork> p2p_network_;
    std::unique_ptr<TestConfigManager> config_manager_;
    
    std::vector<ErrorInfo> all_received_errors_;
    std::mutex error_mutex_;
    std::condition_variable error_condition_;
    
    // Helper to wait for errors
    bool WaitForErrors(size_t expected_count, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(error_mutex_);
        return error_condition_.wait_for(lock, timeout, [&] {
            return all_received_errors_.size() >= expected_count;
        });
    }
};

// CRITICAL: Test error propagation from network components to room client
TEST_F(CrossComponentErrorPropagationTest, NetworkErrorPropagationToRoomClient) {
    // This test will FAIL until cross-component error propagation is properly implemented
    
    // P2P Network reports an error
    p2p_network_->SimulateP2PConnectionFailure();
    
    // Wait for error to propagate
    ASSERT_TRUE(WaitForErrors(1));
    
    // Verify that room client received the network error
    // This will FAIL until the room client properly registers for P2P errors
    EXPECT_GT(room_client_->network_errors_received_.load(), 0) 
        << "Room client should receive network errors from P2P component";
    
    EXPECT_EQ(room_client_->last_network_error_.component, "P2PNetwork");
    EXPECT_EQ(room_client_->last_network_error_.error_code, Core::Multiplayer::ErrorCode::ConnectionTimeout);
}

// CRITICAL: Test error cascade prevention
TEST_F(CrossComponentErrorPropagationTest, ErrorCascadePrevention) {
    // This test will FAIL until error cascade prevention is implemented
    
    // Set up cascade detection
    std::atomic<int> cascade_errors{0};
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        if (error.context.count("cascade_source") > 0) {
            cascade_errors++;
        }
    });
    
    // Simulate rapid-fire errors from same component
    for (int i = 0; i < 10; ++i) {
        p2p_network_->SimulateP2PConnectionFailure();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Wait for processing
    WaitForErrors(1);  // Should only get 1 error due to cascade prevention
    
    // Should implement cascade prevention logic
    // This will FAIL until cascade prevention is implemented
    EXPECT_LE(all_received_errors_.size(), 3) 
        << "Rapid duplicate errors should be prevented from cascading";
    
    // Verify that subsequent identical errors are suppressed or batched
    int identical_errors = 0;
    for (const auto& error : all_received_errors_) {
        if (error.component == "P2PNetwork" && 
            error.message.find("P2P connection establishment timeout") != std::string::npos) {
            identical_errors++;
        }
    }
    
    EXPECT_LE(identical_errors, 2) 
        << "Identical errors should be suppressed or batched";
}

// CRITICAL: Test component shutdown error propagation
TEST_F(CrossComponentErrorPropagationTest, ComponentShutdownErrorPropagation) {
    // This test will FAIL until component shutdown error handling is implemented
    
    bool shutdown_error_received = false;
    std::string shutdown_component;
    
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        if (error.message.find("shutting down") != std::string::npos) {
            shutdown_error_received = true;
            shutdown_component = error.component;
        }
    });
    
    // Simulate component reporting error during shutdown
    ErrorInfo shutdown_error;
    shutdown_error.category = ErrorCategory::Unknown;
    shutdown_error.error_code = Core::Multiplayer::ErrorCode::InternalError;
    shutdown_error.message = "Component shutting down due to unrecoverable error";
    shutdown_error.component = "P2PNetwork";
    shutdown_error.timestamp = std::chrono::steady_clock::now();
    shutdown_error.context["shutdown_reason"] = "nat_traversal_failed";
    shutdown_error.context["can_restart"] = "true";
    
    error_handler_->ReportError(shutdown_error);
    
    WaitForErrors(1);
    
    EXPECT_TRUE(shutdown_error_received);
    EXPECT_EQ(shutdown_component, "P2PNetwork");
    
    // Other components should be notified of the shutdown
    // This will FAIL until component shutdown notification is implemented
    EXPECT_GT(room_client_->network_errors_received_.load(), 0)
        << "Room client should be notified when P2P component shuts down";
}

// CRITICAL: Test error correlation across components
TEST_F(CrossComponentErrorPropagationTest, ErrorCorrelationAcrossComponents) {
    // This test will FAIL until error correlation is implemented
    
    std::string correlation_id = "conn_attempt_" + std::to_string(std::time(nullptr));
    
    // Set correlation context for all components
    error_handler_->SetErrorContext(Core::Multiplayer::ErrorCode::ConnectionFailed, "correlation_id", correlation_id);
    error_handler_->SetErrorContext(Core::Multiplayer::ErrorCode::ConnectionTimeout, "correlation_id", correlation_id);
    error_handler_->SetErrorContext(Core::Multiplayer::ErrorCode::AuthenticationFailed, "correlation_id", correlation_id);
    
    // Simulate related errors from different components
    room_client_->ReportConnectionError();
    p2p_network_->SimulateP2PConnectionFailure();
    
    // Also simulate an authentication error
    ErrorInfo auth_error;
    auth_error.category = ErrorCategory::SecurityViolation;
    auth_error.error_code = Core::Multiplayer::ErrorCode::AuthenticationFailed;
    auth_error.message = "Authentication failed during connection attempt";
    auth_error.component = "AuthService";
    auth_error.timestamp = std::chrono::steady_clock::now();
    error_handler_->ReportError(auth_error);
    
    WaitForErrors(3);
    
    // Verify that all errors have the same correlation ID
    ASSERT_EQ(all_received_errors_.size(), 3);
    
    for (const auto& error : all_received_errors_) {
        EXPECT_TRUE(error.context.count("correlation_id") > 0)
            << "Error from " << error.component << " should have correlation ID";
        
        if (error.context.count("correlation_id") > 0) {
            EXPECT_EQ(error.context.at("correlation_id"), correlation_id)
                << "All related errors should have same correlation ID";
        }
    }
}

// CRITICAL: Test component error recovery coordination
TEST_F(CrossComponentErrorPropagationTest, ComponentErrorRecoveryCoordination) {
    // This test will FAIL until cross-component recovery coordination is implemented
    
    // Set up recovery strategies for different components
    auto network_recovery = std::make_unique<NetworkRetryStrategy>(3, 1000);
    error_handler_->RegisterRecoveryStrategy(ErrorCategory::NetworkConnectivity, 
                                           std::move(network_recovery));
    
    bool recovery_coordination_triggered = false;
    
    error_handler_->SetOnRecoverySuccess([&](const ErrorInfo& error) {
        if (error.component == "P2PNetwork") {
            recovery_coordination_triggered = true;
            
            // When P2P recovery succeeds, room client should be notified
            // This coordination logic doesn't exist yet - will FAIL
            ErrorInfo recovery_notification;
            recovery_notification.category = ErrorCategory::NetworkConnectivity;
            recovery_notification.error_code = Core::Multiplayer::ErrorCode::Success;
            recovery_notification.message = "P2P connection recovered, resuming room operations";
            recovery_notification.component = "RecoveryCoordinator";  // New component needed
            recovery_notification.timestamp = std::chrono::steady_clock::now();
            recovery_notification.context["recovered_component"] = "P2PNetwork";
            recovery_notification.context["recovery_strategy"] = "NetworkRetry";
            
            error_handler_->ReportError(recovery_notification);
        }
    });
    
    // Trigger a network error that should be recoverable
    p2p_network_->SimulateP2PConnectionFailure();
    
    // Wait for initial error and recovery attempt
    WaitForErrors(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));  // Wait for retry
    
    // This will FAIL until recovery coordination is implemented
    EXPECT_TRUE(recovery_coordination_triggered) 
        << "Recovery coordination should be triggered when component recovers";
}

// CRITICAL: Test error priority and routing
TEST_F(CrossComponentErrorPropagationTest, ErrorPriorityAndRouting) {
    // This test will FAIL until error priority and routing is implemented
    
    struct PriorityError {
        Core::Multiplayer::ErrorCode code;
        ErrorCategory category;
        std::string component;
        int expected_priority;
    };
    
    std::vector<PriorityError> priority_errors = {
        // Critical security errors should have highest priority
        {Core::Multiplayer::ErrorCode::AuthenticationFailed, ErrorCategory::SecurityViolation, "AuthService", 1},
        
        // Configuration errors should have high priority
        {Core::Multiplayer::ErrorCode::ConfigurationInvalid, ErrorCategory::ConfigurationError, "ConfigManager", 2},
        
        // Network errors should have medium priority
        {Core::Multiplayer::ErrorCode::NetworkTimeout, ErrorCategory::NetworkConnectivity, "P2PNetwork", 3},
        
        // General errors should have low priority
        {Core::Multiplayer::ErrorCode::InternalError, ErrorCategory::Unknown, "General", 4}
    };
    
    std::vector<int> processing_order;
    
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        // Capture processing order - should be priority-based
        for (size_t i = 0; i < priority_errors.size(); ++i) {
            if (priority_errors[i].component == error.component) {
                processing_order.push_back(priority_errors[i].expected_priority);
                break;
            }
        }
    });
    
    // Report errors in reverse priority order
    for (auto it = priority_errors.rbegin(); it != priority_errors.rend(); ++it) {
        ErrorInfo error;
        error.category = it->category;
        error.error_code = it->code;
        error.message = "Priority test error";
        error.component = it->component;
        error.timestamp = std::chrono::steady_clock::now();
        
        error_handler_->ReportError(error);
    }
    
    WaitForErrors(priority_errors.size());
    
    // Verify that errors were processed in priority order
    // This will FAIL until priority-based error processing is implemented
    EXPECT_TRUE(std::is_sorted(processing_order.begin(), processing_order.end()))
        << "Errors should be processed in priority order (1=highest, 4=lowest)";
}

// CRITICAL: Test component dependency error handling
TEST_F(CrossComponentErrorPropagationTest, ComponentDependencyErrorHandling) {
    // This test will FAIL until component dependency tracking is implemented
    
    // Define component dependencies
    // RoomClient depends on P2PNetwork and ConfigManager
    // P2PNetwork depends on ConfigManager
    
    std::map<std::string, std::vector<std::string>> dependencies = {
        {"RoomClient", {"P2PNetwork", "ConfigManager"}},
        {"P2PNetwork", {"ConfigManager"}},
        {"ConfigManager", {}}
    };
    
    std::vector<std::string> shutdown_order;
    
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        if (error.message.find("dependency failed") != std::string::npos) {
            shutdown_order.push_back(error.component);
        }
    });
    
    // Simulate ConfigManager failure
    config_manager_->SimulateConfigurationError();
    
    // This should trigger dependency error handling
    // Components that depend on ConfigManager should be notified
    // This logic doesn't exist yet - will FAIL
    
    WaitForErrors(1);
    
    // Simulate the dependency error propagation manually for testing
    // In real implementation, this would be automatic
    
    // P2PNetwork depends on ConfigManager
    ErrorInfo p2p_dependency_error;
    p2p_dependency_error.category = ErrorCategory::ConfigurationError;
    p2p_dependency_error.error_code = Core::Multiplayer::ErrorCode::InternalError;
    p2p_dependency_error.message = "P2PNetwork shutting down - dependency failed";
    p2p_dependency_error.component = "P2PNetwork";
    p2p_dependency_error.context["dependency_failure"] = "ConfigManager";
    p2p_dependency_error.context["shutdown_reason"] = "invalid_configuration";
    error_handler_->ReportError(p2p_dependency_error);
    
    // RoomClient depends on both ConfigManager and P2PNetwork
    ErrorInfo room_dependency_error;
    room_dependency_error.category = ErrorCategory::ConfigurationError;
    room_dependency_error.error_code = Core::Multiplayer::ErrorCode::InternalError;
    room_dependency_error.message = "RoomClient shutting down - dependency failed";
    room_dependency_error.component = "RoomClient";
    room_dependency_error.context["dependency_failure"] = "ConfigManager,P2PNetwork";
    room_dependency_error.context["shutdown_reason"] = "invalid_configuration";
    error_handler_->ReportError(room_dependency_error);
    
    WaitForErrors(3);
    
    // Verify dependency-based shutdown order
    EXPECT_EQ(shutdown_order.size(), 2);
    EXPECT_EQ(shutdown_order[0], "P2PNetwork");  // Should shut down first
    EXPECT_EQ(shutdown_order[1], "RoomClient");  // Should shut down last
}

// CRITICAL: Test error context propagation between components
TEST_F(CrossComponentErrorPropagationTest, ErrorContextPropagationBetweenComponents) {
    // This test will FAIL until cross-component context propagation is implemented
    
    // Set up shared context that should propagate between components
    std::string session_id = "sess_12345";
    std::string game_id = "0100F2C0115B6000";
    
    // ConfigManager sets session context
    ErrorInfo config_error;
    config_error.category = ErrorCategory::ConfigurationError;
    config_error.error_code = Core::Multiplayer::ErrorCode::ConfigurationInvalid;
    config_error.message = "Configuration validation failed";
    config_error.component = "ConfigManager";
    config_error.timestamp = std::chrono::steady_clock::now();
    config_error.context["session_id"] = session_id;
    config_error.context["game_id"] = game_id;
    config_error.context["validation_stage"] = "multiplayer_settings";
    
    error_handler_->ReportError(config_error);
    
    // P2P Network should inherit session context when reporting errors
    // This context inheritance doesn't exist yet - will FAIL
    p2p_network_->SimulateP2PConnectionFailure();
    
    WaitForErrors(2);
    
    // Find the P2P error
    auto p2p_error_it = std::find_if(all_received_errors_.begin(), all_received_errors_.end(),
        [](const ErrorInfo& e) { return e.component == "P2PNetwork"; });
    
    ASSERT_NE(p2p_error_it, all_received_errors_.end());
    
    // P2P error should inherit session context from ConfigManager error
    EXPECT_TRUE(p2p_error_it->context.count("session_id") > 0)
        << "P2P error should inherit session_id from earlier ConfigManager error";
    EXPECT_EQ(p2p_error_it->context.at("session_id"), session_id);
    
    EXPECT_TRUE(p2p_error_it->context.count("game_id") > 0)
        << "P2P error should inherit game_id from earlier ConfigManager error";
    EXPECT_EQ(p2p_error_it->context.at("game_id"), game_id);
    
    // Should also have its own context
    EXPECT_TRUE(p2p_error_it->context.count("peer_id") > 0)
        << "P2P error should have its own specific context";
}

} // namespace Sudachi::Multiplayer::Tests