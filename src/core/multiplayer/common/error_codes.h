// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Core::Multiplayer {

/**
 * Error codes for multiplayer operations
 */
enum class ErrorCode {
    Success = 0,
    
    // Connection errors
    ConnectionFailed,
    ConnectionTimeout,
    ConnectionRefused,
    ConnectionLost,
    AuthenticationFailed,
    AlreadyConnected,
    NotConnected,
    
    // Room errors
    RoomNotFound,
    RoomFull,
    RoomPasswordRequired,
    InvalidRoomPassword,
    AlreadyInRoom,
    NotInRoom,
    
    // Message errors
    MessageTooLarge,
    MessageTimeout,
    InvalidMessage,
    MessageQueueFull,
    
    // General errors
    InvalidParameter,
    InternalError,
    NetworkError,
    NetworkTimeout,
    HostUnreachable,
    InvalidResponse,
    SSLError,
    Timeout,
    NotSupported,
    PermissionDenied,
    
    // State errors
    NotInitialized,
    InvalidState,
    
    // Discovery errors
    DiscoveryFailed,
    ServiceUnavailable,
    
    // Peer errors
    MaxPeersExceeded,
    
    // Protocol errors
    ProtocolError,
    
    // Resource errors  
    ResourceExhausted,
    
    // Platform-specific errors (for Windows Mobile Hotspot)
    PlatformAPIError,
    PlatformFeatureUnavailable,
    PlatformPermissionDenied,
    
    // Configuration errors
    ConfigurationInvalid,
    ConfigurationMissing
};

} // namespace Core::Multiplayer