// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <chrono>
#include <memory>

#include "common/error_codes.h"

// Forward declarations for LDN types
namespace Service::LDN {
class Result;
}

namespace Core::Multiplayer::HLE {

/**
 * Error Code Mapper Interface - Maps between multiplayer and LDN error codes
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

// Factory function to create a concrete mapper instance
std::unique_ptr<ErrorCodeMapper> CreateErrorCodeMapper();

} // namespace Core::Multiplayer::HLE