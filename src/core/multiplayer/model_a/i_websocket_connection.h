// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <functional>

namespace Core::Multiplayer::ModelA {

/**
 * WebSocket connection interface
 * This interface abstracts WebSocket connectivity for testing and implementation flexibility
 */
class IWebSocketConnection {
public:
    virtual ~IWebSocketConnection() = default;
    
    // Connection management
    virtual void Connect(const std::string& uri) = 0;
    virtual void Disconnect(const std::string& reason = "") = 0;
    virtual bool IsConnected() const = 0;
    virtual std::string GetUri() const = 0;
    
    // Authentication
    virtual void SetAuthToken(const std::string& token) = 0;
    
    // Message handling
    virtual void SendMessage(const std::string& message) = 0;
    virtual void SetOnMessageCallback(std::function<void(const std::string&)> callback) = 0;
    
    // Event callbacks
    virtual void SetOnConnectCallback(std::function<void()> callback) = 0;
    virtual void SetOnDisconnectCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void SetOnErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

} // namespace Core::Multiplayer::ModelA
