// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "error_code_mapper.h"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "sudachi/src/core/hle/result.h"
#include "sudachi/src/core/hle/service/ldn/ldn_results.h"

namespace Core::Multiplayer::HLE {

namespace {

struct ErrorToLdn {
  ErrorCode error;
  Service::LDN::Result result;
};

struct LdnToError {
  Service::LDN::Result result;
  ErrorCode error;
};

struct ErrorDesc {
  ErrorCode error;
  std::string_view description;
};

struct LdnDesc {
  Service::LDN::Result result;
  std::string_view description;
};

// Multiplayer Error -> LDN Result mappings
constexpr std::array<ErrorToLdn, 40> multiplayer_to_ldn_map_{
    {{ErrorCode::Success, Service::LDN::ResultSuccess},

     // Connection errors
     {ErrorCode::ConnectionFailed, Service::LDN::ResultConnectionFailed},
     {ErrorCode::ConnectionTimeout, Service::LDN::ResultAuthenticationTimeout},
     {ErrorCode::ConnectionRefused, Service::LDN::ResultConnectionFailed},
     {ErrorCode::ConnectionLost, Service::LDN::ResultConnectionFailed},
     {ErrorCode::AuthenticationFailed,
      Service::LDN::ResultAuthenticationFailed},
     {ErrorCode::AlreadyConnected, Service::LDN::ResultBadState},
     {ErrorCode::NotConnected, Service::LDN::ResultBadState},

     // Room errors
     {ErrorCode::RoomNotFound,
      Service::LDN::ResultLocalCommunicationIdNotFound},
     {ErrorCode::RoomFull, Service::LDN::ResultMaximumNodeCount},
     {ErrorCode::RoomPasswordRequired,
      Service::LDN::ResultAuthenticationFailed},
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
     {ErrorCode::PermissionDenied,
      Service::LDN::ResultAccessPointConnectionFailed},

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
     {ErrorCode::PlatformPermissionDenied,
      Service::LDN::ResultAccessPointConnectionFailed},

     // Configuration errors
     {ErrorCode::ConfigurationInvalid, Service::LDN::ResultBadInput},
     {ErrorCode::ConfigurationMissing, Service::LDN::ResultBadInput}}};

// Reverse mapping from LDN Result -> Multiplayer Error
constexpr std::array<LdnToError, 13> ldn_to_multiplayer_map_{
    {{Service::LDN::ResultSuccess, ErrorCode::Success},
     {Service::LDN::ResultConnectionFailed, ErrorCode::ConnectionFailed},
     {Service::LDN::ResultAuthenticationTimeout, ErrorCode::ConnectionTimeout},
     {Service::LDN::ResultAuthenticationFailed,
      ErrorCode::AuthenticationFailed},
     {Service::LDN::ResultBadState, ErrorCode::AlreadyConnected},
     {Service::LDN::ResultLocalCommunicationIdNotFound,
      ErrorCode::RoomNotFound},
     {Service::LDN::ResultMaximumNodeCount, ErrorCode::RoomFull},
     {Service::LDN::ResultAdvertiseDataTooLarge, ErrorCode::MessageTooLarge},
     {Service::LDN::ResultBadInput, ErrorCode::InvalidMessage},
     {Service::LDN::ResultInvalidBufferCount, ErrorCode::MessageQueueFull},
     {Service::LDN::ResultInternalError, ErrorCode::InternalError},
     {Service::LDN::ResultDisabled, ErrorCode::NotSupported},
     {Service::LDN::ResultAccessPointConnectionFailed,
      ErrorCode::PermissionDenied}}};

// Human readable descriptions for multiplayer errors
constexpr std::array<ErrorDesc, 40> error_descriptions_{
    {{ErrorCode::Success, "Operation completed successfully"},
     {ErrorCode::ConnectionFailed, "Failed to establish network connection"},
     {ErrorCode::ConnectionTimeout, "Connection attempt timed out"},
     {ErrorCode::ConnectionRefused, "Connection was refused by remote host"},
     {ErrorCode::ConnectionLost, "Network connection was lost"},
     {ErrorCode::AuthenticationFailed,
      "Authentication with remote host failed"},
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
     {ErrorCode::MessageQueueFull,
      "Message queue is full, cannot accept more messages"},
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
     {ErrorCode::PlatformFeatureUnavailable,
      "Required platform feature unavailable"},
     {ErrorCode::PlatformPermissionDenied, "Platform permission denied"},
     {ErrorCode::ConfigurationInvalid, "Configuration is invalid"},
     {ErrorCode::ConfigurationMissing, "Required configuration is missing"}}};

// Human readable descriptions for LDN results
constexpr std::array<LdnDesc, 18> ldn_result_descriptions_{
    {{Service::LDN::ResultSuccess, "LDN operation completed successfully"},
     {Service::LDN::ResultAdvertiseDataTooLarge,
      "Advertise data exceeds maximum size"},
     {Service::LDN::ResultAuthenticationFailed, "LDN authentication failed"},
     {Service::LDN::ResultDisabled, "LDN service is disabled"},
     {Service::LDN::ResultAirplaneModeEnabled, "Airplane mode is enabled"},
     {Service::LDN::ResultInvalidNodeCount, "Invalid node count specified"},
     {Service::LDN::ResultConnectionFailed, "LDN connection failed"},
     {Service::LDN::ResultBadState, "LDN service is in bad state"},
     {Service::LDN::ResultNoIpAddress, "No IP address available"},
     {Service::LDN::ResultInvalidBufferCount, "Invalid buffer count"},
     {Service::LDN::ResultAccessPointConnectionFailed,
      "Access point connection failed"},
     {Service::LDN::ResultAuthenticationTimeout,
      "LDN authentication timed out"},
     {Service::LDN::ResultMaximumNodeCount, "Maximum node count reached"},
     {Service::LDN::ResultBadInput, "Invalid input provided to LDN service"},
     {Service::LDN::ResultLocalCommunicationIdNotFound,
      "Local communication ID not found"},
     {Service::LDN::ResultLocalCommunicationVersionTooLow,
      "Local communication version too low"},
     {Service::LDN::ResultLocalCommunicationVersionTooHigh,
      "Local communication version too high"},
     {Service::LDN::ResultInternalError, "Internal LDN error occurred"}}};

} // namespace

/**
 * Concrete Error Code Mapper Implementation
 */
class ConcreteErrorCodeMapper : public ErrorCodeMapper {
public:
  ConcreteErrorCodeMapper() = default;

