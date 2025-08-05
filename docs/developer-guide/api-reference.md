# Sudachi Multiplayer API Reference

## Overview

This document provides comprehensive API documentation for the Sudachi Multiplayer system. The API is designed in layers, from the high-level HLE interface that games interact with, down to the low-level platform-specific implementations.

## API Layers

```
┌─────────────────────────────────────────┐
│              Game Layer                 │
│        (Nintendo Switch Games)          │
└─────────────────┬───────────────────────┘
                  │ IPC Calls
┌─────────────────▼───────────────────────┐
│            HLE nn::ldn API              │
│          (LdnServiceBridge)             │
└─────────────────┬───────────────────────┘
                  │ Backend Interface
┌─────────────────▼───────────────────────┐
│        Multiplayer Backend API          │
│      (IMultiplayerBackend)              │
└─────────────────┬───────────────────────┘
                  │ Implementation
┌─────────────────▼───────────────────────┐
│     Model A/B Implementation APIs       │
│  (RoomClient, P2PNetwork, MdnsDiscovery) │
└─────────────────────────────────────────┘
```

## Core Types and Enumerations

### Error Codes

```cpp
namespace Core::Multiplayer {
    enum class ErrorCode : uint32_t {
        Success = 0,
        
        // Connection errors
        ConnectionFailed = 1001,
        ConnectionTimeout = 1002,
        ConnectionRefused = 1003,
        ConnectionLost = 1004,
        AuthenticationFailed = 1005,
        
        // Session errors
        RoomNotFound = 2001,
        RoomFull = 2002,
        RoomPasswordRequired = 2003,
        InvalidRoomPassword = 2004,
        
        // Network errors
        NetworkError = 3001,
        NetworkTimeout = 3002,
        HostUnreachable = 3003,
        SSLError = 3004,
        
        // Platform errors
        PlatformAPIError = 4001,
        PlatformFeatureUnavailable = 4002,
        PlatformPermissionDenied = 4003,
        
        // Configuration errors
        ConfigurationInvalid = 5001,
        ConfigurationMissing = 5002
    };
    
    // Error handling utilities
    std::string GetErrorDescription(ErrorCode error);
    bool IsRetryableError(ErrorCode error);
    ErrorCategory GetErrorCategory(ErrorCode error);
}
```

### Multiplayer Types

```cpp
namespace Core::Multiplayer {
    enum class MultiplayerMode {
        Internet,      // Model A: Internet-based multiplayer
        Offline,       // Model B: Offline ad-hoc multiplayer
        Auto          // Automatic selection based on configuration
    };
    
    enum class SessionState {
        Idle,
        Creating,
        Hosting,
        Scanning,
        Connecting,
        Connected,
        Disconnecting,
        Error
    };
    
    enum class PlayerRole {
        Host,
        Client,
        Observer
    };
    
    using PlayerId = uint8_t;
    using SessionId = uint32_t;
    using GameId = uint64_t;
}
```

### Network Information Structures

```cpp
namespace Core::Multiplayer {
    struct NetworkConfig {
        GameId game_id;
        std::string game_version;
        std::string session_name;
        std::string password;  // Optional
        uint8_t max_players;
        bool is_public;
        std::map<std::string, std::string> custom_attributes;
    };
    
    struct NetworkInfo {
        SessionId session_id;
        GameId game_id;
        std::string game_version;
        std::string session_name;
        std::string host_name;
        uint8_t current_players;
        uint8_t max_players;
        bool has_password;
        bool is_local;  // true for Model B sessions
        std::chrono::milliseconds latency;
        std::map<std::string, std::string> attributes;
    };
    
    struct PlayerInfo {
        PlayerId player_id;
        std::string display_name;
        PlayerRole role;
        bool is_ready;
        std::chrono::milliseconds last_seen;
    };
}
```

## HLE nn::ldn API

### LdnServiceBridge Interface

The primary interface that emulates Nintendo's nn::ldn service:

