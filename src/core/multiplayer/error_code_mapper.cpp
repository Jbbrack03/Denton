// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "error_code_mapper.h"

#include <unordered_map>

#include "sudachi/src/core/hle/service/ldn/ldn_results.h"
#include "sudachi/src/core/hle/result.h"

namespace Core::Multiplayer::HLE {

/**
 * Concrete Error Code Mapper Implementation
 * Minimal implementation to make tests pass
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
        // Minimal mapping to make tests pass
        // Full mapping will be implemented as needed
        multiplayer_to_ldn_map_ = {
            {ErrorCode::Success, ResultSuccess},
            {ErrorCode::ConnectionFailed, Service::LDN::ResultConnectionFailed},
            {ErrorCode::ConnectionTimeout, Service::LDN::ResultAuthenticationTimeout},
            {ErrorCode::AuthenticationFailed, Service::LDN::ResultAuthenticationFailed},
            {ErrorCode::MessageTooLarge, Service::LDN::ResultAdvertiseDataTooLarge},
            {ErrorCode::InvalidParameter, Service::LDN::ResultBadInput},
            {ErrorCode::InvalidState, Service::LDN::ResultBadState},
            {ErrorCode::MaxPeersExceeded, Service::LDN::ResultMaximumNodeCount},
            {ErrorCode::InternalError, Service::LDN::ResultInternalError}
        };
        
        // Create reverse mapping
        for (const auto& pair : multiplayer_to_ldn_map_) {
            ldn_to_multiplayer_map_[pair.second] = pair.first;
        }
        
        // Minimal error descriptions
        error_descriptions_ = {
            {ErrorCode::Success, "Operation completed successfully"},
            {ErrorCode::ConnectionFailed, "Failed to establish network connection"},
            {ErrorCode::ConnectionTimeout, "Connection attempt timed out"},
            {ErrorCode::AuthenticationFailed, "Authentication with remote host failed"},
            {ErrorCode::MessageTooLarge, "Message exceeds maximum allowed size"},
            {ErrorCode::InvalidParameter, "One or more parameters are invalid"},
            {ErrorCode::InvalidState, "System is in invalid state for this operation"},
            {ErrorCode::MaxPeersExceeded, "Maximum number of peers exceeded"},
            {ErrorCode::InternalError, "Internal system error occurred"}
        };
        
        // Minimal LDN result descriptions
        ldn_result_descriptions_ = {
            {Service::LDN::ResultSuccess, "LDN operation completed successfully"},
            {Service::LDN::ResultConnectionFailed, "LDN connection failed"},
            {Service::LDN::ResultAuthenticationTimeout, "LDN authentication timed out"},
            {Service::LDN::ResultAuthenticationFailed, "LDN authentication failed"},
            {Service::LDN::ResultAdvertiseDataTooLarge, "Advertise data exceeds maximum size"},
            {Service::LDN::ResultBadInput, "Invalid input provided to LDN service"},
            {Service::LDN::ResultBadState, "LDN service is in bad state"},
            {Service::LDN::ResultMaximumNodeCount, "Maximum node count reached"},
            {Service::LDN::ResultInternalError, "Internal LDN error occurred"}
        };
    }
    
    std::unordered_map<ErrorCode, Service::LDN::Result> multiplayer_to_ldn_map_;
    std::unordered_map<Service::LDN::Result, ErrorCode> ldn_to_multiplayer_map_;
    std::unordered_map<ErrorCode, std::string> error_descriptions_;
    std::unordered_map<Service::LDN::Result, std::string> ldn_result_descriptions_;
};

} // namespace Core::Multiplayer::HLE