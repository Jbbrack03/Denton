// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "../error_handling.h"

namespace Sudachi::Multiplayer::Tests {

/**
 * Test class for error context enhancement functionality
 * Tests the ability to enrich error information with additional context
 */
class ErrorContextEnhancementTest : public ::testing::Test {
protected:
    void SetUp() override {
        error_handler_ = std::make_unique<ErrorHandler>();
        
        // Set up error callback to capture reported errors
        error_handler_->SetOnError([this](const ErrorInfo& error) {
            captured_errors_.push_back(error);
        });
    }

    void TearDown() override {
        error_handler_.reset();
        captured_errors_.clear();
    }

    std::unique_ptr<ErrorHandler> error_handler_;
    std::vector<ErrorInfo> captured_errors_;
};

// CRITICAL: Test error context setting before error reporting
TEST_F(ErrorContextEnhancementTest, SetErrorContextBeforeReporting) {
    // This test will FAIL until SetErrorContext is properly implemented
    // Currently the implementation is stubbed out
    
    Core::Multiplayer::ErrorCode test_code = Core::Multiplayer::ErrorCode::ConnectionFailed;
    
    // Set context information before reporting error
    error_handler_->SetErrorContext(test_code, "server_url", "ws://room.sudachi.dev:8080");
    error_handler_->SetErrorContext(test_code, "room_id", "12345");
    error_handler_->SetErrorContext(test_code, "attempt_number", "3");
    error_handler_->SetErrorContext(test_code, "user_id", "player_001");
    error_handler_->SetErrorContext(test_code, "game_id", "0100F2C0115B6000");
    
    // Report the error
    error_handler_->ReportError(test_code, "Failed to connect to room server", "RoomClient");
    
    // Verify that context was applied to the reported error
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // These assertions will FAIL until context enhancement is implemented
    EXPECT_TRUE(error.context.count("server_url") > 0);
    EXPECT_EQ(error.context.at("server_url"), "ws://room.sudachi.dev:8080");
    
    EXPECT_TRUE(error.context.count("room_id") > 0);
    EXPECT_EQ(error.context.at("room_id"), "12345");
    
    EXPECT_TRUE(error.context.count("attempt_number") > 0);
    EXPECT_EQ(error.context.at("attempt_number"), "3");
    
    EXPECT_TRUE(error.context.count("user_id") > 0);
    EXPECT_EQ(error.context.at("user_id"), "player_001");
    
    EXPECT_TRUE(error.context.count("game_id") > 0);
    EXPECT_EQ(error.context.at("game_id"), "0100F2C0115B6000");
}

// CRITICAL: Test retry delay setting functionality
TEST_F(ErrorContextEnhancementTest, SetRetryDelayFunctionality) {
    // This test will FAIL until SetRetryDelay is properly implemented
    // Currently the implementation is stubbed out
    
    Core::Multiplayer::ErrorCode network_error = Core::Multiplayer::ErrorCode::NetworkTimeout;
    
    // Set retry delay for network errors
    error_handler_->SetRetryDelay(network_error, 30); // 30 seconds
    
    // Report the error
    error_handler_->ReportError(network_error, "Network connection timeout", "NetworkLayer");
    
    // Verify that retry delay was set
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // This will FAIL until retry delay setting is implemented
    EXPECT_TRUE(error.retry_after_seconds.has_value());
    EXPECT_EQ(error.retry_after_seconds.value(), 30);
}

// CRITICAL: Test suggested action addition functionality
TEST_F(ErrorContextEnhancementTest, AddSuggestedActionFunctionality) {
    // This test will FAIL until AddSuggestedAction is properly implemented
    // Currently the implementation is stubbed out
    
    Core::Multiplayer::ErrorCode config_error = Core::Multiplayer::ErrorCode::ConfigurationInvalid;
    
    // Add multiple suggested actions
    error_handler_->AddSuggestedAction(config_error, "Check multiplayer settings");
    error_handler_->AddSuggestedAction(config_error, "Reset configuration to defaults");
    error_handler_->AddSuggestedAction(config_error, "Verify network settings");
    error_handler_->AddSuggestedAction(config_error, "Contact support");
    
    // Report the error
    error_handler_->ReportError(config_error, "invalid multiplayer configuration", "ConfigManager");
    
    // Verify that suggested actions were added
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // This will FAIL until suggested action addition is implemented
    EXPECT_EQ(error.suggested_actions.size(), 4);
    EXPECT_TRUE(std::find(error.suggested_actions.begin(), error.suggested_actions.end(), 
                         "Check multiplayer settings") != error.suggested_actions.end());
    EXPECT_TRUE(std::find(error.suggested_actions.begin(), error.suggested_actions.end(), 
                         "Reset configuration to defaults") != error.suggested_actions.end());
    EXPECT_TRUE(std::find(error.suggested_actions.begin(), error.suggested_actions.end(), 
                         "Verify network settings") != error.suggested_actions.end());
    EXPECT_TRUE(std::find(error.suggested_actions.begin(), error.suggested_actions.end(), 
                         "Contact support") != error.suggested_actions.end());
}

// CRITICAL: Test context persistence across multiple error reports
TEST_F(ErrorContextEnhancementTest, ContextPersistenceAcrossErrors) {
    // This test will FAIL until context persistence is properly implemented
    
    Core::Multiplayer::ErrorCode error_code = Core::Multiplayer::ErrorCode::RoomNotFound;
    
    // Set context that should persist
    error_handler_->SetErrorContext(error_code, "session_id", "abc123");
    error_handler_->SetErrorContext(error_code, "server_region", "us-west");
    
    // Report first error
    error_handler_->ReportError(error_code, "Room not found - first attempt", "RoomClient");
    
    // Report second error of same type - should inherit context
    error_handler_->ReportError(error_code, "Room not found - second attempt", "RoomClient");
    
    // Verify both errors have the context
    ASSERT_EQ(captured_errors_.size(), 2);
    
    for (const auto& error : captured_errors_) {
        EXPECT_TRUE(error.context.count("session_id") > 0);
        EXPECT_EQ(error.context.at("session_id"), "abc123");
        
        EXPECT_TRUE(error.context.count("server_region") > 0);
        EXPECT_EQ(error.context.at("server_region"), "us-west");
    }
}

// CRITICAL: Test context override behavior
TEST_F(ErrorContextEnhancementTest, ContextOverrideBehavior) {
    // This test will FAIL until context override behavior is defined and implemented
    
    Core::Multiplayer::ErrorCode error_code = Core::Multiplayer::ErrorCode::AuthenticationFailed;
    
    // Set initial context
    error_handler_->SetErrorContext(error_code, "auth_method", "token");
    error_handler_->SetErrorContext(error_code, "user_id", "user123");
    
    // Override existing context
    error_handler_->SetErrorContext(error_code, "auth_method", "oauth2");
    error_handler_->SetErrorContext(error_code, "token_expiry", "2025-08-04T12:00:00Z");
    
    // Report error
    error_handler_->ReportError(error_code, "Authentication failed with new method", "AuthService");
    
    // Verify that context was properly overridden and merged
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // Should have overridden value
    EXPECT_TRUE(error.context.count("auth_method") > 0);
    EXPECT_EQ(error.context.at("auth_method"), "oauth2");  // Overridden value
    
    // Should have original value
    EXPECT_TRUE(error.context.count("user_id") > 0);
    EXPECT_EQ(error.context.at("user_id"), "user123");
    
    // Should have new value
    EXPECT_TRUE(error.context.count("token_expiry") > 0);
    EXPECT_EQ(error.context.at("token_expiry"), "2025-08-04T12:00:00Z");
}

// CRITICAL: Test context clearing or isolation between different error codes
TEST_F(ErrorContextEnhancementTest, ContextIsolationBetweenErrorCodes) {
    // This test will FAIL until context isolation is properly implemented
    
    Core::Multiplayer::ErrorCode network_error = Core::Multiplayer::ErrorCode::NetworkTimeout;
    Core::Multiplayer::ErrorCode permission_error = Core::Multiplayer::ErrorCode::PermissionDenied;
    
    // Set context for network error
    error_handler_->SetErrorContext(network_error, "server_url", "ws://test.com");
    error_handler_->SetErrorContext(network_error, "timeout_ms", "5000");
    
    // Set context for permission error
    error_handler_->SetErrorContext(permission_error, "permission_name", "NEARBY_WIFI");
    error_handler_->SetErrorContext(permission_error, "platform", "android");
    
    // Report both errors
    error_handler_->ReportError(network_error, "Network timeout occurred", "NetworkClient");
    error_handler_->ReportError(permission_error, "Permission denied", "PermissionManager");
    
    // Verify context isolation
    ASSERT_EQ(captured_errors_.size(), 2);
    
    // Find network error
    auto* network_error_info = std::find_if(captured_errors_.begin(), captured_errors_.end(),
        [](const ErrorInfo& e) { return e.component == "NetworkClient"; });
    ASSERT_NE(network_error_info, captured_errors_.end());
    
    // Find permission error
    auto* permission_error_info = std::find_if(captured_errors_.begin(), captured_errors_.end(),
        [](const ErrorInfo& e) { return e.component == "PermissionManager"; });
    ASSERT_NE(permission_error_info, captured_errors_.end());
    
    // Network error should only have network context
    EXPECT_TRUE(network_error_info->context.count("server_url") > 0);
    EXPECT_TRUE(network_error_info->context.count("timeout_ms") > 0);
    EXPECT_FALSE(network_error_info->context.count("permission_name") > 0);  // Should be isolated
    EXPECT_FALSE(network_error_info->context.count("platform") > 0);  // Should be isolated
    
    // Permission error should only have permission context
    EXPECT_TRUE(permission_error_info->context.count("permission_name") > 0);
    EXPECT_TRUE(permission_error_info->context.count("platform") > 0);
    EXPECT_FALSE(permission_error_info->context.count("server_url") > 0);  // Should be isolated
    EXPECT_FALSE(permission_error_info->context.count("timeout_ms") > 0);  // Should be isolated
}

// CRITICAL: Test complex context scenarios with nested information
TEST_F(ErrorContextEnhancementTest, ComplexContextScenarios) {
    // This test will FAIL until complex context handling is implemented
    
    Core::Multiplayer::ErrorCode p2p_error = Core::Multiplayer::ErrorCode::ConnectionFailed;
    
    // Set complex nested context information
    error_handler_->SetErrorContext(p2p_error, "connection_type", "p2p");
    error_handler_->SetErrorContext(p2p_error, "local_peer_id", "12345");
    error_handler_->SetErrorContext(p2p_error, "remote_peer_id", "67890");
    error_handler_->SetErrorContext(p2p_error, "ice_state", "failed");
    error_handler_->SetErrorContext(p2p_error, "stun_servers", "stun1.l.google.com:19302,stun2.l.google.com:19302");
    error_handler_->SetErrorContext(p2p_error, "turn_servers", "turn.sudachi.dev:3478");
    error_handler_->SetErrorContext(p2p_error, "nat_type", "symmetric");
    error_handler_->SetErrorContext(p2p_error, "bandwidth_estimate", "1.5mbps");
    error_handler_->SetErrorContext(p2p_error, "rtt_estimate", "150ms");
    error_handler_->SetErrorContext(p2p_error, "packet_loss", "0.02");
    
    // Also set retry delay and suggested actions
    error_handler_->SetRetryDelay(p2p_error, 15);
    error_handler_->AddSuggestedAction(p2p_error, "Try different STUN/TURN servers");
    error_handler_->AddSuggestedAction(p2p_error, "Check firewall settings");
    error_handler_->AddSuggestedAction(p2p_error, "Switch to relay mode");
    
    // Report the error
    error_handler_->ReportError(p2p_error, "P2P connection establishment failed", "P2PNetwork");
    
    // Verify all context information is present
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // Verify all context keys are present
    std::vector<std::string> expected_keys = {
        "connection_type", "local_peer_id", "remote_peer_id", "ice_state",
        "stun_servers", "turn_servers", "nat_type", "bandwidth_estimate",
        "rtt_estimate", "packet_loss"
    };
    
    for (const auto& key : expected_keys) {
        EXPECT_TRUE(error.context.count(key) > 0) 
            << "Missing context key: " << key;
    }
    
    // Verify specific values
    EXPECT_EQ(error.context.at("connection_type"), "p2p");
    EXPECT_EQ(error.context.at("nat_type"), "symmetric");
    EXPECT_EQ(error.context.at("packet_loss"), "0.02");
    
    // Verify retry delay and suggested actions
    EXPECT_TRUE(error.retry_after_seconds.has_value());
    EXPECT_EQ(error.retry_after_seconds.value(), 15);
    
    EXPECT_EQ(error.suggested_actions.size(), 3);
    EXPECT_TRUE(std::find(error.suggested_actions.begin(), error.suggested_actions.end(),
                         "Try different STUN/TURN servers") != error.suggested_actions.end());
}

// CRITICAL: Test context enhancement with ErrorInfo direct reporting
TEST_F(ErrorContextEnhancementTest, ContextEnhancementWithDirectErrorInfo) {
    // This test will FAIL until context enhancement works with direct ErrorInfo reporting
    
    Core::Multiplayer::ErrorCode test_code = Core::Multiplayer::ErrorCode::ServiceUnavailable;
    
    // Set context for the error code
    error_handler_->SetErrorContext(test_code, "service_name", "mdns_discovery");
    error_handler_->SetErrorContext(test_code, "network_interface", "wlan0");
    error_handler_->SetRetryDelay(test_code, 10);
    
    // Create ErrorInfo directly
    ErrorInfo direct_error;
    direct_error.category = ErrorCategory::NetworkConnectivity;
    direct_error.error_code = test_code;
    direct_error.message = "mDNS service discovery failed";
    direct_error.component = "MdnsDiscovery";
    direct_error.timestamp = std::chrono::steady_clock::now();
    
    // Report using direct ErrorInfo - should still get context enhancement
    error_handler_->ReportError(direct_error);
    
    // Verify that context was enhanced even with direct reporting
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // Should have original ErrorInfo data
    EXPECT_EQ(error.message, "mDNS service discovery failed");
    EXPECT_EQ(error.component, "MdnsDiscovery");
    
    // Should also have enhanced context
    EXPECT_TRUE(error.context.count("service_name") > 0);
    EXPECT_EQ(error.context.at("service_name"), "mdns_discovery");
    
    EXPECT_TRUE(error.context.count("network_interface") > 0);
    EXPECT_EQ(error.context.at("network_interface"), "wlan0");
    
    // Should have enhanced retry delay (if not already set)
    if (!direct_error.retry_after_seconds.has_value()) {
        EXPECT_TRUE(error.retry_after_seconds.has_value());
        EXPECT_EQ(error.retry_after_seconds.value(), 10);
    }
}

// CRITICAL: Test context storage and retrieval mechanism
TEST_F(ErrorContextEnhancementTest, ContextStorageAndRetrievalMechanism) {
    // This test will FAIL until proper context storage mechanism is implemented
    
    // Test that context is properly stored internally and can be retrieved
    Core::Multiplayer::ErrorCode error_code = Core::Multiplayer::ErrorCode::MaxPeersExceeded;
    
    // Set various context items
    error_handler_->SetErrorContext(error_code, "current_peers", "8");
    error_handler_->SetErrorContext(error_code, "max_peers", "8");
    error_handler_->SetErrorContext(error_code, "room_mode", "public");
    
    // There should be a way to query the stored context (this API doesn't exist yet)
    // This will FAIL until context query API is implemented
    
    // For now, we test indirectly by reporting an error and checking the result
    error_handler_->ReportError(error_code, "Maximum peer limit reached", "RoomManager");
    
    ASSERT_EQ(captured_errors_.size(), 1);
    const auto& error = captured_errors_[0];
    
    // Verify context was stored and retrieved correctly
    EXPECT_EQ(error.context.size(), 3);
    EXPECT_EQ(error.context.at("current_peers"), "8");
    EXPECT_EQ(error.context.at("max_peers"), "8");
    EXPECT_EQ(error.context.at("room_mode"), "public");
    
    // Test context update
    error_handler_->SetErrorContext(error_code, "current_peers", "9");  // Update existing
    error_handler_->SetErrorContext(error_code, "waiting_queue", "2");   // Add new
    
    // Report another error
    error_handler_->ReportError(error_code, "Peer limit exceeded again", "RoomManager");
    
    ASSERT_EQ(captured_errors_.size(), 2);
    const auto& updated_error = captured_errors_[1];
    
    // Should have updated context
    EXPECT_EQ(updated_error.context.at("current_peers"), "9");  // Updated
    EXPECT_EQ(updated_error.context.at("max_peers"), "8");      // Unchanged
    EXPECT_EQ(updated_error.context.at("waiting_queue"), "2");  // New
}

// CRITICAL: Test context thread safety
TEST_F(ErrorContextEnhancementTest, ContextThreadSafety) {
    // This test will FAIL until thread safety is properly implemented in context handling
    
    Core::Multiplayer::ErrorCode thread_error = Core::Multiplayer::ErrorCode::InternalError;
    
    std::atomic<int> completed_threads{0};
    const int num_threads = 10;
    const int operations_per_thread = 50;
    
    // Launch multiple threads that set context and report errors concurrently
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string thread_id = "thread_" + std::to_string(t);
                std::string operation_id = "op_" + std::to_string(i);
                
                // Set context
                error_handler_->SetErrorContext(thread_error, "thread_id", thread_id);
                error_handler_->SetErrorContext(thread_error, "operation_id", operation_id);
                error_handler_->SetRetryDelay(thread_error, t + 1);
                
                // Report error
                error_handler_->ReportError(thread_error, 
                    "Thread " + thread_id + " operation " + operation_id, 
                    "ThreadTest");
                
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            completed_threads++;
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed_threads, num_threads);
    
    // Verify that all errors were reported (thread safety check)
    EXPECT_EQ(captured_errors_.size(), num_threads * operations_per_thread);
    
    // Verify that each error has proper context (no data corruption)
    for (const auto& error : captured_errors_) {
        EXPECT_TRUE(error.context.count("thread_id") > 0);
        EXPECT_TRUE(error.context.count("operation_id") > 0);
        EXPECT_TRUE(error.retry_after_seconds.has_value());
        
        // Context should be consistent (thread_id should match what's in the message)
        std::string thread_id = error.context.at("thread_id");
        EXPECT_TRUE(error.message.find(thread_id) != std::string::npos)
            << "Context thread_id " << thread_id << " should match message: " << error.message;
    }
}

} // namespace Sudachi::Multiplayer::Tests