```cpp
namespace Core::Multiplayer {
    class LdnServiceBridge {
    public:
        /**
         * Initialize the multiplayer service
         * @return Success or error code
         */
        ResultCode Initialize();
        
        /**
         * Finalize and cleanup the multiplayer service
         * @return Success or error code
         */
        ResultCode Finalize();
        
        /**
         * Create a new multiplayer session
         * @param config Session configuration parameters
         * @return Success or error code
         */
        ResultCode CreateNetwork(const NetworkConfig& config);
        
        /**
         * Start scanning for available sessions
         * @param filter Optional scan filter criteria
         * @return Success or error code
         */
        ResultCode Scan(const ScanFilter& filter = {});
        
        /**
         * Get list of discovered sessions
         * @return Vector of discovered network information
         */
        std::vector<NetworkInfo> GetNetworkInfo();
        
        /**
         * Connect to a specific session
         * @param target Network information of target session
         * @return Success or error code
         */
        ResultCode Connect(const NetworkInfo& target);
        
        /**
         * Disconnect from current session
         * @return Success or error code
         */
        ResultCode Disconnect();
        
        /**
         * Send data to specific player
         * @param data Data buffer to send
         * @param target_player Target player ID
         * @return Success or error code
         */
        ResultCode SendData(const std::vector<uint8_t>& data, PlayerId target_player);
        
        /**
         * Broadcast data to all players
         * @param data Data buffer to broadcast
         * @return Success or error code
         */
        ResultCode BroadcastData(const std::vector<uint8_t>& data);
        
        /**
         * Receive data from network
         * @param buffer Buffer to store received data
         * @param sender_id Output parameter for sender player ID
         * @return Number of bytes received, or error code if negative
         */
        int32_t ReceiveData(std::vector<uint8_t>& buffer, PlayerId& sender_id);
        
        /**
         * Get current session state
         * @return Current session state
         */
        SessionState GetState() const;
        
        /**
         * Get information about current session
         * @return Current session info, or nullopt if not in session
         */
        std::optional<NetworkInfo> GetCurrentSession() const;
        
        /**
         * Get list of players in current session
         * @return Vector of player information
         */
        std::vector<PlayerInfo> GetPlayers() const;
        
        // Event callbacks
        using StateChangeCallback = std::function<void(SessionState old_state, SessionState new_state)>;
        using PlayerJoinCallback = std::function<void(const PlayerInfo& player)>;
        using PlayerLeaveCallback = std::function<void(PlayerId player_id)>;
        using DataReceiveCallback = std::function<void(const std::vector<uint8_t>& data, PlayerId sender)>;
        
        void SetStateChangeCallback(StateChangeCallback callback);
        void SetPlayerJoinCallback(PlayerJoinCallback callback);
        void SetPlayerLeaveCallback(PlayerLeaveCallback callback);
        void SetDataReceiveCallback(DataReceiveCallback callback);
    };
}
```

### Type Translation API

```cpp
namespace Core::Multiplayer {
    class TypeTranslator {
    public:
        // LDN to internal type conversion
        static Internal::SessionInfo FromLdnNetworkInfo(const NetworkInfo& network);
        static Internal::PlayerData FromLdnPlayerInfo(const PlayerInfo& player);
        static Internal::GameConfig FromLdnNetworkConfig(const NetworkConfig& config);
        
        // Internal to LDN type conversion
        static NetworkInfo ToLdnNetworkInfo(const Internal::SessionInfo& session);
        static PlayerInfo ToLdnPlayerInfo(const Internal::PlayerData& player);
        static NetworkConfig ToLdnNetworkConfig(const Internal::GameConfig& config);
        
        // Error code translation
        static LdnResult ToLdnResult(ErrorCode error);
        static ErrorCode FromLdnResult(LdnResult result);
    };
}
```

## Backend Interface API

### IMultiplayerBackend Interface

The common interface implemented by both Model A and Model B backends:

