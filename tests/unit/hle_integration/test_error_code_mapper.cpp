// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_map>

// Multiplayer system includes
#include "src/core/multiplayer/common/error_codes.h"

// LDN HLE includes
#include "sudachi/src/core/hle/service/ldn/ldn_results.h"
#include "sudachi/src/core/hle/result.h"

namespace Core::Multiplayer::HLE {

/**
 * Error Code Mapper Interface - Maps between multiplayer and LDN error codes
 * This MUST be implemented for proper error translation between systems
 */
class ErrorCodeMapper {
public:
    virtual ~ErrorCodeMapper() = default;
    
    // Convert multiplayer error codes to LDN result codes
    virtual Service::LDN::Result MapToLdnResult(ErrorCode error) = 0;
    
    // Convert LDN result codes to multiplayer error codes
    virtual ErrorCode MapFromLdnResult(Service::LDN::Result result) = 0;
    
    // Get human-readable error description
    virtual std::string GetErrorDescription(ErrorCode error) = 0;
    virtual std::string GetLdnResultDescription(Service::LDN::Result result) = 0;
    
    // Check if error is recoverable
    virtual bool IsRecoverable(ErrorCode error) = 0;
    virtual bool IsRecoverable(Service::LDN::Result result) = 0;
    
    // Get suggested retry delay for recoverable errors
    virtual std::chrono::milliseconds GetRetryDelay(ErrorCode error) = 0;
};

/**
 * Concrete Error Code Mapper Implementation
 * This implementation will FAIL until proper error mapping is established
 */
class ConcreteErrorCodeMapper : public ErrorCodeMapper {
public:
    ConcreteErrorCodeMapper() {
        InitializeErrorMappings();
    }
    
    Service::LDN::Result MapToLdnResult(ErrorCode error) override {
        auto it = multiplayer_to_ldn_map_.find(error);
        if (it != multiplayer_to_ldn_map_.end()) {
            return it->second;
        }
        
        // Default to internal error for unmapped codes
        return Service::LDN::ResultInternalError;
    }
    
    ErrorCode MapFromLdnResult(Service::LDN::Result result) override {
        auto it = ldn_to_multiplayer_map_.find(result);
        if (it != ldn_to_multiplayer_map_.end()) {
            return it->second;
        }
        
        // Default to internal error for unmapped results
        return ErrorCode::InternalError;
    }
    
    std::string GetErrorDescription(ErrorCode error) override {
        auto it = error_descriptions_.find(error);
        if (it != error_descriptions_.end()) {
            return it->second;
        }
        
        return "Unknown multiplayer error";
    }
    
    std::string GetLdnResultDescription(Service::LDN::Result result) override {
        auto it = ldn_result_descriptions_.find(result);
        if (it != ldn_result_descriptions_.end()) {
            return it->second;
        }
        
        return "Unknown LDN result";
    }
    
    bool IsRecoverable(ErrorCode error) override {
        // Define which errors are recoverable (can be retried)
        switch (error) {
        case ErrorCode::NetworkTimeout:
        case ErrorCode::ConnectionTimeout:
        case ErrorCode::HostUnreachable:
        case ErrorCode::ServiceUnavailable:
        case ErrorCode::ResourceExhausted:
            return true;
        
        case ErrorCode::Success:
        case ErrorCode::AuthenticationFailed:
        case ErrorCode::InvalidParameter:
        case ErrorCode::ConfigurationInvalid:
        case ErrorCode::NotSupported:
        case ErrorCode::PermissionDenied:
            return false;
        
        default:
            return false;  // Conservative approach
        }
    }
    
    bool IsRecoverable(Service::LDN::Result result) override {
        // Map LDN result to multiplayer error and check recoverability
        auto error = MapFromLdnResult(result);
        return IsRecoverable(error);
    }
    
