// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <unordered_set>
#include "../error_codes.h"
#include "../error_handling.h"

namespace Sudachi::Multiplayer::Tests {

/**
 * Test class for error code integration with error handling framework
 * Tests the critical integration between error_codes.h and error_handling.h/cpp
 */
class ErrorCodeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        error_handler_ = std::make_unique<ErrorHandler>();
    }

    void TearDown() override {
        error_handler_.reset();
    }

    std::unique_ptr<ErrorHandler> error_handler_;
};

// CRITICAL: Test namespace consistency between error_codes.h and error_handling.h
TEST_F(ErrorCodeIntegrationTest, NamespaceConsistency) {
    // This test will FAIL until namespaces are aligned
    // error_codes.h uses Core::Multiplayer
    // error_handling.h uses Sudachi::Multiplayer
    
    // Test that we can create ErrorInfo with error codes from the same namespace
    ErrorInfo error;
    error.error_code = Core::Multiplayer::ErrorCode::Success;  // This should FAIL compilation
    error.category = ErrorCategory::NetworkConnectivity;
    
    EXPECT_EQ(error.error_code, Core::Multiplayer::ErrorCode::Success);
}

// CRITICAL: Test that all error codes referenced in error_handling.cpp exist
TEST_F(ErrorCodeIntegrationTest, AllReferencedErrorCodesExist) {
    // These error codes are referenced in error_handling.cpp but don't exist in error_codes.h
    // This test will FAIL until they are added
    
    std::vector<Core::Multiplayer::ErrorCode> referenced_codes = {
        Core::Multiplayer::ErrorCode::NetworkTimeout,        // MISSING - will fail
        Core::Multiplayer::ErrorCode::ConnectionRefused,     // MISSING - will fail
        Core::Multiplayer::ErrorCode::HostUnreachable,       // MISSING - will fail
        Core::Multiplayer::ErrorCode::ConnectionLost,        // MISSING - will fail
        Core::Multiplayer::ErrorCode::InvalidResponse,       // MISSING - will fail
        Core::Multiplayer::ErrorCode::SSLError,              // MISSING - will fail
        Core::Multiplayer::ErrorCode::ProtocolError,         // MISSING - will fail
        Core::Multiplayer::ErrorCode::ResourceExhausted,     // MISSING - will fail
    };
    
    // Test that each code can be used without compilation errors
    for (auto code : referenced_codes) {
        ErrorInfo error;
        error.error_code = code;
        error.category = GetErrorCategory(code);  // This function should exist
        EXPECT_NE(error.category, ErrorCategory::Unknown);
    }
}

// CRITICAL: Test error code categorization completeness
TEST_F(ErrorCodeIntegrationTest, ErrorCodeCategorizationCompleteness) {
    // Test that ALL error codes from error_codes.h have proper categorization
    std::vector<Core::Multiplayer::ErrorCode> all_codes = {
        Core::Multiplayer::ErrorCode::Success,
        Core::Multiplayer::ErrorCode::ConnectionFailed,
        Core::Multiplayer::ErrorCode::ConnectionTimeout,
        Core::Multiplayer::ErrorCode::AuthenticationFailed,
        Core::Multiplayer::ErrorCode::AlreadyConnected,
        Core::Multiplayer::ErrorCode::NotConnected,
        Core::Multiplayer::ErrorCode::RoomNotFound,
        Core::Multiplayer::ErrorCode::RoomFull,
        Core::Multiplayer::ErrorCode::RoomPasswordRequired,
        Core::Multiplayer::ErrorCode::InvalidRoomPassword,
        Core::Multiplayer::ErrorCode::AlreadyInRoom,
        Core::Multiplayer::ErrorCode::NotInRoom,
        Core::Multiplayer::ErrorCode::MessageTooLarge,
        Core::Multiplayer::ErrorCode::MessageTimeout,
        Core::Multiplayer::ErrorCode::InvalidMessage,
        Core::Multiplayer::ErrorCode::MessageQueueFull,
        Core::Multiplayer::ErrorCode::InvalidParameter,
        Core::Multiplayer::ErrorCode::InternalError,
        Core::Multiplayer::ErrorCode::NetworkError,
        Core::Multiplayer::ErrorCode::Timeout,
        Core::Multiplayer::ErrorCode::NotSupported,
        Core::Multiplayer::ErrorCode::PermissionDenied,
        Core::Multiplayer::ErrorCode::NotInitialized,
        Core::Multiplayer::ErrorCode::InvalidState,
        Core::Multiplayer::ErrorCode::DiscoveryFailed,
        Core::Multiplayer::ErrorCode::ServiceUnavailable,
        Core::Multiplayer::ErrorCode::MaxPeersExceeded,
        Core::Multiplayer::ErrorCode::PlatformAPIError,
        Core::Multiplayer::ErrorCode::PlatformFeatureUnavailable,
        Core::Multiplayer::ErrorCode::PlatformPermissionDenied,
        Core::Multiplayer::ErrorCode::ConfigurationInvalid,
        Core::Multiplayer::ErrorCode::ConfigurationMissing
    };
    
    for (auto code : all_codes) {
        // This will FAIL until GetErrorCategory is accessible and all codes are mapped
        ErrorCategory category = GetErrorCategory(code);  // Function doesn't exist yet
        EXPECT_NE(category, ErrorCategory::Unknown) 
            << "Error code " << static_cast<int>(code) << " should have a proper category";
    }
}