```cpp
namespace Core::Multiplayer {
    class IMultiplayerBackend {
    public:
        virtual ~IMultiplayerBackend() = default;
        
        // Lifecycle management
        virtual ResultCode Initialize(const BackendConfig& config) = 0;
        virtual ResultCode Shutdown() = 0;
        
        // Session management
        virtual ResultCode CreateSession(const NetworkConfig& config) = 0;
        virtual ResultCode JoinSession(const NetworkInfo& session) = 0;
        virtual ResultCode LeaveSession() = 0;
        
        // Discovery
        virtual ResultCode StartDiscovery(const ScanFilter& filter) = 0;
        virtual ResultCode StopDiscovery() = 0;
        virtual std::vector<NetworkInfo> GetDiscoveredSessions() = 0;
        
        // Communication
        virtual ResultCode SendPacket(const NetworkPacket& packet) = 0;
        virtual ResultCode ReceivePacket(NetworkPacket& packet) = 0;
        
        // State queries
        virtual SessionState GetState() const = 0;
        virtual std::optional<NetworkInfo> GetCurrentSession() const = 0;
        virtual std::vector<PlayerInfo> GetConnectedPlayers() const = 0;
        
        // Event handlers (to be implemented by backend)
        virtual void OnStateChanged(SessionState new_state) = 0;
        virtual void OnPlayerJoined(const PlayerInfo& player) = 0;
        virtual void OnPlayerLeft(PlayerId player_id) = 0;
        virtual void OnPacketReceived(const NetworkPacket& packet) = 0;
    };
}
```

### Backend Factory API

```cpp
namespace Core::Multiplayer {
    class BackendFactory {
    public:
        /**
         * Create appropriate backend based on mode
         * @param mode Desired multiplayer mode
         * @return Unique pointer to backend implementation
         */
        static std::unique_ptr<IMultiplayerBackend> CreateBackend(MultiplayerMode mode);
        
        /**
         * Check if Model A (Internet) is supported on current platform
         * @return True if Model A is available
         */
        static bool IsModelASupported();
        
        /**
         * Check if Model B (Offline) is supported on current platform
         * @return True if Model B is available
         */
        static bool IsModelBSupported();
        
        /**
         * Get recommended mode for current platform and configuration
         * @return Recommended multiplayer mode
         */
        static MultiplayerMode GetRecommendedMode();
        
        /**
         * Get platform capabilities
         * @return Structure describing platform capabilities
         */
        static PlatformCapabilities GetPlatformCapabilities();
    };
}
```

## Model A: Internet Multiplayer APIs

### Room Client API

```cpp
namespace Core::Multiplayer::ModelA {
    class RoomClient {
    public:
        /**
         * Connect to room server
         * @param server_url WebSocket URL of room server
         * @param auth_token Optional authentication token
         * @return Success or error code
         */
        ResultCode Connect(const std::string& server_url, 
                          const std::optional<std::string>& auth_token = std::nullopt);
        
        /**
         * Disconnect from room server
         * @return Success or error code
         */
        ResultCode Disconnect();
        
        /**
         * Register a new session with the room server
         * @param session Session information to register
         * @return Success or error code
         */
        ResultCode RegisterSession(const SessionInfo& session);
        
        /**
         * Unregister current session from room server
         * @return Success or error code
         */
        ResultCode UnregisterSession();
        
        /**
         * Query available sessions from room server
         * @param filter Optional filter criteria
         * @return Vector of available sessions
         */
        std::vector<SessionInfo> QuerySessions(const SessionFilter& filter = {});
        
        /**
         * Request to join a specific session
         * @param session_id Target session ID
         * @param password Optional session password
         * @return Success or error code
         */
        ResultCode RequestJoinSession(SessionId session_id, 
                                     const std::optional<std::string>& password = std::nullopt);
        
        // Event callbacks
        using SessionUpdateCallback = std::function<void(const SessionInfo& session)>;
        using JoinRequestCallback = std::function<void(const JoinRequest& request)>;
        using ServerErrorCallback = std::function<void(ErrorCode error, const std::string& message)>;
        
        void SetSessionUpdateCallback(SessionUpdateCallback callback);
        void SetJoinRequestCallback(JoinRequestCallback callback);
        void SetServerErrorCallback(ServerErrorCallback callback);
        
        /**
         * Get connection status
         * @return Current connection state
         */
        ConnectionState GetConnectionState() const;
        
        /**
         * Get server information
         * @return Server info if connected
         */
        std::optional<ServerInfo> GetServerInfo() const;
    };
}
```