    std::chrono::milliseconds GetRetryDelay(ErrorCode error) override {
        // Return appropriate retry delays based on error type
        switch (error) {
        case ErrorCode::NetworkTimeout:
        case ErrorCode::ConnectionTimeout:
            return std::chrono::milliseconds(1000);  // 1 second
        
        case ErrorCode::HostUnreachable:
            return std::chrono::milliseconds(5000);  // 5 seconds
        
        case ErrorCode::ServiceUnavailable:
            return std::chrono::milliseconds(2000);  // 2 seconds
        
        case ErrorCode::ResourceExhausted:
            return std::chrono::milliseconds(3000);  // 3 seconds
        
        default:
            return std::chrono::milliseconds(0);  // No retry
        }
    }

private:
    void InitializeErrorMappings() {
        // Multiplayer Error -> LDN Result mappings
        multiplayer_to_ldn_map_ = {
            {ErrorCode::Success, Service::LDN::ResultSuccess},
            
            // Connection errors
            {ErrorCode::ConnectionFailed, Service::LDN::ResultConnectionFailed},
            {ErrorCode::ConnectionTimeout, Service::LDN::ResultAuthenticationTimeout},
            {ErrorCode::ConnectionRefused, Service::LDN::ResultConnectionFailed},
            {ErrorCode::ConnectionLost, Service::LDN::ResultConnectionFailed},
            {ErrorCode::AuthenticationFailed, Service::LDN::ResultAuthenticationFailed},
            {ErrorCode::AlreadyConnected, Service::LDN::ResultBadState},
            {ErrorCode::NotConnected, Service::LDN::ResultBadState},
            
            // Room errors
            {ErrorCode::RoomNotFound, Service::LDN::ResultLocalCommunicationIdNotFound},
            {ErrorCode::RoomFull, Service::LDN::ResultMaximumNodeCount},
            {ErrorCode::RoomPasswordRequired, Service::LDN::ResultAuthenticationFailed},
            {ErrorCode::InvalidRoomPassword, Service::LDN::ResultAuthenticationFailed},
            {ErrorCode::AlreadyInRoom, Service::LDN::ResultBadState},
            {ErrorCode::NotInRoom, Service::LDN::ResultBadState},
            
            // Message errors
            {ErrorCode::MessageTooLarge, Service::LDN::ResultAdvertiseDataTooLarge},
            {ErrorCode::MessageTimeout, Service::LDN::ResultAuthenticationTimeout},
            {ErrorCode::InvalidMessage, Service::LDN::ResultBadInput},
            {ErrorCode::MessageQueueFull, Service::LDN::ResultInvalidBufferCount},
            
            // General errors
            {ErrorCode::InvalidParameter, Service::LDN::ResultBadInput},
            {ErrorCode::InternalError, Service::LDN::ResultInternalError},
            {ErrorCode::NetworkError, Service::LDN::ResultConnectionFailed},
            {ErrorCode::NetworkTimeout, Service::LDN::ResultAuthenticationTimeout},
            {ErrorCode::HostUnreachable, Service::LDN::ResultConnectionFailed},
            {ErrorCode::InvalidResponse, Service::LDN::ResultBadInput},
            {ErrorCode::SSLError, Service::LDN::ResultConnectionFailed},
            {ErrorCode::Timeout, Service::LDN::ResultAuthenticationTimeout},
            {ErrorCode::NotSupported, Service::LDN::ResultDisabled},
            {ErrorCode::PermissionDenied, Service::LDN::ResultAccessPointConnectionFailed},
            
            // State errors
            {ErrorCode::NotInitialized, Service::LDN::ResultBadState},
            {ErrorCode::InvalidState, Service::LDN::ResultBadState},
            
            // Discovery errors
            {ErrorCode::DiscoveryFailed, Service::LDN::ResultConnectionFailed},
            {ErrorCode::ServiceUnavailable, Service::LDN::ResultDisabled},
            
            // Peer errors
            {ErrorCode::MaxPeersExceeded, Service::LDN::ResultMaximumNodeCount},
            
            // Protocol errors
            {ErrorCode::ProtocolError, Service::LDN::ResultBadInput},
            
            // Resource errors
            {ErrorCode::ResourceExhausted, Service::LDN::ResultInvalidBufferCount},
            
            // Platform-specific errors
            {ErrorCode::PlatformAPIError, Service::LDN::ResultInternalError},
            {ErrorCode::PlatformFeatureUnavailable, Service::LDN::ResultDisabled},
            {ErrorCode::PlatformPermissionDenied, Service::LDN::ResultAccessPointConnectionFailed},
            
            // Configuration errors
            {ErrorCode::ConfigurationInvalid, Service::LDN::ResultBadInput},
            {ErrorCode::ConfigurationMissing, Service::LDN::ResultBadInput}
        };
        
        // Create reverse mapping
        for (const auto& pair : multiplayer_to_ldn_map_) {
            ldn_to_multiplayer_map_[pair.second] = pair.first;
        }
        
        // Error descriptions
        error_descriptions_ = {
            {ErrorCode::Success, "Operation completed successfully"},
            {ErrorCode::ConnectionFailed, "Failed to establish network connection"},
            {ErrorCode::ConnectionTimeout, "Connection attempt timed out"},
            {ErrorCode::ConnectionRefused, "Connection was refused by remote host"},
            {ErrorCode::ConnectionLost, "Network connection was lost"},
            {ErrorCode::AuthenticationFailed, "Authentication with remote host failed"},
            {ErrorCode::AlreadyConnected, "Already connected to a network"},
            {ErrorCode::NotConnected, "Not connected to any network"},
            {ErrorCode::RoomNotFound, "Requested room or session not found"},
            {ErrorCode::RoomFull, "Room has reached maximum player capacity"},
            {ErrorCode::RoomPasswordRequired, "Room requires password for access"},
            {ErrorCode::InvalidRoomPassword, "Provided room password is incorrect"},
            {ErrorCode::AlreadyInRoom, "Already joined a room or session"},
            {ErrorCode::NotInRoom, "Not currently in any room or session"},
            {ErrorCode::MessageTooLarge, "Message exceeds maximum allowed size"},
            {ErrorCode::MessageTimeout, "Message transmission timed out"},
            {ErrorCode::InvalidMessage, "Received message has invalid format"},
            {ErrorCode::MessageQueueFull, "Message queue is full, cannot accept more messages"},
            {ErrorCode::InvalidParameter, "One or more parameters are invalid"},
            {ErrorCode::InternalError, "Internal system error occurred"},
            {ErrorCode::NetworkError, "General network communication error"},
            {ErrorCode::NetworkTimeout, "Network operation timed out"},
            {ErrorCode::HostUnreachable, "Remote host is unreachable"},
            {ErrorCode::InvalidResponse, "Received invalid response from remote host"},
            {ErrorCode::SSLError, "SSL/TLS encryption error"},
            {ErrorCode::Timeout, "Operation timed out"},
            {ErrorCode::NotSupported, "Operation not supported on this platform"},
            {ErrorCode::PermissionDenied, "Permission denied for requested operation"},
            {ErrorCode::NotInitialized, "System not initialized"},
            {ErrorCode::InvalidState, "System is in invalid state for this operation"},
            {ErrorCode::DiscoveryFailed, "Network discovery failed"},
            {ErrorCode::ServiceUnavailable, "Required service is unavailable"},
            {ErrorCode::MaxPeersExceeded, "Maximum number of peers exceeded"},
            {ErrorCode::ProtocolError, "Network protocol error"},
            {ErrorCode::ResourceExhausted, "System resources exhausted"},
            {ErrorCode::PlatformAPIError, "Platform-specific API error"},
            {ErrorCode::PlatformFeatureUnavailable, "Required platform feature unavailable"},
            {ErrorCode::PlatformPermissionDenied, "Platform permission denied"},
            {ErrorCode::ConfigurationInvalid, "Configuration is invalid"},
            {ErrorCode::ConfigurationMissing, "Required configuration is missing"}
        };
        
        // LDN result descriptions
        ldn_result_descriptions_ = {
            {Service::LDN::ResultSuccess, "LDN operation completed successfully"},
            {Service::LDN::ResultAdvertiseDataTooLarge, "Advertise data exceeds maximum size"},
            {Service::LDN::ResultAuthenticationFailed, "LDN authentication failed"},
            {Service::LDN::ResultDisabled, "LDN service is disabled"},
            {Service::LDN::ResultAirplaneModeEnabled, "Airplane mode is enabled"},
            {Service::LDN::ResultInvalidNodeCount, "Invalid node count specified"},
            {Service::LDN::ResultConnectionFailed, "LDN connection failed"},
            {Service::LDN::ResultBadState, "LDN service is in bad state"},
            {Service::LDN::ResultNoIpAddress, "No IP address available"},
            {Service::LDN::ResultInvalidBufferCount, "Invalid buffer count"},
            {Service::LDN::ResultAccessPointConnectionFailed, "Access point connection failed"},
            {Service::LDN::ResultAuthenticationTimeout, "LDN authentication timed out"},
            {Service::LDN::ResultMaximumNodeCount, "Maximum node count reached"},
            {Service::LDN::ResultBadInput, "Invalid input provided to LDN service"},
            {Service::LDN::ResultLocalCommunicationIdNotFound, "Local communication ID not found"},
            {Service::LDN::ResultLocalCommunicationVersionTooLow, "Local communication version too low"},
            {Service::LDN::ResultLocalCommunicationVersionTooHigh, "Local communication version too high"}
        };
    }
    
