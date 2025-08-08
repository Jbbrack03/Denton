// SPDX-FileCopyrightText: 2025 sudachi Emulator Project  
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <cstdint>

namespace Core::Multiplayer::ModelA::Test {

/**
 * Mock interface for relay connection
 * Used to test RelayClient functionality without actual network connections
 */
class MockRelayConnection {
public:
    virtual ~MockRelayConnection() = default;
    
    // Connection management
    MOCK_METHOD(void, Connect, (const std::string& host, uint16_t port), ());
    MOCK_METHOD(void, Disconnect, (const std::string& reason), ());
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(std::string, GetEndpoint, (), (const));
    
    // Authentication
    MOCK_METHOD(void, SetAuthToken, (const std::string& jwt_token), ());
    MOCK_METHOD(bool, Authenticate, (), ());
    
    // Session management
    MOCK_METHOD(bool, CreateSession, (uint32_t session_token), ());
    MOCK_METHOD(bool, JoinSession, (uint32_t session_token), ());
    MOCK_METHOD(void, LeaveSession, (), ());
    MOCK_METHOD(uint32_t, GetCurrentSession, (), (const));
    
    // Data transmission
    MOCK_METHOD(bool, SendData, (const std::vector<uint8_t>& data), ());
    MOCK_METHOD(void, SetOnDataReceived, (std::function<void(const std::vector<uint8_t>&)> callback), ());
    
    // Event callbacks
    MOCK_METHOD(void, SetOnConnected, (std::function<void()> callback), ());
    MOCK_METHOD(void, SetOnDisconnected, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnError, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnSessionCreated, (std::function<void(uint32_t)> callback), ());
    MOCK_METHOD(void, SetOnSessionJoined, (std::function<void(uint32_t)> callback), ());
    
    // Bandwidth and performance
    MOCK_METHOD(uint64_t, GetBytesSent, (), (const));
    MOCK_METHOD(uint64_t, GetBytesReceived, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetLatency, (), (const));
    MOCK_METHOD(bool, IsBandwidthLimited, (), (const));
    MOCK_METHOD(void, SetBandwidthLimit, (uint64_t bytes_per_second), ());
};

/**
 * Mock interface for relay protocol handler
 * Tests relay protocol header serialization/deserialization
 */
class MockRelayProtocolHandler {
public:
    virtual ~MockRelayProtocolHandler() = default;
    
    // Header serialization
    MOCK_METHOD(std::vector<uint8_t>, SerializeHeader, 
                (uint32_t session_token, uint16_t payload_size, uint8_t flags, uint32_t sequence_num), ());
    MOCK_METHOD(bool, DeserializeHeader, 
                (const std::vector<uint8_t>& data, uint32_t& session_token, 
                 uint16_t& payload_size, uint8_t& flags, uint32_t& sequence_num), ());
    
    // Message handling
    MOCK_METHOD(std::vector<uint8_t>, CreateDataMessage, 
                (uint32_t session_token, const std::vector<uint8_t>& payload, uint32_t sequence_num), ());
    MOCK_METHOD(std::vector<uint8_t>, CreateControlMessage, 
                (uint32_t session_token, uint8_t control_type, uint32_t sequence_num), ());
    
    // Validation
    MOCK_METHOD(bool, ValidateHeader, (const std::vector<uint8_t>& header_data), ());
    MOCK_METHOD(bool, ValidateMessage, (const std::vector<uint8_t>& message_data), ());
    MOCK_METHOD(size_t, GetHeaderSize, (), (const));
    MOCK_METHOD(size_t, GetMaxPayloadSize, (), (const));
};

/**
 * Mock interface for bandwidth limiter
 * Tests bandwidth limiting functionality for relay connections
 */
class MockBandwidthLimiter {
public:
    virtual ~MockBandwidthLimiter() = default;
    
    // Rate limiting
    MOCK_METHOD(bool, CanSendBytes, (size_t byte_count), ());
    MOCK_METHOD(void, ConsumeBytes, (size_t byte_count), ());
    MOCK_METHOD(std::chrono::milliseconds, GetNextAvailableTime, (size_t byte_count), ());
    
    // Configuration
    MOCK_METHOD(void, SetBandwidthLimit, (uint64_t bytes_per_second), ());
    MOCK_METHOD(uint64_t, GetBandwidthLimit, (), (const));
    MOCK_METHOD(void, SetBurstSize, (size_t burst_bytes), ());
    MOCK_METHOD(size_t, GetBurstSize, (), (const));
    
    // Statistics
    MOCK_METHOD(uint64_t, GetTotalBytesConsumed, (), (const));
    MOCK_METHOD(double, GetCurrentUtilization, (), (const));
    MOCK_METHOD(void, Reset, (), ());
};

/**
 * Mock interface for P2P connection
 * Used to test P2P to relay fallback mechanism
 */
class MockP2PConnection {
public:
    virtual ~MockP2PConnection() = default;
    
    // Connection management
    MOCK_METHOD(void, InitiateConnection, (const std::string& peer_id), ());
    MOCK_METHOD(void, AcceptConnection, (const std::string& peer_id), ());
    MOCK_METHOD(void, CloseConnection, (), ());
    MOCK_METHOD(bool, IsConnected, (), (const));
    
    // NAT traversal
    MOCK_METHOD(bool, PerformNATTraversal, (), ());
    MOCK_METHOD(bool, IsNATTraversalSuccessful, (), (const));
    MOCK_METHOD(std::string, GetConnectionType, (), (const)); // "direct", "stun", "turn", "failed"
    
    // Data transmission
    MOCK_METHOD(bool, SendData, (const std::vector<uint8_t>& data), ());
    MOCK_METHOD(void, SetOnDataReceived, (std::function<void(const std::vector<uint8_t>&)> callback), ());
    
    // Event callbacks
    MOCK_METHOD(void, SetOnConnected, (std::function<void()> callback), ());
    MOCK_METHOD(void, SetOnDisconnected, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnError, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnConnectionFailed, (std::function<void(const std::string&)> callback), ());
    
    // Performance metrics
    MOCK_METHOD(std::chrono::milliseconds, GetLatency, (), (const));
    MOCK_METHOD(uint64_t, GetBytesSent, (), (const));
    MOCK_METHOD(uint64_t, GetBytesReceived, (), (const));
};

/**
 * Mock interface for relay server selection
 * Tests relay server discovery and selection logic
 */
class MockRelayServerSelector {
public:
    virtual ~MockRelayServerSelector() = default;
    
    // Server discovery
    MOCK_METHOD(std::vector<std::string>, GetAvailableServers, (), ());
    MOCK_METHOD(std::string, SelectBestServer, (), ());
    MOCK_METHOD(std::string, SelectServerByRegion, (const std::string& region), ());
    
    // Server health
    MOCK_METHOD(bool, IsServerHealthy, (const std::string& server_endpoint), ());
    MOCK_METHOD(std::chrono::milliseconds, GetServerLatency, (const std::string& server_endpoint), ());
    MOCK_METHOD(double, GetServerLoad, (const std::string& server_endpoint), ());
    
    // Failover management
    MOCK_METHOD(void, MarkServerUnhealthy, (const std::string& server_endpoint), ());
    MOCK_METHOD(std::string, GetNextAvailableServer, (const std::string& failed_server), ());
    MOCK_METHOD(void, RefreshServerList, (), ());
};

} // namespace Core::Multiplayer::ModelA::Test