### P2P Network API

```cpp
namespace Core::Multiplayer::ModelA {
    class P2PNetwork {
    public:
        /**
         * Initialize P2P network with configuration
         * @param config P2P configuration parameters
         * @return Success or error code
         */
        ResultCode Initialize(const P2PConfig& config);
        
        /**
         * Create P2P host for incoming connections
         * @param port Port to listen on (0 for automatic)
         * @return Success or error code
         */
        ResultCode StartHost(uint16_t port = 0);
        
        /**
         * Stop hosting P2P connections
         * @return Success or error code
         */
        ResultCode StopHost();
        
        /**
         * Connect to a remote peer
         * @param peer_info Information about target peer
         * @return Success or error code
         */
        ResultCode ConnectToPeer(const PeerInfo& peer_info);
        
        /**
         * Disconnect from a specific peer
         * @param peer_id Target peer ID
         * @return Success or error code
         */
        ResultCode DisconnectFromPeer(PlayerId peer_id);
        
        /**
         * Send data to specific peer
         * @param data Data to send
         * @param peer_id Target peer ID
         * @return Success or error code
         */
        ResultCode SendToPeer(const std::vector<uint8_t>& data, PlayerId peer_id);
        
        /**
         * Broadcast data to all connected peers
         * @param data Data to broadcast
         * @return Success or error code
         */
        ResultCode BroadcastToAllPeers(const std::vector<uint8_t>& data);
        
        /**
         * Get list of connected peers
         * @return Vector of peer information
         */
        std::vector<PeerInfo> GetConnectedPeers() const;
        
        /**
         * Check if direct connection is possible with peer
         * @param peer_info Target peer information
         * @return True if direct connection is likely to succeed
         */
        bool CanDirectConnect(const PeerInfo& peer_info) const;
        
        /**
         * Get NAT traversal status
         * @return Current NAT traversal state
         */
        NATTraversalState GetNATTraversalState() const;
        
        // Event callbacks
        using PeerConnectedCallback = std::function<void(const PeerInfo& peer)>;
        using PeerDisconnectedCallback = std::function<void(PlayerId peer_id)>;
        using DataReceivedCallback = std::function<void(const std::vector<uint8_t>& data, PlayerId sender)>;
        
        void SetPeerConnectedCallback(PeerConnectedCallback callback);
        void SetPeerDisconnectedCallback(PeerDisconnectedCallback callback);
        void SetDataReceivedCallback(DataReceivedCallback callback);
    };
}
```

### Relay Client API

```cpp
namespace Core::Multiplayer::ModelA {
    class RelayClient {
    public:
        /**
         * Connect to relay server
         * @param server_info Relay server connection information
         * @return Success or error code
         */
        ResultCode ConnectToRelay(const RelayServerInfo& server_info);
        
        /**
         * Disconnect from relay server
         * @return Success or error code
         */
        ResultCode DisconnectFromRelay();
        
        /**
         * Create a new relay session
         * @param session_id Unique session identifier
         * @return Success or error code
         */
        ResultCode CreateRelaySession(SessionId session_id);
        
        /**
         * Join an existing relay session
         * @param session_id Target session identifier
         * @return Success or error code
         */
        ResultCode JoinRelaySession(SessionId session_id);
        
        /**
         * Leave current relay session
         * @return Success or error code
         */
        ResultCode LeaveRelaySession();
        
        /**
         * Send packet through relay
         * @param packet Packet to send
         * @return Success or error code
         */
        ResultCode SendRelayPacket(const RelayPacket& packet);
        
        /**
         * Set bandwidth limit for relay connection
         * @param max_mbps Maximum bandwidth in Mbps
         * @return Success or error code
         */
        ResultCode SetBandwidthLimit(uint32_t max_mbps);
        
        /**
         * Get current bandwidth statistics
         * @return Bandwidth usage statistics
         */
        BandwidthStats GetBandwidthStats() const;
        
        /**
         * Get relay connection latency
         * @return Current latency in milliseconds
         */
        std::chrono::milliseconds GetRelayLatency() const;
        
        // Event callback
        using RelayPacketCallback = std::function<void(const RelayPacket& packet)>;
        void SetRelayPacketCallback(RelayPacketCallback callback);
    };
}
```