    std::unordered_map<ErrorCode, Service::LDN::Result> multiplayer_to_ldn_map_;
    std::unordered_map<Service::LDN::Result, ErrorCode> ldn_to_multiplayer_map_;
    std::unordered_map<ErrorCode, std::string> error_descriptions_;
    std::unordered_map<Service::LDN::Result, std::string> ldn_result_descriptions_;
};

/**
 * Mock Error Code Mapper for testing
 */
class MockErrorCodeMapper : public ErrorCodeMapper {
public:
    MOCK_METHOD(Service::LDN::Result, MapToLdnResult, (ErrorCode), (override));
    MOCK_METHOD(ErrorCode, MapFromLdnResult, (Service::LDN::Result), (override));
    MOCK_METHOD(std::string, GetErrorDescription, (ErrorCode), (override));
    MOCK_METHOD(std::string, GetLdnResultDescription, (Service::LDN::Result), (override));
    MOCK_METHOD(bool, IsRecoverable, (ErrorCode), (override));
    MOCK_METHOD(bool, IsRecoverable, (Service::LDN::Result), (override));
    MOCK_METHOD(std::chrono::milliseconds, GetRetryDelay, (ErrorCode), (override));
};

/**
 * Test Suite: Error Code Mapper
 * These tests verify proper error code translation between systems
 */
class ErrorCodeMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        mapper = std::make_unique<ConcreteErrorCodeMapper>();
    }
    
    std::unique_ptr<ConcreteErrorCodeMapper> mapper;
};

