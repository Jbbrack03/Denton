# Sudachi Multiplayer Architecture Guide

## Overview

The Sudachi Multiplayer system implements a dual-mode architecture that provides both internet-based and offline ad-hoc multiplayer capabilities. This document provides a comprehensive technical overview of the system architecture, component relationships, and data flow patterns.

## High-Level Architecture

### Dual-Mode Design Philosophy

The multiplayer system is built around two distinct operational modes:

- **Model A (Internet Multiplayer)**: Global connectivity using centralized discovery with P2P/relay data transmission
- **Model B (Offline Multiplayer)**: Local device-to-device networking using platform-specific wireless APIs

Both modes are unified through a common HLE (High-Level Emulation) interface that emulates the Nintendo Switch's `nn::ldn` system service.

```
┌─────────────────────────────────────────────────────────────┐
│                        Game Layer                           │
│  (Nintendo Switch games using nn::ldn for local wireless)  │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                    HLE nn::ldn Layer                       │
│        (LdnService, LdnServiceBridge, TypeTranslator)      │
└─────────────────┬───────────────────────┬───────────────────┘
                  │                       │
        ┌─────────▼─────────┐   ┌─────────▼─────────┐
        │    Model A        │   │    Model B        │
        │ (Internet-based)  │   │ (Offline Ad-hoc)  │
        └───────────────────┘   └───────────────────┘
```

## Core Components

### 1. HLE nn::ldn Interface Layer

#### LdnService (`src/core/multiplayer/ldn_service_bridge.cpp`)

The primary interface that games interact with, implementing the IPC commands that Nintendo Switch games expect:

```cpp
class LdnServiceBridge {
public:
    // Core nn::ldn IPC commands
    ResultCode Initialize();
    ResultCode CreateNetwork(const NetworkConfig& config);
    ResultCode Scan(const ScanFilter& filter);
    ResultCode Connect(const NetworkInfo& target);
    ResultCode Disconnect();
    
    // State management
    ResultCode OpenAccessPoint();
    ResultCode CloseAccessPoint();
    std::vector<NetworkInfo> GetNetworkInfo();
    
private:
    std::unique_ptr<IMultiplayerBackend> backend;
    LdnSessionState current_state;
};
```

#### Backend Selection (`src/core/multiplayer/backend_factory.cpp`)

Automatically selects the appropriate backend based on user configuration and platform capabilities:

```cpp
class BackendFactory {
public:
    static std::unique_ptr<IMultiplayerBackend> CreateBackend(MultiplayerMode mode);
    static bool IsModelASupported();
    static bool IsModelBSupported();
    static MultiplayerMode GetRecommendedMode();
};
```

#### Type Translation (`src/core/multiplayer/type_translator.cpp`)

Handles bidirectional conversion between Nintendo's LDN types and internal multiplayer types:

```cpp
class TypeTranslator {
public:
    static NetworkInfo ToLdnNetworkInfo(const Internal::SessionInfo& session);
    static Internal::SessionInfo FromLdnNetworkInfo(const NetworkInfo& network);
    static PlayerInfo ToLdnPlayerInfo(const Internal::PlayerData& player);
    // ... additional conversion methods
};
```

### 2. Model A: Internet Multiplayer Architecture

#### Component Overview

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│ Room Client  │    │ P2P Network  │    │ Relay Client │
│              │    │              │    │              │
│ - Discovery  │────│ - Direct P2P │────│ - Fallback   │
│ - Session    │    │ - NAT Trav.  │    │ - Bandwidth  │
│   Management │    │ - libp2p     │    │   Control    │
└──────────────┘    └──────────────┘    └──────────────┘
        │                    │                    │
        └────────────────────┼────────────────────┘
                             │
                ┌────────────▼────────────┐
                │   Network Security      │
                │                         │
                │ - Input Validation      │
                │ - Rate Limiting         │
                │ - DDoS Protection       │
                └─────────────────────────┘