## Model B: Offline Multiplayer APIs

### mDNS Discovery API

```cpp
namespace Core::Multiplayer::ModelB {
    class MdnsDiscovery {
    public:
        /**
         * Initialize mDNS discovery service
         * @param config mDNS configuration parameters
         * @return Success or error code
         */
        ResultCode Initialize(const MdnsConfig& config);
        
        /**
         * Advertise a service on the local network
         * @param service Service information to advertise
         * @return Success or error code
         */
        ResultCode AdvertiseService(const ServiceInfo& service);
        
        /**
         * Stop advertising current service
         * @return Success or error code
         */
        ResultCode StopAdvertising();
        
        /**
         * Start discovering services on local network
         * @param filter Optional discovery filter
         * @return Success or error code
         */
        ResultCode StartDiscovery(const DiscoveryFilter& filter = {});
        
        /**
         * Stop service discovery
         * @return Success or error code
         */
        ResultCode StopDiscovery();
        
        /**
         * Get list of discovered services
         * @return Vector of discovered services
         */
        std::vector<ServiceInfo> GetDiscoveredServices() const;
        
        /**
         * Update TXT record for advertised service
         * @param txt_records New TXT record data
         * @return Success or error code
         */
        ResultCode UpdateTxtRecords(const TxtRecordMap& txt_records);
        
        // Event callbacks
        using ServiceDiscoveredCallback = std::function<void(const ServiceInfo& service)>;
        using ServiceLostCallback = std::function<void(const ServiceInfo& service)>;
        using ServiceUpdatedCallback = std::function<void(const ServiceInfo& service)>;
        
        void SetServiceDiscoveredCallback(ServiceDiscoveredCallback callback);
        void SetServiceLostCallback(ServiceLostCallback callback);
        void SetServiceUpdatedCallback(ServiceUpdatedCallback callback);
    };
}
```

### Platform-Specific APIs

#### Android Wi-Fi Direct API

```cpp
namespace Core::Multiplayer::ModelB::Android {
    class WiFiDirectWrapper {
    public:
        /**
         * Initialize Wi-Fi Direct wrapper
         * @param env JNI environment pointer
         * @param wifi_manager WifiP2pManager Java object
         * @return Success or error code
         */
        ResultCode Initialize(JNIEnv* env, jobject wifi_manager);
        
        /**
         * Create Wi-Fi Direct group (become group owner)
         * @param config Group configuration
         * @return Success or error code
         */
        ResultCode CreateGroup(const WiFiDirectGroupConfig& config = {});
        
        /**
         * Remove current Wi-Fi Direct group
         * @return Success or error code
         */
        ResultCode RemoveGroup();
        
        /**
         * Connect to existing Wi-Fi Direct group
         * @param group_info Target group information
         * @return Success or error code
         */
        ResultCode ConnectToGroup(const WiFiDirectGroupInfo& group_info);
        
        /**
         * Disconnect from current group
         * @return Success or error code
         */
        ResultCode DisconnectFromGroup();
        
        /**
         * Start peer discovery
         * @return Success or error code
         */
        ResultCode StartPeerDiscovery();
        
        /**
         * Stop peer discovery
         * @return Success or error code
         */
        ResultCode StopPeerDiscovery();
        
        /**
         * Get list of discovered peers
         * @return Vector of peer information
         */
        std::vector<WiFiDirectPeer> GetDiscoveredPeers() const;
        
        /**
         * Get current group information
         * @return Group info if connected, nullopt otherwise
         */
        std::optional<WiFiDirectGroupInfo> GetGroupInfo() const;
        
        /**
         * Get current connection state
         * @return Current Wi-Fi Direct state
         */
        WiFiDirectState GetState() const;
        
        // JNI callback methods (called from Java)
        static void JNICALL OnConnectionStateChanged(JNIEnv* env, jobject obj, jint state);
        static void JNICALL OnPeersAvailable(JNIEnv* env, jobject obj, jobjectArray peers);
        static void JNICALL OnGroupInfoAvailable(JNIEnv* env, jobject obj, jobject group_info);
    };
}
```