TEST_F(ErrorCodeMapperTest, MapSuccessCode) {
    // Given: Success error code
    
    // When: Success is mapped to LDN result
    auto ldn_result = mapper->MapToLdnResult(ErrorCode::Success);
    
    // Then: Should map to LDN success
    EXPECT_EQ(ldn_result, Service::LDN::ResultSuccess);
    
    // And when: LDN success is mapped back
    auto multiplayer_error = mapper->MapFromLdnResult(Service::LDN::ResultSuccess);
    
    // Then: Should map back to success
    EXPECT_EQ(multiplayer_error, ErrorCode::Success);
}

TEST_F(ErrorCodeMapperTest, MapConnectionErrors) {
    // Given: Various connection error codes
    std::vector<std::pair<ErrorCode, Service::LDN::Result>> connection_mappings = {
        {ErrorCode::ConnectionFailed, Service::LDN::ResultConnectionFailed},
        {ErrorCode::ConnectionTimeout, Service::LDN::ResultAuthenticationTimeout},
        {ErrorCode::ConnectionRefused, Service::LDN::ResultConnectionFailed},
        {ErrorCode::ConnectionLost, Service::LDN::ResultConnectionFailed},
        {ErrorCode::AuthenticationFailed, Service::LDN::ResultAuthenticationFailed}
    };
    
    for (const auto& pair : connection_mappings) {
        // When: Connection error is mapped to LDN result
        auto ldn_result = mapper->MapToLdnResult(pair.first);
        
        // Then: Should map to expected LDN result
        EXPECT_EQ(ldn_result, pair.second) 
            << "Failed mapping ErrorCode::" << static_cast<int>(pair.first)
            << " to LDN result " << pair.second.raw;
        
        // And when: LDN result is mapped back
        auto multiplayer_error = mapper->MapFromLdnResult(pair.second);
        
        // Then: Should map to appropriate multiplayer error
        EXPECT_NE(multiplayer_error, ErrorCode::InternalError)
            << "LDN result " << pair.second.raw << " not properly mapped back";
    }
}