```

#### Room Client (`src/core/multiplayer/model_a/room_client.cpp`)

Manages communication with the centralized room server for session discovery:

```cpp
class RoomClient {
public:
    // WebSocket connection management
    ResultCode Connect(const std::string& server_url);
    ResultCode Disconnect();
    
    // Session management
    ResultCode RegisterSession(const SessionInfo& session);
    ResultCode UnregisterSession();
    std::vector<SessionInfo> GetAvailableSessions(const ScanFilter& filter);
    
    // Event handling
    void SetSessionUpdateCallback(std::function<void(const SessionInfo&)> callback);
    void SetPlayerJoinCallback(std::function<void(const PlayerInfo&)> callback);
    
private:
    std::unique_ptr<IWebSocketConnection> websocket;
    ThreadSafeMessageQueue<RoomMessage> message_queue;
    std::thread message_processor;
};
```

#### P2P Network (`src/core/multiplayer/model_a/p2p_network.cpp`)

Handles direct peer-to-peer connections using cpp-libp2p:

```cpp
class P2PNetwork {
public:
    // Connection lifecycle
    ResultCode Initialize(const P2PConfig& config);
    ResultCode CreateHost(uint16_t port);
    ResultCode ConnectToPeer(const PeerInfo& peer);
    ResultCode Disconnect();
    
    // Data transmission
    ResultCode SendData(const std::vector<uint8_t>& data, PlayerId target);
    ResultCode BroadcastData(const std::vector<uint8_t>& data);
    
    // NAT traversal
    bool SupportsDirectConnection(const PeerInfo& peer);
    ResultCode EstablishDirectConnection(const PeerInfo& peer);
    
private:
    std::shared_ptr<libp2p::Host> libp2p_host;
    std::unordered_map<PlayerId, libp2p::peer::PeerId> peer_connections;
    NATTraversalHelper nat_helper;
};
```

#### Relay Client (`src/core/multiplayer/model_a/relay_client.cpp`)

Provides fallback connectivity when P2P connections fail:

```cpp
class RelayClient {
public:
    // Relay connection management
    ResultCode ConnectToRelay(const RelayServerInfo& server);
    ResultCode CreateSession(uint32_t session_id);
    ResultCode JoinSession(uint32_t session_id);
    
    // Data relaying
    ResultCode SendPacket(const RelayPacket& packet);
    void SetPacketReceiveCallback(std::function<void(const RelayPacket&)> callback);
    
    // Bandwidth management
    void SetBandwidthLimit(uint32_t max_mbps);
    BandwidthStats GetBandwidthStats();
    
private:
    std::unique_ptr<ITcpConnection> relay_connection;
    TokenBucketRateLimit rate_limiter;
    std::queue<RelayPacket> send_queue;
};
```

### 3. Model B: Offline Ad-Hoc Architecture

#### Component Overview

```
┌──────────────────┐    ┌─────────────────────────────────┐
│  mDNS Discovery  │    │      Platform Abstraction       │
│                  │    │                                 │
│ - Service Adv.   │    │  ┌─────────┐  ┌─────────────┐   │
│ - Peer Discovery │────│  │ Android │  │   Windows   │   │
│ - TXT Records    │    │  │ Wi-Fi   │  │   Mobile    │   │
└──────────────────┘    │  │ Direct  │  │  Hotspot    │   │
                        │  └─────────┘  └─────────────┘   │
                        └─────────────────────────────────┘
```

#### mDNS Discovery (`src/core/multiplayer/model_b/mdns_discovery.cpp`)

Cross-platform service discovery using mjansson/mdns:

```cpp
class MdnsDiscovery {
public:
    // Service advertisement
    ResultCode AdvertiseService(const ServiceInfo& service);
    ResultCode StopAdvertising();
    
    // Service discovery
    ResultCode StartDiscovery(const DiscoveryFilter& filter);
    ResultCode StopDiscovery();
    std::vector<ServiceInfo> GetDiscoveredServices();
    