#### Windows Mobile Hotspot API

```cpp
namespace Core::Multiplayer::ModelB::Windows {
    class MobileHotspotManager {
    public:
        /**
         * Initialize mobile hotspot manager
         * @return Success or error code
         */
        ResultCode Initialize();
        
        /**
         * Start mobile hotspot with configuration
         * @param config Hotspot configuration parameters
         * @return Success or error code
         */
        ResultCode StartHotspot(const HotspotConfig& config);
        
        /**
         * Stop mobile hotspot
         * @return Success or error code
         */
        ResultCode StopHotspot();
        
        /**
         * Get current hotspot state
         * @return Current hotspot state
         */
        HotspotState GetHotspotState() const;
        
        /**
         * Get list of connected clients
         * @return Vector of connected client information
         */
        std::vector<ConnectedClient> GetConnectedClients() const;
        
        /**
         * Disconnect a specific client
         * @param client_info Client to disconnect
         * @return Success or error code
         */
        ResultCode DisconnectClient(const ConnectedClient& client_info);
        
        /**
         * Update hotspot configuration
         * @param config New configuration parameters
         * @return Success or error code
         */
        ResultCode UpdateConfiguration(const HotspotConfig& config);
        
        /**
         * Get platform capabilities
         * @return Structure describing what features are available
         */
        HotspotCapabilities GetCapabilities() const;
        
        // Event callbacks
        using ClientConnectedCallback = std::function<void(const ConnectedClient& client)>;
        using ClientDisconnectedCallback = std::function<void(const ConnectedClient& client)>;
        using HotspotStateCallback = std::function<void(HotspotState new_state)>;
        
        void SetClientConnectedCallback(ClientConnectedCallback callback);
        void SetClientDisconnectedCallback(ClientDisconnectedCallback callback);
        void SetHotspotStateCallback(HotspotStateCallback callback);
    };
}
```

## Configuration API

### Configuration Manager

```cpp
namespace Core::Multiplayer {
    class ConfigurationManager {
    public:
        /**
         * Load configuration from file
         * @param config_path Path to configuration file
         * @return Success or error code
         */
        static ResultCode LoadConfiguration(const std::string& config_path);
        
        /**
         * Save configuration to file
         * @param config_path Path to save configuration
         * @return Success or error code
         */
        static ResultCode SaveConfiguration(const std::string& config_path);
        
        /**
         * Get current multiplayer configuration
         * @return Current configuration structure
         */
        static MultiplayerConfig GetConfiguration();
        
        /**
         * Update multiplayer configuration
         * @param config New configuration to apply
         * @return Success or error code
         */
        static ResultCode SetConfiguration(const MultiplayerConfig& config);
        
        /**
         * Reset configuration to defaults
         * @return Success or error code
         */
        static ResultCode ResetToDefaults();
        
        /**
         * Validate configuration structure
         * @param config Configuration to validate
         * @return True if configuration is valid
         */
        static bool ValidateConfiguration(const MultiplayerConfig& config);
        
        /**
         * Get platform-specific default configuration
         * @return Default configuration for current platform
         */
        static MultiplayerConfig GetPlatformDefaults();
    };
}
```

## Error Handling API

### Error Context and Recovery