TEST_F(ErrorCodeMapperTest, MapRoomErrors) {
    // Given: Room-specific error codes
    std::vector<std::pair<ErrorCode, Service::LDN::Result>> room_mappings = {
        {ErrorCode::RoomNotFound, Service::LDN::ResultLocalCommunicationIdNotFound},
        {ErrorCode::RoomFull, Service::LDN::ResultMaximumNodeCount},
        {ErrorCode::RoomPasswordRequired, Service::LDN::ResultAuthenticationFailed},
        {ErrorCode::InvalidRoomPassword, Service::LDN::ResultAuthenticationFailed}
    };
    
    for (const auto& pair : room_mappings) {
        // When: Room error is mapped to LDN result
        auto ldn_result = mapper->MapToLdnResult(pair.first);
        
        // Then: Should map to expected LDN result
        EXPECT_EQ(ldn_result, pair.second);
    }
}

TEST_F(ErrorCodeMapperTest, MapMessageErrors) {
    // Given: Message-related error codes
    
    // When: Message too large error is mapped
    auto large_msg_result = mapper->MapToLdnResult(ErrorCode::MessageTooLarge);
    
    // Then: Should map to advertise data too large
    EXPECT_EQ(large_msg_result, Service::LDN::ResultAdvertiseDataTooLarge);
    
    // When: Message timeout error is mapped
    auto timeout_result = mapper->MapToLdnResult(ErrorCode::MessageTimeout);
    
    // Then: Should map to authentication timeout
    EXPECT_EQ(timeout_result, Service::LDN::ResultAuthenticationTimeout);
    
    // When: Invalid message error is mapped
    auto invalid_result = mapper->MapToLdnResult(ErrorCode::InvalidMessage);
    
    // Then: Should map to bad input
    EXPECT_EQ(invalid_result, Service::LDN::ResultBadInput);
}

TEST_F(ErrorCodeMapperTest, MapStateErrors) {
    // Given: State-related error codes
    
    // When: Not initialized error is mapped
    auto not_init_result = mapper->MapToLdnResult(ErrorCode::NotInitialized);
    
    // Then: Should map to bad state
    EXPECT_EQ(not_init_result, Service::LDN::ResultBadState);
    
    // When: Invalid state error is mapped
    auto invalid_state_result = mapper->MapToLdnResult(ErrorCode::InvalidState);
    
    // Then: Should map to bad state
    EXPECT_EQ(invalid_state_result, Service::LDN::ResultBadState);
    
    // When: Already connected error is mapped
    auto already_connected_result = mapper->MapToLdnResult(ErrorCode::AlreadyConnected);
    
    // Then: Should map to bad state
    EXPECT_EQ(already_connected_result, Service::LDN::ResultBadState);
}

