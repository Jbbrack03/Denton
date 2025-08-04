// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>

#include "../common/error_codes.h"

namespace Core::Multiplayer::ModelB {

// Forward declarations for dependency injection
class MockMdnsSocket;
class MockNetworkInterfaceProvider;
class MockMdnsConfig;

/**
 * Discovery state enumeration
 */
enum class DiscoveryState {
    Stopped = 0,
    Initializing,
    Initialized,
    Discovering,
    Advertising,
    TimedOut,
    Failed
};

/**
 * IP version enumeration
 */
enum class IPVersion {
    IPv4,
    IPv6,
    Both
};

/**
 * Game session information structure
 * Contains all the information needed to represent a discovered game session
 */
struct GameSessionInfo {
    std::string game_id;            // Game identifier (required)
    std::string version;            // Game version (required)
    int current_players;            // Current number of players (required)
    int max_players;                // Maximum number of players (required)
    bool has_password;              // Whether the session requires a password (required)
    std::string host_name;          // Host name
    std::string host_ip;            // Host IP address
    uint16_t port;                  // Host port
    std::string session_id;         // Unique session identifier
    IPVersion ip_version;           // IP version (IPv4 or IPv6)
    std::chrono::system_clock::time_point discovered_at; // When the service was discovered
    std::chrono::system_clock::time_point last_seen;     // Last time the service was seen
    
    GameSessionInfo()
        : current_players(0), max_players(0), has_password(false), 
          port(0), ip_version(IPVersion::IPv4),
          discovered_at(std::chrono::system_clock::now()),
          last_seen(std::chrono::system_clock::now()) {}
};

/**
 * mDNS Discovery Service
 * 
 * This class provides mDNS-based service discovery and advertisement functionality
 * for offline ad-hoc multiplayer gaming. It implements the discovery mechanisms
 * required by PRD Section 4.2 (Offline Ad-Hoc Multiplayer).
 * 
 * Key Features:
 * - Service discovery using mDNS queries
 * - Service advertisement with TXT records
 * - Multi-interface support (IPv4 and IPv6)
 * - Thread-safe operations
 * - Automatic service refresh and cleanup
 * 
 * This is a stub implementation - all methods will fail until implemented
 * following TDD red phase methodology.
 */
class MdnsDiscovery {
public:
    /**
     * Constructor with dependency injection for testing
     */
    MdnsDiscovery(std::shared_ptr<MockMdnsSocket> socket,
                  std::shared_ptr<MockNetworkInterfaceProvider> interface_provider,
                  std::shared_ptr<MockMdnsConfig> config);
    
    /**
     * Destructor - ensures cleanup of resources
     */
    ~MdnsDiscovery();
    
    // Core functionality - will be implemented to make tests pass
    ErrorCode Initialize();
    ErrorCode StartDiscovery();
    ErrorCode StopDiscovery();
    ErrorCode AdvertiseService(const GameSessionInfo& session_info);
    ErrorCode StopAdvertising();
    
    // State queries
    bool IsRunning() const;
    bool IsAdvertising() const;
    DiscoveryState GetState() const;
    
    // Service management
    std::vector<GameSessionInfo> GetDiscoveredServices() const;
    std::optional<GameSessionInfo> GetServiceByHostName(const std::string& host_name) const;
    void RefreshServices();
    
    // Network interface management
    std::vector<std::string> GetActiveInterfaces() const;
    
    // Callback registration
    void SetOnServiceDiscoveredCallback(std::function<void(const GameSessionInfo&)> callback);
    void SetOnServiceRemovedCallback(std::function<void(const std::string& service_name)> callback);
    void SetOnDiscoveryTimeoutCallback(std::function<void()> callback);
    void SetOnErrorCallback(std::function<void(ErrorCode, const std::string&)> callback);
    
    // Packet processing
    void ProcessIncomingPacket(const uint8_t* data, size_t size, const std::string& source_address);
    
    // Lifecycle methods
    void OnWebSocketConnected();
    void OnWebSocketDisconnected(const std::string& reason);
    
    // Heartbeat and periodic operations
    void StartHeartbeat();
    void StopHeartbeat();

private:
    // This is a stub - implementation will be added during TDD green phase
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods
    std::string ParseTxtRecordsFromPacket(const uint8_t* data, size_t size);
    bool ParseGameSessionFromTxtRecords(const std::string& txt_records, GameSessionInfo& session_info);
};

} // namespace Core::Multiplayer::ModelB