```cpp
namespace Core::Multiplayer {
    class ErrorHandler {
    public:
        /**
         * Set additional context for error reporting
         * @param context Context information
         */
        static void SetErrorContext(const ErrorContext& context);
        
        /**
         * Set retry delay for specific error types
         * @param error Error code
         * @param delay Delay before retry
         */
        static void SetRetryDelay(ErrorCode error, std::chrono::milliseconds delay);
        
        /**
         * Add suggested action for error recovery
         * @param error Error code
         * @param action Suggested recovery action
         */
        static void AddSuggestedAction(ErrorCode error, const std::string& action);
        
        /**
         * Get formatted error message with context
         * @param error Error code
         * @return Human-readable error message
         */
        static std::string FormatError(ErrorCode error);
        
        /**
         * Check if error should trigger automatic retry
         * @param error Error code
         * @return True if automatic retry is recommended
         */
        static bool ShouldRetry(ErrorCode error);
        
        /**
         * Get suggested recovery actions for error
         * @param error Error code
         * @return Vector of suggested actions
         */
        static std::vector<std::string> GetRecoveryActions(ErrorCode error);
    };
}
```

## Event System API

### Event Dispatcher

```cpp
namespace Core::Multiplayer {
    enum class EventType {
        StateChanged,
        PlayerJoined,
        PlayerLeft,
        SessionCreated,
        SessionDestroyed,
        ConnectionLost,
        ConnectionRestored,
        DataReceived,
        ErrorOccurred
    };
    
    struct Event {
        EventType type;
        std::chrono::steady_clock::time_point timestamp;
        std::any payload;  // Type-erased event data
    };
    
    class EventDispatcher {
    public:
        using EventCallback = std::function<void(const Event& event)>;
        using EventFilter = std::function<bool(const Event& event)>;
        
        /**
         * Subscribe to specific event types
         * @param types Event types to subscribe to
         * @param callback Callback function for events
         * @param filter Optional event filter
         * @return Subscription ID for unsubscribing
         */
        uint32_t Subscribe(const std::vector<EventType>& types,
                          EventCallback callback,
                          EventFilter filter = nullptr);
        
        /**
         * Unsubscribe from events
         * @param subscription_id ID returned from Subscribe
         */
        void Unsubscribe(uint32_t subscription_id);
        
        /**
         * Dispatch event to all subscribers
         * @param event Event to dispatch
         */
        void DispatchEvent(const Event& event);
        
        /**
         * Get event history
         * @param max_events Maximum number of events to return
         * @return Vector of recent events
         */
        std::vector<Event> GetEventHistory(size_t max_events = 100) const;
        
        /**
         * Clear event history
         */
        void ClearEventHistory();
    };
}
```

## Performance Monitoring API

### Performance Metrics

```cpp
namespace Core::Multiplayer {
    struct PerformanceMetrics {
        // Connection metrics
        std::chrono::milliseconds average_latency;
        std::chrono::milliseconds peak_latency;
        uint32_t packet_loss_percentage;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        
        // Session metrics
        std::chrono::steady_clock::time_point session_start_time;
        uint32_t reconnection_count;
        uint32_t total_players_seen;
        
        // Error metrics
        std::map<ErrorCode, uint32_t> error_counts;
        std::chrono::steady_clock::time_point last_error_time;
    };
    
    class PerformanceMonitor {
    public:
        /**
         * Get current performance metrics
         * @return Current metrics snapshot
         */
        static PerformanceMetrics GetMetrics();
        
        /**
         * Reset all performance counters
         */
        static void ResetMetrics();
        
        /**
         * Record latency measurement
         * @param latency Measured latency
         */
        static void RecordLatency(std::chrono::milliseconds latency);
        
        /**
         * Record packet loss event
         * @param lost_packets Number of packets lost
         * @param total_packets Total packets in measurement window
         */
        static void RecordPacketLoss(uint32_t lost_packets, uint32_t total_packets);
        
        /**
         * Record data transfer
         * @param bytes_sent Bytes sent in this measurement
         * @param bytes_received Bytes received in this measurement
         */
        static void RecordDataTransfer(uint64_t bytes_sent, uint64_t bytes_received);
        
        /**
         * Record error occurrence
         * @param error Error code that occurred
         */
        static void RecordError(ErrorCode error);
        
        /**
         * Set performance monitoring callback
         * @param callback Function called when metrics update
         */
        static void SetMetricsCallback(std::function<void(const PerformanceMetrics&)> callback);
    };
}
```