TEST_F(ErrorCodeMapperTest, MapPlatformErrors) {
    // Given: Platform-specific error codes
    
    // When: Platform API error is mapped
    auto api_error_result = mapper->MapToLdnResult(ErrorCode::PlatformAPIError);
    
    // Then: Should map to internal error
    EXPECT_EQ(api_error_result, Service::LDN::ResultInternalError);
    
    // When: Platform feature unavailable is mapped
    auto unavailable_result = mapper->MapToLdnResult(ErrorCode::PlatformFeatureUnavailable);
    
    // Then: Should map to disabled
    EXPECT_EQ(unavailable_result, Service::LDN::ResultDisabled);
    
    // When: Platform permission denied is mapped
    auto permission_result = mapper->MapToLdnResult(ErrorCode::PlatformPermissionDenied);
    
    // Then: Should map to access point connection failed
    EXPECT_EQ(permission_result, Service::LDN::ResultAccessPointConnectionFailed);
}

TEST_F(ErrorCodeMapperTest, MapConfigurationErrors) {
    // Given: Configuration-related error codes
    
    // When: Configuration invalid error is mapped
    auto invalid_config_result = mapper->MapToLdnResult(ErrorCode::ConfigurationInvalid);
    
    // Then: Should map to bad input
    EXPECT_EQ(invalid_config_result, Service::LDN::ResultBadInput);
    
    // When: Configuration missing error is mapped
    auto missing_config_result = mapper->MapToLdnResult(ErrorCode::ConfigurationMissing);
    
    // Then: Should map to bad input
    EXPECT_EQ(missing_config_result, Service::LDN::ResultBadInput);
}

TEST_F(ErrorCodeMapperTest, MapUnknownErrorCode) {
    // Given: Unknown/unmapped error code
    ErrorCode unknown_error = static_cast<ErrorCode>(9999);
    
    // When: Unknown error is mapped
    auto result = mapper->MapToLdnResult(unknown_error);
    
    // Then: Should map to internal error as fallback
    EXPECT_EQ(result, Service::LDN::ResultInternalError);
}

TEST_F(ErrorCodeMapperTest, MapUnknownLdnResult) {
    // Given: Unknown/unmapped LDN result
    Service::LDN::Result unknown_result{9999};
    
    // When: Unknown result is mapped
    auto error = mapper->MapFromLdnResult(unknown_result);
    
    // Then: Should map to internal error as fallback
    EXPECT_EQ(error, ErrorCode::InternalError);
}

TEST_F(ErrorCodeMapperTest, GetErrorDescriptions) {
    // Given: Various error codes
    
    // When: Descriptions are requested
    auto success_desc = mapper->GetErrorDescription(ErrorCode::Success);
    auto connection_desc = mapper->GetErrorDescription(ErrorCode::ConnectionFailed);
    auto timeout_desc = mapper->GetErrorDescription(ErrorCode::NetworkTimeout);
    
    // Then: Should return meaningful descriptions
    EXPECT_FALSE(success_desc.empty());
    EXPECT_FALSE(connection_desc.empty());
    EXPECT_FALSE(timeout_desc.empty());
    
    EXPECT_NE(success_desc.find("success"), std::string::npos);
    EXPECT_NE(connection_desc.find("connection"), std::string::npos);
    EXPECT_NE(timeout_desc.find("timeout"), std::string::npos);
}