  Service::LDN::Result MapToLdnResult(ErrorCode error) override {
    std::span<const ErrorToLdn> map{multiplayer_to_ldn_map_};
    auto it =
        std::find_if(map.begin(), map.end(), [error](const ErrorToLdn &entry) {
          return entry.error == error;
        });
    if (it != map.end()) {
      return it->result;
    }
    return Service::LDN::ResultInternalError;
  }

  ErrorCode MapFromLdnResult(Service::LDN::Result result) override {
    std::span<const LdnToError> map{ldn_to_multiplayer_map_};
    auto it =
        std::find_if(map.begin(), map.end(), [result](const LdnToError &entry) {
          return entry.result == result;
        });
    if (it != map.end()) {
      return it->error;
    }
    return ErrorCode::InternalError;
  }

  std::string GetErrorDescription(ErrorCode error) override {
    std::span<const ErrorDesc> map{error_descriptions_};
    auto it =
        std::find_if(map.begin(), map.end(), [error](const ErrorDesc &entry) {
          return entry.error == error;
        });
    if (it != map.end()) {
      return std::string{it->description};
    }
    return "Unknown multiplayer error";
  }

  std::string GetLdnResultDescription(Service::LDN::Result result) override {
    std::span<const LdnDesc> map{ldn_result_descriptions_};
    auto it =
        std::find_if(map.begin(), map.end(), [result](const LdnDesc &entry) {
          return entry.result == result;
        });
    if (it != map.end()) {
      return std::string{it->description};
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
      return false; // Conservative approach
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
      return std::chrono::milliseconds(1000); // 1 second

    case ErrorCode::HostUnreachable:
      return std::chrono::milliseconds(5000); // 5 seconds

    case ErrorCode::ServiceUnavailable:
      return std::chrono::milliseconds(2000); // 2 seconds

    case ErrorCode::ResourceExhausted:
      return std::chrono::milliseconds(3000); // 3 seconds

    default:
      return std::chrono::milliseconds(0); // No retry
    }
  }
};

} // namespace Core::Multiplayer::HLE