// CRITICAL: Test PRD Section 7.2 error code ranges
TEST_F(ErrorCodeIntegrationTest, PRDErrorCodeRangesImplemented) {
    // PRD Section 7.2 specifies these error code ranges:
    // 1000-1999: Network errors
    // 2000-2099: Permission errors  
    // 2100-2199: Configuration errors
    // 2200-2299: Protocol errors
    // 2300-2399: Resource errors
    // 2400-2499: Security errors
    // 4000-4999: Platform-specific errors
    
    // These ranges are not implemented in current error_codes.h
    // This test will FAIL until numeric error codes are added
    
    // Network errors (1000-1999)
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::NetworkTimeout) >= 1000);
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::NetworkTimeout) < 2000);
    
    // Permission errors (2000-2099)
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::PermissionDenied) >= 2000);
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::PermissionDenied) < 2100);
    
    // Configuration errors (2100-2199)
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::ConfigurationInvalid) >= 2100);
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::ConfigurationInvalid) < 2200);
    
    // Platform errors (4000-4999)
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::PlatformAPIError) >= 4000);
    EXPECT_TRUE(static_cast<int>(Core::Multiplayer::ErrorCode::PlatformAPIError) < 5000);
}

// CRITICAL: Test default error message consistency
TEST_F(ErrorCodeIntegrationTest, DefaultErrorMessageConsistency) {
    // Test that GetDefaultErrorMessage function exists and works for all codes
    std::vector<Core::Multiplayer::ErrorCode> codes_with_messages = {
        Core::Multiplayer::ErrorCode::NetworkTimeout,
        Core::Multiplayer::ErrorCode::ConnectionRefused,
        Core::Multiplayer::ErrorCode::PermissionDenied,
        Core::Multiplayer::ErrorCode::InvalidParameter,
        Core::Multiplayer::ErrorCode::AuthenticationFailed,
        Core::Multiplayer::ErrorCode::ConfigurationInvalid
    };
    
    for (auto code : codes_with_messages) {
        // This will FAIL until GetDefaultErrorMessage is made public or accessible
        std::string message = GetDefaultErrorMessage(code);  // Function is private
        EXPECT_FALSE(message.empty()) 
            << "Error code " << static_cast<int>(code) << " should have a default message";
        EXPECT_NE(message, "An error occurred") 
            << "Error code " << static_cast<int>(code) << " should have a specific message";
    }
}

