// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    Timeout,
    NotSupported,
    PermissionDenied
};

} // namespace Core::Multiplayer