    // Event callbacks
    void SetServiceDiscoveredCallback(std::function<void(const ServiceInfo&)> callback);
    void SetServiceLostCallback(std::function<void(const ServiceInfo&)> callback);
    
private:
    mdns_socket_t* mdns_socket;
    std::thread discovery_thread;
    std::vector<ServiceInfo> discovered_services;
    std::mutex services_mutex;
};
```

#### Platform-Specific Implementations

**Android Wi-Fi Direct** (`src/core/multiplayer/model_b/platform/android/wifi_direct_wrapper.cpp`):

```cpp
class WiFiDirectWrapper {
public:
    // Initialization
    ResultCode Initialize(JNIEnv* env, jobject wifi_manager);
    
    // Group management
    ResultCode CreateGroup();
    ResultCode ConnectToGroup(const GroupInfo& group);
    ResultCode LeaveGroup();
    
    // Peer discovery
    ResultCode StartPeerDiscovery();
    ResultCode StopPeerDiscovery();
    std::vector<PeerInfo> GetAvailablePeers();
    
    // JNI callbacks
    static void JNICALL OnConnectionStateChanged(JNIEnv* env, jobject obj, jint state);
    static void JNICALL OnPeersAvailable(JNIEnv* env, jobject obj, jobjectArray peers);
    
private:
    JavaVM* jvm;
    jobject wifi_p2p_manager;
    WiFiDirectState current_state;
};
```

**Windows Mobile Hotspot** (`src/core/multiplayer/model_b/platform/windows/mobile_hotspot_manager.cpp`):

```cpp
class MobileHotspotManager {
public:
    // Hotspot lifecycle
    ResultCode StartHotspot(const HotspotConfig& config);
    ResultCode StopHotspot();
    HotspotState GetHotspotState();
    
    // Client management
    std::vector<ConnectedClient> GetConnectedClients();
    ResultCode DisconnectClient(const ClientInfo& client);
    
    // Configuration
    ResultCode SetHotspotConfig(const HotspotConfig& config);
    HotspotCapabilities GetPlatformCapabilities();
    
private:
    winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringManager tethering_manager;
    winrt::Windows::Networking::Connectivity::NetworkInformation network_info;
    HotspotState current_state;
};
```

## Data Flow Patterns

### 1. Session Creation Flow (Model A)

```
Game Request → LdnService → Backend Factory → Model A Backend
    ↓
Room Client → WebSocket → Room Server (registers session)
    ↓
P2P Network → libp2p → Direct connection setup
    ↓
Session Ready ← Backend ← LdnService ← Game receives response
```

### 2. Session Discovery Flow (Model A)

```
Game Scan → LdnService → Model A Backend
    ↓
Room Client → WebSocket → Room Server (query sessions)
    ↓
Session List ← Room Server ← WebSocket ← Room Client
    ↓
Type Translation → LdnService → Game receives NetworkInfo[]
```

### 3. Data Transmission Flow (Model A)

```
Game Data → LdnService → Backend → P2P Network
    ↓
libp2p (direct) OR Relay Client (fallback)
    ↓
Network → Remote P2P/Relay → Remote Backend
    ↓
Remote LdnService → Remote Game
```

### 4. Session Creation Flow (Model B)

```
Game Request → LdnService → Backend Factory → Model B Backend
    ↓
Platform Manager (Android/Windows) → Create hotspot/group
    ↓
mDNS Discovery → Advertise service
    ↓
Session Ready ← Backend ← LdnService ← Game receives response
```

### 5. Session Discovery Flow (Model B)

```
Game Scan → LdnService → Model B Backend
    ↓
mDNS Discovery → Query local network services
    ↓
Platform Manager → Scan for Wi-Fi Direct groups/hotspots
    ↓
Service List ← mDNS ← Platform ← Backend
    ↓
Type Translation → LdnService → Game receives NetworkInfo[]
```

## Error Handling Architecture

### Error Code Hierarchy

```cpp
namespace Core::Multiplayer {
    enum class ErrorCode {
        // Success
        Success = 0,
        