// CRITICAL: Test error code to category mapping ranges
TEST_F(ErrorCodeIntegrationTest, ErrorCodeCategoryMappingRanges) {
    // Test the range-based categorization logic in GetErrorCategory
    // This will FAIL until the function is properly implemented with correct ranges
    
    struct TestCase {
        Core::Multiplayer::ErrorCode code;
        ErrorCategory expected_category;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        // Network errors (1000-1999)
        {Core::Multiplayer::ErrorCode::NetworkTimeout, ErrorCategory::NetworkConnectivity, "Network timeout"},
        {Core::Multiplayer::ErrorCode::ConnectionLost, ErrorCategory::NetworkConnectivity, "Connection lost"},
        {Core::Multiplayer::ErrorCode::SSLError, ErrorCategory::NetworkConnectivity, "SSL error"},
        
        // Permission errors (2000-2099)
        {Core::Multiplayer::ErrorCode::PermissionDenied, ErrorCategory::PermissionDenied, "Permission denied"},
        
        // Configuration errors (2100-2199)
        {Core::Multiplayer::ErrorCode::ConfigurationInvalid, ErrorCategory::ConfigurationError, "Invalid config"},
        {Core::Multiplayer::ErrorCode::ConfigurationMissing, ErrorCategory::ConfigurationError, "Missing config"},
        
        // Protocol errors (2200-2299)
        {Core::Multiplayer::ErrorCode::ProtocolError, ErrorCategory::ProtocolMismatch, "Protocol error"},
        
        // Resource errors (2300-2399)
        {Core::Multiplayer::ErrorCode::ResourceExhausted, ErrorCategory::ResourceExhausted, "Resource exhausted"},
        
        // Security errors (2400-2499)
        {Core::Multiplayer::ErrorCode::AuthenticationFailed, ErrorCategory::SecurityViolation, "Auth failed"},
        
        // Platform errors (4000-4999)
        {Core::Multiplayer::ErrorCode::PlatformAPIError, ErrorCategory::HardwareLimitation, "Platform API error"},
        {Core::Multiplayer::ErrorCode::PlatformFeatureUnavailable, ErrorCategory::HardwareLimitation, "Feature unavailable"},
        {Core::Multiplayer::ErrorCode::PlatformPermissionDenied, ErrorCategory::HardwareLimitation, "Platform permission"}
    };
    
    for (const auto& test_case : test_cases) {
        ErrorCategory actual = GetErrorCategory(test_case.code);
        EXPECT_EQ(actual, test_case.expected_category) 
            << "Error code for " << test_case.description 
            << " should be categorized as " << static_cast<int>(test_case.expected_category)
            << " but got " << static_cast<int>(actual);
    }
}

// CRITICAL: Test that error handler can process all defined error codes
TEST_F(ErrorCodeIntegrationTest, ErrorHandlerProcessesAllCodes) {
    // Test that ErrorHandler can successfully report errors for all defined codes
    bool callback_called = false;
    ErrorInfo received_error;
    
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        callback_called = true;
        received_error = error;
    });
    
    std::vector<Core::Multiplayer::ErrorCode> all_codes = {
        Core::Multiplayer::ErrorCode::Success,
        Core::Multiplayer::ErrorCode::ConnectionFailed,
        Core::Multiplayer::ErrorCode::AuthenticationFailed,
        Core::Multiplayer::ErrorCode::PermissionDenied,
        Core::Multiplayer::ErrorCode::ConfigurationInvalid,
        Core::Multiplayer::ErrorCode::PlatformAPIError
    };
    
    for (auto code : all_codes) {
        callback_called = false;
        
        // This will FAIL until namespace issues are resolved
        error_handler_->ReportError(code, "Test error", "TestComponent");
        
        EXPECT_TRUE(callback_called) 
            << "Error handler should call callback for error code " << static_cast<int>(code);
        EXPECT_EQ(received_error.error_code, code);
        EXPECT_EQ(received_error.component, "TestComponent");
        EXPECT_FALSE(received_error.message.empty());
    }
}

// CRITICAL: Test cross-component error propagation
TEST_F(ErrorCodeIntegrationTest, CrossComponentErrorPropagation) {
    // Test that errors can be propagated between different multiplayer components
    std::vector<ErrorInfo> received_errors;
    
    error_handler_->SetOnError([&](const ErrorInfo& error) {
        received_errors.push_back(error);
    });
    
    // Simulate errors from different components
    struct ComponentError {
        std::string component;
        Core::Multiplayer::ErrorCode code;
        std::string context_key;
        std::string context_value;
    };
    
    std::vector<ComponentError> component_errors = {
        {"RoomClient", Core::Multiplayer::ErrorCode::ConnectionFailed, "server_url", "ws://test.com"},
        {"P2PNetwork", Core::Multiplayer::ErrorCode::NetworkTimeout, "peer_id", "12345"},
        {"MdnsDiscovery", Core::Multiplayer::ErrorCode::ServiceUnavailable, "service_type", "_sudachi-ldn._tcp"},
        {"WiFiDirect", Core::Multiplayer::ErrorCode::PlatformPermissionDenied, "permission", "NEARBY_WIFI"},
        {"ConfigManager", Core::Multiplayer::ErrorCode::ConfigurationInvalid, "setting", "max_players"}
    };
    
    for (const auto& comp_error : component_errors) {
        ErrorInfo error;
        error.error_code = comp_error.code;
        error.component = comp_error.component;
        error.message = "Component-specific error";
        error.context[comp_error.context_key] = comp_error.context_value;
        error.timestamp = std::chrono::steady_clock::now();
        
        // This will FAIL until namespace issues are resolved
        error_handler_->ReportError(error);
    }
    
    EXPECT_EQ(received_errors.size(), component_errors.size());
    
    for (size_t i = 0; i < received_errors.size(); ++i) {
        EXPECT_EQ(received_errors[i].component, component_errors[i].component);
        EXPECT_EQ(received_errors[i].error_code, component_errors[i].code);
        EXPECT_TRUE(received_errors[i].context.count(component_errors[i].context_key) > 0);
    }
}