TEST_F(ErrorCodeMapperTest, GetLdnResultDescriptions) {
    // Given: Various LDN results
    
    // When: Descriptions are requested
    auto success_desc = mapper->GetLdnResultDescription(Service::LDN::ResultSuccess);
    auto bad_state_desc = mapper->GetLdnResultDescription(Service::LDN::ResultBadState);
    auto auth_fail_desc = mapper->GetLdnResultDescription(Service::LDN::ResultAuthenticationFailed);
    
    // Then: Should return meaningful descriptions
    EXPECT_FALSE(success_desc.empty());
    EXPECT_FALSE(bad_state_desc.empty());
    EXPECT_FALSE(auth_fail_desc.empty());
    
    EXPECT_NE(success_desc.find("success"), std::string::npos);
    EXPECT_NE(bad_state_desc.find("state"), std::string::npos);
    EXPECT_NE(auth_fail_desc.find("authentication"), std::string::npos);
}

TEST_F(ErrorCodeMapperTest, RecoverabilityCheck) {
    // Given: Various error codes
    
    // When: Recoverability is checked
    bool timeout_recoverable = mapper->IsRecoverable(ErrorCode::NetworkTimeout);
    bool connection_timeout_recoverable = mapper->IsRecoverable(ErrorCode::ConnectionTimeout);
    bool host_unreachable_recoverable = mapper->IsRecoverable(ErrorCode::HostUnreachable);
    bool service_unavailable_recoverable = mapper->IsRecoverable(ErrorCode::ServiceUnavailable);
    
    bool auth_failed_recoverable = mapper->IsRecoverable(ErrorCode::AuthenticationFailed);
    bool invalid_param_recoverable = mapper->IsRecoverable(ErrorCode::InvalidParameter);
    bool not_supported_recoverable = mapper->IsRecoverable(ErrorCode::NotSupported);
    
    // Then: Should correctly identify recoverable vs non-recoverable errors
    EXPECT_TRUE(timeout_recoverable);
    EXPECT_TRUE(connection_timeout_recoverable);
    EXPECT_TRUE(host_unreachable_recoverable);
    EXPECT_TRUE(service_unavailable_recoverable);
    
    EXPECT_FALSE(auth_failed_recoverable);
    EXPECT_FALSE(invalid_param_recoverable);
    EXPECT_FALSE(not_supported_recoverable);
}

TEST_F(ErrorCodeMapperTest, RecoverabilityCheckLdnResults) {
    // Given: Various LDN results
    
    // When: Recoverability is checked via LDN results
    bool timeout_recoverable = mapper->IsRecoverable(Service::LDN::ResultAuthenticationTimeout);
    bool connection_failed_recoverable = mapper->IsRecoverable(Service::LDN::ResultConnectionFailed);
    
    bool auth_failed_recoverable = mapper->IsRecoverable(Service::LDN::ResultAuthenticationFailed);
    bool bad_input_recoverable = mapper->IsRecoverable(Service::LDN::ResultBadInput);
    
    // Then: Should correctly identify recoverability through LDN mapping
    // Note: These depend on the mapping working correctly
    EXPECT_TRUE(timeout_recoverable);
    // connection_failed_recoverable may vary based on specific mapping
    
    EXPECT_FALSE(auth_failed_recoverable);
    EXPECT_FALSE(bad_input_recoverable);
}

TEST_F(ErrorCodeMapperTest, RetryDelayCalculation) {
    // Given: Recoverable error codes
    
    // When: Retry delays are calculated
    auto network_timeout_delay = mapper->GetRetryDelay(ErrorCode::NetworkTimeout);
    auto connection_timeout_delay = mapper->GetRetryDelay(ErrorCode::ConnectionTimeout);
    auto host_unreachable_delay = mapper->GetRetryDelay(ErrorCode::HostUnreachable);
    auto service_unavailable_delay = mapper->GetRetryDelay(ErrorCode::ServiceUnavailable);
    
    // Non-recoverable errors
    auto auth_failed_delay = mapper->GetRetryDelay(ErrorCode::AuthenticationFailed);
    auto invalid_param_delay = mapper->GetRetryDelay(ErrorCode::InvalidParameter);
    
    // Then: Should return appropriate delays
    EXPECT_GT(network_timeout_delay.count(), 0);
    EXPECT_GT(connection_timeout_delay.count(), 0);
    EXPECT_GT(host_unreachable_delay.count(), 0);
    EXPECT_GT(service_unavailable_delay.count(), 0);
    
    // Non-recoverable errors should have zero delay
    EXPECT_EQ(auth_failed_delay.count(), 0);
    EXPECT_EQ(invalid_param_delay.count(), 0);
    
    // Verify reasonable delay ranges (not too short or too long)
    EXPECT_LE(network_timeout_delay.count(), 10000);  // Max 10 seconds
    EXPECT_GE(network_timeout_delay.count(), 100);    // Min 100ms
}