        // Connection errors (1000-1999)
        ConnectionFailed = 1001,
        ConnectionTimeout = 1002,
        ConnectionRefused = 1003,
        
        // Room errors (2000-2999)
        RoomNotFound = 2001,
        RoomFull = 2002,
        RoomPasswordRequired = 2003,
        
        // Platform errors (4000-4999)
        PlatformAPIError = 4001,
        PlatformFeatureUnavailable = 4002,
        // ... additional codes
    };
}
```

### Error Code Translation

The system includes comprehensive error translation between different layers:

```cpp
class ErrorCodeMapper {
public:
    static Core::Multiplayer::ErrorCode ToMultiplayerError(LdnResult ldn_result);
    static LdnResult ToLdnResult(Core::Multiplayer::ErrorCode mp_error);
    static std::string GetErrorDescription(Core::Multiplayer::ErrorCode error);
    static bool IsRetryableError(Core::Multiplayer::ErrorCode error);
};
```

### Error Recovery Strategies

#### Circuit Breaker Pattern

```cpp
class CircuitBreaker {
public:
    enum class State { Closed, Open, HalfOpen };
    
    bool AllowRequest();
    void RecordSuccess();
    void RecordFailure();
    State GetState() const;
    
private:
    State current_state;
    std::chrono::steady_clock::time_point last_failure_time;
    uint32_t failure_count;
    uint32_t failure_threshold;
    std::chrono::milliseconds timeout;
};
```

#### Graceful Degradation

```cpp
class GracefulDegradationManager {
public:
    void RegisterFallbackStrategy(ServiceType service, FallbackFunction fallback);
    ResultCode ExecuteWithFallback(ServiceType service, Operation operation);
    void SetDegradationLevel(DegradationLevel level);
    
private:
    std::unordered_map<ServiceType, std::vector<FallbackFunction>> fallback_strategies;
    DegradationLevel current_level;
};
```

## Security Architecture

### Network Security Layer

```cpp
class NetworkSecurityManager {
public:
    // Input validation
    bool ValidatePacket(const NetworkPacket& packet);
    bool ValidateJsonMessage(const std::string& json);
    
    // Rate limiting
    bool CheckRateLimit(const std::string& client_id, RateLimitType type);
    void RecordRequest(const std::string& client_id, RateLimitType type);
    
    // DDoS protection
    bool IsIPBlacklisted(const std::string& ip_address);
    void HandleSuspiciousActivity(const std::string& ip_address);
    
private:
    NetworkInputValidator validator;
    ClientRateManager rate_manager;
    DDoSProtection ddos_protection;
};
```

### Encryption and Authentication

- **Model A**: TLS/SSL encryption for all WebSocket and relay communications
- **Model B**: WPA3 encryption through platform Wi-Fi Direct/hotspot implementations
- **Data packets**: Application-level encryption using modern cryptographic libraries

## Performance Characteristics

### Latency Requirements

| Component | Target Latency | Maximum Acceptable |
|-----------|---------------|-------------------|
| P2P Direct Connection | < 5ms | < 20ms |
| Relay Connection | < 20ms | < 50ms |
| mDNS Discovery | < 100ms | < 500ms |
| Session Creation | < 1s | < 5s |
| HLE Command Processing | < 1ms | < 5ms |

### Throughput Requirements

| Data Type | Bandwidth Usage | Peak Bandwidth |
|-----------|----------------|----------------|
| Game State Sync | 50-200 KB/s | 500 KB/s |
| Audio Data | 64-128 KB/s | 256 KB/s |
| Control Input | 1-10 KB/s | 50 KB/s |
| Discovery Messages | < 1 KB/s | 10 KB/s |

### Memory Usage

| Component | Typical Usage | Maximum Usage |
|-----------|--------------|---------------|
| HLE Service | 1-2 MB | 5 MB |
| Model A Backend | 5-10 MB | 25 MB |
| Model B Backend | 2-5 MB | 10 MB |
| P2P Network | 10-20 MB | 50 MB |
| Total System | 20-40 MB | 100 MB |

## Thread Architecture

### Thread Model

```cpp
// Main application thread
└── HLE Service Thread (IPC command processing)
    ├── Model A Threads
    │   ├── WebSocket Message Thread
    │   ├── P2P Network Thread
    │   └── Relay Client Thread
    └── Model B Threads
        ├── mDNS Discovery Thread
        └── Platform Manager Thread