// CRITICAL: Test error code completeness against PRD requirements
TEST_F(ErrorCodeIntegrationTest, PRDErrorCodeCompleteness) {
    // PRD Section 7.2 specifies these error scenarios that must have error codes:
    // This test will FAIL until all required error codes are implemented
    
    std::unordered_set<Core::Multiplayer::ErrorCode> required_codes = {
        // Network connectivity issues
        Core::Multiplayer::ErrorCode::NetworkTimeout,
        Core::Multiplayer::ErrorCode::ConnectionRefused,
        Core::Multiplayer::ErrorCode::HostUnreachable,
        Core::Multiplayer::ErrorCode::ConnectionLost,
        Core::Multiplayer::ErrorCode::DNSResolutionFailed,  // MISSING
        Core::Multiplayer::ErrorCode::SSLHandshakeFailed,   // MISSING
        Core::Multiplayer::ErrorCode::ProxyAuthRequired,    // MISSING
        
        // Room management errors
        Core::Multiplayer::ErrorCode::RoomNotFound,
        Core::Multiplayer::ErrorCode::RoomFull,
        Core::Multiplayer::ErrorCode::RoomPasswordRequired,
        Core::Multiplayer::ErrorCode::InvalidRoomPassword,
        Core::Multiplayer::ErrorCode::RoomBanned,           // MISSING
        Core::Multiplayer::ErrorCode::RoomVersionMismatch,  // MISSING
        
        // P2P connection errors
        Core::Multiplayer::ErrorCode::NATTraversalFailed,   // MISSING
        Core::Multiplayer::ErrorCode::STUNServerUnavailable, // MISSING
        Core::Multiplayer::ErrorCode::TURNServerUnavailable, // MISSING
        Core::Multiplayer::ErrorCode::ICEConnectionFailed,  // MISSING
        
        // Discovery errors
        Core::Multiplayer::ErrorCode::DiscoveryFailed,
        Core::Multiplayer::ErrorCode::ServiceUnavailable,
        Core::Multiplayer::ErrorCode::MDNSQueryTimeout,     // MISSING
        Core::Multiplayer::ErrorCode::NoNetworkInterface,   // MISSING
        
        // Platform-specific errors
        Core::Multiplayer::ErrorCode::WiFiDirectNotSupported,     // MISSING
        Core::Multiplayer::ErrorCode::HotspotCreationFailed,      // MISSING
        Core::Multiplayer::ErrorCode::BluetoothNotAvailable,      // MISSING
        Core::Multiplayer::ErrorCode::LocationPermissionDenied,   // MISSING
        
        // Game session errors
        Core::Multiplayer::ErrorCode::GameVersionMismatch,        // MISSING
        Core::Multiplayer::ErrorCode::SaveDataMismatch,           // MISSING
        Core::Multiplayer::ErrorCode::PlayerLimitExceeded,        // MISSING
        Core::Multiplayer::ErrorCode::SessionExpired              // MISSING
    };
    
    // Test that each required error code exists and can be used
    for (auto code : required_codes) {
        // This will fail for missing error codes
        ErrorInfo error;
        error.error_code = code;
        error.category = GetErrorCategory(code);
        
        EXPECT_NE(error.category, ErrorCategory::Unknown) 
            << "Required error code " << static_cast<int>(code) << " is missing or not categorized";
    }
}

} // namespace Sudachi::Multiplayer::Tests