## Thread Safety

### Thread Safety Guarantees

All public APIs in the Sudachi Multiplayer system provide the following thread safety guarantees:

1. **Read Operations**: Safe to call from multiple threads concurrently
2. **Write Operations**: Protected by internal synchronization
3. **Callbacks**: May be called from any thread, implementations must be thread-safe
4. **Configuration**: Thread-safe getter/setter operations

### Thread-Safe Collections

```cpp
namespace Core::Multiplayer {
    template<typename T>
    class ThreadSafeQueue {
    public:
        void Push(const T& item);
        void Push(T&& item);
        bool TryPop(T& item);
        T WaitAndPop();
        bool Empty() const;
        size_t Size() const;
    };
    
    template<typename Key, typename Value>
    class ThreadSafeMap {
    public:
        void Insert(const Key& key, const Value& value);
        bool TryGet(const Key& key, Value& value) const;
        bool Remove(const Key& key);
        std::vector<Key> GetKeys() const;
        size_t Size() const;
    };
}
```

## Usage Examples

### Basic Session Creation and Discovery

```cpp
#include "core/multiplayer/ldn_service_bridge.h"

using namespace Core::Multiplayer;

// Initialize multiplayer service
LdnServiceBridge ldn_service;
auto result = ldn_service.Initialize();
if (result != ErrorCode::Success) {
    // Handle initialization error
    return;
}

// Create a session
NetworkConfig config;
config.game_id = 0x0100000000000001;  // Game-specific ID
config.game_version = "1.0.0";
config.session_name = "My Game Session";
config.max_players = 4;
config.is_public = true;

result = ldn_service.CreateNetwork(config);
if (result != ErrorCode::Success) {
    // Handle session creation error
    return;
}

// Set up event callbacks
ldn_service.SetPlayerJoinCallback([](const PlayerInfo& player) {
    std::cout << "Player joined: " << player.display_name << std::endl;
});

ldn_service.SetDataReceiveCallback([](const std::vector<uint8_t>& data, PlayerId sender) {
    // Process received game data
    ProcessGameData(data, sender);
});
```

### Session Discovery and Joining

```cpp
// Start scanning for sessions
result = ldn_service.Scan();
if (result != ErrorCode::Success) {
    // Handle scan error
    return;
}

// Wait for discovery results
std::this_thread::sleep_for(std::chrono::seconds(2));

// Get discovered sessions
auto sessions = ldn_service.GetNetworkInfo();
if (!sessions.empty()) {
    // Join the first available session
    result = ldn_service.Connect(sessions[0]);
    if (result == ErrorCode::Success) {
        std::cout << "Connected to session: " << sessions[0].session_name << std::endl;
    }
}
```

### Error Handling Example

```cpp
auto result = ldn_service.Connect(target_session);
switch (result) {
    case ErrorCode::Success:
        std::cout << "Connected successfully" << std::endl;
        break;
    case ErrorCode::RoomFull:
        std::cout << "Session is full, trying another..." << std::endl;
        // Try next session
        break;
    case ErrorCode::RoomPasswordRequired:
        std::cout << "Password required" << std::endl;
        // Prompt user for password
        break;
    case ErrorCode::ConnectionTimeout:
        std::cout << "Connection timed out, retrying..." << std::endl;
        if (ErrorHandler::ShouldRetry(result)) {
            // Retry after delay
            std::this_thread::sleep_for(ErrorHandler::GetRetryDelay(result));
            // Retry connection...
        }
        break;
    default:
        std::cout << "Connection failed: " << ErrorHandler::FormatError(result) << std::endl;
        auto actions = ErrorHandler::GetRecoveryActions(result);
        for (const auto& action : actions) {
            std::cout << "Suggested: " << action << std::endl;
        }
        break;
}
```

---

*This API reference is maintained by the Sudachi development team. For implementation examples and tutorials, see the [Developer Guide](../developer-guide/) section.*