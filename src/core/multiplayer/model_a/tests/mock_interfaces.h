// SPDX-FileCopyrightText: 2025 sudachi Emulator Project  
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <functional>
#include <chrono>

namespace Core::Multiplayer::ModelA::Test {

/**
 * Mock WebSocket connection interface
 * This interface will be implemented by the actual WebSocket wrapper
 */
class MockWebSocketInterface {
public:
    virtual ~MockWebSocketInterface() = default;
    
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

/**
 * Mock configuration interface
 * This interface will be implemented by the actual configuration provider
 */
class MockConfigInterface {
public:
    virtual ~MockConfigInterface() = default;
    
    // Server configuration
    virtual std::string GetRoomServerUrl() const = 0;
    virtual std::string GetAuthToken() const = 0;
    
    // Connection timeouts
    virtual std::chrono::milliseconds GetConnectionTimeout() const = 0;
    virtual std::chrono::milliseconds GetHeartbeatInterval() const = 0;
    virtual std::chrono::milliseconds GetMessageTimeout() const = 0;
    
    // Reconnection settings
    virtual bool IsAutoReconnectEnabled() const = 0;
    virtual int GetMaxReconnectAttempts() const = 0;
    virtual std::chrono::milliseconds GetReconnectBaseDelay() const = 0;
    virtual double GetReconnectBackoffMultiplier() const = 0;
    virtual std::chrono::milliseconds GetMaxReconnectDelay() const = 0;
    virtual bool ShouldReconnectOnError(const std::string& error_type) const = 0;
    
    // Performance settings
    virtual int GetMaxConcurrentMessages() const = 0;
    virtual size_t GetMessageQueueSize() const = 0;
};

} // namespace Core::Multiplayer::ModelA::Test