```

### Thread Safety

All public APIs are thread-safe through the use of:
- `std::mutex` for critical sections
- `std::atomic` for simple state variables
- Lock-free queues for high-performance data paths
- Thread-safe collections for shared data structures

## Configuration Management

### Configuration Schema

```json
{
  "multiplayer": {
    "default_mode": "internet",
    "display_name": "Player",
    "model_a": {
      "room_server_url": "wss://room.sudachi.org",
      "relay_servers": ["relay1.sudachi.org", "relay2.sudachi.org"],
      "p2p_config": {
        "enable_upnp": true,
        "port_range": { "min": 7777, "max": 7787 },
        "nat_traversal_timeout": 30000
      }
    },
    "model_b": {
      "mdns_service_type": "_sudachi-ldn._tcp.local.",
      "discovery_timeout": 10000,
      "platform_config": {
        "android": {
          "wifi_direct_timeout": 30000,
          "group_name_prefix": "Sudachi-"
        },
        "windows": {
          "hotspot_name": "Sudachi-Hotspot",
          "hotspot_password": "generated"
        }
      }
    }
  }
}
```

## Testing Architecture

### Unit Testing Strategy

- **Mock Interfaces**: All external dependencies have mock implementations
- **Dependency Injection**: Components accept interfaces rather than concrete types
- **Test Fixtures**: Reusable test environments for different scenarios
- **Performance Benchmarks**: Automated performance regression testing

### Integration Testing

- **Docker Environment**: Containerized test servers and relay infrastructure
- **Network Simulation**: Artificial latency, packet loss, and bandwidth limitations
- **Multi-Instance Testing**: Multiple emulator instances for end-to-end testing
- **Game-Specific Tests**: Automated testing with real Nintendo Switch games

## Deployment Architecture

### Build System Integration

The multiplayer system integrates with Sudachi's existing CMake build system:

```cmake
# Enable multiplayer support
option(ENABLE_MULTIPLAYER "Enable multiplayer support" ON)

if(ENABLE_MULTIPLAYER)
    add_subdirectory(src/core/multiplayer)
    target_link_libraries(sudachi_core multiplayer_system)
    target_compile_definitions(sudachi_core PRIVATE SUDACHI_ENABLE_MULTIPLAYER)
endif()
```

### Platform-Specific Packaging

- **Windows**: WinRT dependencies included in installer
- **Android**: JNI libraries packaged in APK
- **Linux**: System dependency detection and optional package installation
- **macOS**: Framework dependencies bundled in app package

## Future Architecture Considerations

### Planned Enhancements

1. **Voice Chat Integration**: Platform-specific voice communication
2. **Cloud Save Sync**: Multiplayer-aware save state synchronization
3. **Spectator Mode**: Non-participating observer functionality
4. **Replay System**: Record and playback multiplayer sessions
5. **Anti-Cheat Integration**: Basic cheat detection and prevention

### Scalability Improvements

1. **Distributed Room Servers**: Geographic distribution for reduced latency
2. **Relay Server Load Balancing**: Automatic server selection based on load
3. **P2P Mesh Networks**: Multi-hop P2P for improved connectivity
4. **Edge Computing**: CDN integration for static assets

---

*This architecture document is maintained by the Sudachi development team and updated with each major release. For implementation details, see the [API Reference](api-reference.md) and [Building from Source](building-from-source.md) guides.*