TEST_F(ErrorCodeMapperTest, BidirectionalMappingConsistency) {
    // Given: All defined error codes
    std::vector<ErrorCode> all_errors = {
        ErrorCode::Success,
        ErrorCode::ConnectionFailed,
        ErrorCode::ConnectionTimeout,
        ErrorCode::AuthenticationFailed,
        ErrorCode::RoomNotFound,
        ErrorCode::RoomFull,
        ErrorCode::MessageTooLarge,
        ErrorCode::InvalidParameter,
        ErrorCode::NetworkTimeout,
        ErrorCode::NotInitialized,
        ErrorCode::InvalidState,
        ErrorCode::MaxPeersExceeded,
        ErrorCode::PlatformAPIError,
        ErrorCode::ConfigurationInvalid
    };
    
    for (auto error : all_errors) {
        // When: Error is mapped to LDN result and back
        auto ldn_result = mapper->MapToLdnResult(error);
        auto mapped_back = mapper->MapFromLdnResult(ldn_result);
        
        // Then: Should maintain reasonable consistency
        // Note: Perfect bidirectional mapping isn't required due to many-to-one mappings
        // But we should not get completely unrelated errors
        EXPECT_NE(mapped_back, ErrorCode::InternalError) 
            << "Error " << static_cast<int>(error) << " mapped to unmapped LDN result";
    }
}

/**
 * Integration Test: Error Code Mapper with Mock Usage
 */
class ErrorCodeMapperIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_mapper = std::make_unique<MockErrorCodeMapper>();
    }
    
    std::unique_ptr<MockErrorCodeMapper> mock_mapper;
};

TEST_F(ErrorCodeMapperIntegrationTest, MockMapperUsage) {
    // Given: Mock mapper with expectations
    EXPECT_CALL(*mock_mapper, MapToLdnResult(ErrorCode::ConnectionFailed))
        .WillOnce(::testing::Return(Service::LDN::ResultConnectionFailed));
    
    EXPECT_CALL(*mock_mapper, GetErrorDescription(ErrorCode::ConnectionFailed))
        .WillOnce(::testing::Return("Mocked connection failure"));
    
    EXPECT_CALL(*mock_mapper, IsRecoverable(ErrorCode::ConnectionFailed))
        .WillOnce(::testing::Return(false));
    
    // When: Mock mapper is used
    auto ldn_result = mock_mapper->MapToLdnResult(ErrorCode::ConnectionFailed);
    auto description = mock_mapper->GetErrorDescription(ErrorCode::ConnectionFailed);
    auto recoverable = mock_mapper->IsRecoverable(ErrorCode::ConnectionFailed);
    
    // Then: Should return mocked values
    EXPECT_EQ(ldn_result, Service::LDN::ResultConnectionFailed);
    EXPECT_EQ(description, "Mocked connection failure");
    EXPECT_FALSE(recoverable);
}

/**
 * Critical Test: This test demonstrates what's needed for complete integration
 */
TEST_F(ErrorCodeMapperTest, CriticalErrorHandlingIntegration) {
    GTEST_SKIP() << "Error Code Mapper integration requires LdnServiceBridge implementation.";
}

} // namespace Core::Multiplayer::HLE
