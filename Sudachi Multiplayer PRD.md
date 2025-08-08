# **Product Requirements Document: Sudachi Emulator Multiplayer Subsystem**

**Document Version:** 2.0

**Date:** August 2, 2025

**Revision Notes:** Major update adding detailed technical specifications, security architecture, configuration management, performance requirements, and comprehensive error handling framework.

**Author:** Lead Systems Architect

### **Section 1: Introduction and Strategic Overview**

#### **1.1. Project Mandate and Core Objectives**

This document provides the complete technical specification for implementing a dual-mode multiplayer subsystem within the Sudachi emulator. It is intended to serve as the definitive technical blueprint for the development team.

The primary objective is to deliver a "zero-guesswork" specification, defining all components, protocols, and APIs with sufficient detail to eliminate implementation ambiguity. This ensures a predictable, high-quality engineering outcome.

The secondary objective is to fulfill two key user-centric goals:

1. A seamless, zero-configuration **Internet Multiplayer** experience that allows users to play games supporting local wireless multiplayer with others across the globe, directly through the game's own interface.  
2. A robust, reliable **Offline Ad-Hoc Multiplayer** mode for specific platforms, enabling true device-to-device multiplayer without any requirement for an internet connection.

Initial development will target the Android and Windows platforms, with a clear path for future expansion to other supported operating systems.

#### **1.2. The Dual-Track Architecture: A Strategic Summary**

The implementation of multiplayer functionality in Sudachi will be pursued via a dual-track development strategy, with two distinct models being developed in parallel for the Windows and Android platforms. This concurrent approach is designed to avoid duplicative work, prevent backtracking, and ensure both systems are designed cohesively from the start.

* **Model A (Internet Multiplayer \- LDN-over-IP): The Global Experience.** This model represents the core online functionality. It is a universally compatible system that enables global play by intercepting the emulated game's local wireless traffic and tunneling it over a standard internet connection. This approach dramatically expands the potential player pool for any given game, transforming a feature designed for physical proximity into a worldwide online service. This model is compatible with all Sudachi-supported platforms for both hosting and client roles.  
* **Model B (Offline Ad-Hoc Multiplayer): The Local Experience.** This is a platform-specific feature designed to replicate the exact user experience of real Nintendo Switch hardware's "Local Play." It creates a direct, device-to-device wireless network that operates entirely offline. This serves a valuable use case for users in situations without internet access, such as during travel. This model introduces platform-specific complexity, and its initial implementation is scoped exclusively for Windows and Android.

For Windows and Android, users will be provided with a setting to toggle between these two modes, with Internet Multiplayer enabled by default.

#### **1.3. Consolidated Platform Capability Matrix**

The following matrix provides a definitive, at-a-glance summary of each platform's ability to function as a host or client under both architectural models. This table is the critical distillation of the feasibility study. The universal applicability of standard IP networking stacks makes Model A a single, unified engineering problem. In contrast, the fragmentation of OS-level APIs for device-to-device networking makes Model B a series of separate, complex engineering challenges.

| Platform | Model A: Internet-Based (LDN-over-IP) Host Capability | Model A: Internet-Based (LDN-over-IP) Client Capability | Model B: True Offline (Device-to-Device) Host Capability | Model B: True Offline (Device-to-Device) Client Capability |
| :---- | :---- | :---- | :---- | :---- |
| **Windows** | Yes (Standard IP Stack) | Yes (Standard IP Stack) | Yes (Mobile Hotspot/Wi-Fi Direct APIs) | Yes (Wi-Fi Direct/Standard Wi-Fi APIs) |
| **Linux** | Yes (Standard IP Stack) | Yes (Standard IP Stack) | Yes (High Complexity) (hostapd/wpa\_supplicant) | Yes (wpa\_supplicant/nmcli) |
| **macOS** | Yes (Standard IP Stack) | Yes (Standard IP Stack) | Yes (High Risk) (Soft AP Workaround) | Yes (Standard Wi-Fi APIs) |
| **Android** | Yes (Standard IP Stack) | Yes (Standard IP Stack) | Yes (Wi-Fi Direct API) | Yes (Wi-Fi Direct API) |

*Table 1.1: Consolidated Platform Capability Matrix.*

### **Section 2: Foundational Architecture: High-Level Emulation (HLE) of nn::ldn**

#### **2.1. Overview of the HLE Abstraction Layer**

The core technical strategy that enables both multiplayer models is the High-Level Emulation (HLE) of the Nintendo Switch's nn::ldn (Local Discovery and Networking) system service. By creating a software module within Sudachi that intercepts the game's IPC calls to nn::ldn, the emulator can satisfy the game's requests while translating them into actions for either the Internet (Model A) or Offline Ad-Hoc (Model B) backend. The multiplayer subsystem will function as a "virtual" nn::ldn service, decoupling the game's networking logic from the physical hardware it was designed for.

#### **2.2. The nn::ldn HLE Interface: The Internal API Contract**

The HLE module must implement the functions and data structures specified below to be compatible with games that utilize local wireless multiplayer.

| IPC Command | HLE Function Signature | Parameters | Description | Return Data Structure |
| :---- | :---- | :---- | :---- | :---- |
| Initialize | HleLdn::Initialize() | None | Initializes the nn::ldn HLE module and its active backend (Model A or B). | ResultCode |
| CreateNetwork | HleLdn::CreateNetwork(const NetworkConfig& config) | config: Game-provided settings (Game ID, version, player names, etc.). | Intercepts the request to host a session. Routes to the Model A Room Server or initiates the Model B ad-hoc network creation. | ResultCode |
| Scan | HleLdn::Scan(const ScanFilter& filter) | filter: Criteria for the scan (e.g., specific Game ID). | Intercepts the request to find sessions. Routes to the Model A Room Server or initiates a Model B mDNS discovery scan. | ResultCode |
| GetNetworkInfo | std::vector\<NetworkInfo\> HleLdn::GetNetworkInfo() | None | Returns a list of discovered networks in a format the game expects. | std::vector\<NetworkInfo\> |
| OpenAccessPoint | HleLdn::OpenAccessPoint() | None | A command used by the host to signal readiness to accept clients. | ResultCode |
| CloseAccessPoint | HleLdn::CloseAccessPoint() | None | A command used by the host to stop accepting new clients. | ResultCode |
| Connect | HleLdn::Connect(const NetworkInfo& target) | target: The NetworkInfo of the session to join. | Intercepts a client's request to join a session. Routes to the Model A P2P/Proxy flow or the Model B ad-hoc connection flow. | ResultCode |
| Disconnect | HleLdn::Disconnect() | None | Disconnects the client from the current session. | ResultCode |

*Table 2.1: nn::ldn HLE Interface Specification.*

#### **2.3. Internal Session State Management**

The multiplayer subsystem will require C++ classes to manage session state consistently across both models.

* **LdnSessionManager**: A primary manager class responsible for the overall state, holding the current configuration (Model A vs. Model B), and dispatching HLE commands to the appropriate backend.  
* **LdnNetworkInfo**: A C++ struct representing a discoverable game session, capable of storing all information for both the game's UI and connection logic.  
* **LdnClientInfo**: A structure representing a player connected to a session.

### **Section 3: Model A \- Internet-Based Multiplayer (LDN-over-IP)**

#### **3.1. System Architecture: The Hybrid Model**

The architecture for Model A shall be a hybrid system combining a centralized discovery server with peer-to-peer gameplay traffic.

1. **Centralized Room Server**: A lightweight WebSocket-based server for managing a list of active game "rooms." It handles registration from hosts and discovery requests from clients but does not handle gameplay traffic.  
2. **Peer-to-Peer (P2P) Client Connections**: The default method for routing gameplay data is a direct P2P connection between clients, established using modern NAT traversal techniques to minimize latency.  
3. **Proxy Server Fallback**: A mandatory relay server that provides a guaranteed connection path when a direct P2P connection cannot be established.

#### **3.2. Central Room Server Specification**

* **Protocol**: Communication shall be conducted exclusively over WebSockets using JSON-formatted messages.  
* **Infrastructure**: Node.js with Socket.IO for WebSocket management, Redis for session state, and PostgreSQL for persistent data.
* **Deployment**: Docker containers orchestrated with Kubernetes for horizontal scaling.
* **Load Balancing**: NGINX with sticky sessions for WebSocket connections.

**API Message Schema (JSON)**:
```json
// Client Registration
{
  "type": "register_client",
  "data": {
    "username": "string",
    "sudachi_version": "string",
    "platform": "windows|android|linux|macos"
  }
}

// Room Creation
{
  "type": "create_room",
  "data": {
    "game_id": "string (16 hex chars)",
    "game_name": "string",
    "max_players": "number (2-8)",
    "password": "string (optional)",
    "host_peer_id": "string (libp2p peer ID)",
    "network_config": {
      "nat_type": "open|moderate|strict",
      "preferred_relay": "string (region)"
    }
  }
}

// Room Discovery
{
  "type": "list_rooms",
  "data": {
    "filter": {
      "game_id": "string (optional)",
      "has_password": "boolean (optional)",
      "region": "string (optional)"
    },
    "pagination": {
      "offset": "number",
      "limit": "number (max 50)"
    }
  }
}

// Connection Request
{
  "type": "request_join",
  "data": {
    "room_id": "string (UUID)",
    "password": "string (optional)",
    "client_peer_id": "string (libp2p peer ID)"
  }
}

// P2P Failure Report
{
  "type": "p2p_failure",
  "data": {
    "room_id": "string",
    "error_code": "timeout|unreachable|nat_strict",
    "attempted_methods": ["tcp|quic|websocket"]
  }
}
```

**Error Response Schema**:
```json
{
  "type": "error",
  "data": {
    "code": "string (ERROR_CODE)",
    "message": "string (human readable)",
    "retry_after": "number (seconds, optional)"
  }
}
```

**Error Codes**:
- `AUTH_FAILED`: Authentication failure
- `ROOM_FULL`: Room at capacity
- `ROOM_NOT_FOUND`: Invalid room ID
- `VERSION_MISMATCH`: Incompatible Sudachi versions
- `RATE_LIMITED`: Too many requests
- `SERVER_ERROR`: Internal server error

#### **3.3. Client-Side P2P Networking and NAT Traversal**

* **Mandated Library**: The implementation shall use **cpp-libp2p** (C++17 implementation) integrated via Hunter package manager.
* **Transport Protocols**: TCP, QUIC, and WebSocket transports for maximum compatibility.
* **NAT Traversal**: Automatic hole punching using STUN-like behavior, with fallback to relay.

**Integration Steps**:
```cmake
# CMakeLists.txt additions
hunter_add_package(libp2p)
find_package(libp2p REQUIRED)

target_link_libraries(sudachi_multiplayer 
  libp2p::libp2p_core
  libp2p::libp2p_transport_tcp
  libp2p::libp2p_transport_websocket
  libp2p::libp2p_muxer_mplex
  libp2p::libp2p_security_noise
)
```

**Connection Flow**:
1. Host generates libp2p identity and starts listening
2. Host registers multiaddress with room server
3. Client retrieves host's multiaddress from room server
4. Client attempts direct connection using multiple transports
5. On failure, both parties receive relay server details

**P2P Configuration**:
```cpp
struct P2PConfig {
    std::vector<std::string> listen_addresses = {
        "/ip4/0.0.0.0/tcp/0",
        "/ip4/0.0.0.0/udp/0/quic",
        "/ip4/0.0.0.0/tcp/0/ws"
    };
    uint32_t connection_timeout_ms = 5000;
    uint32_t max_retry_attempts = 3;
    bool enable_relay_discovery = true;
};
```

#### **3.4. Proxy Server Fallback System**

* **Implementation**: High-performance relay using Boost.Asio with io_uring on Linux for optimal performance.
* **Protocol**: Custom binary protocol over TCP/UDP with header for session routing.

**Relay Protocol Header (12 bytes)**:
```cpp
struct RelayHeader {
    uint32_t session_token;  // 4 bytes - unique session ID
    uint16_t payload_size;   // 2 bytes - size of following data
    uint8_t  flags;          // 1 byte  - control flags
    uint8_t  reserved;       // 1 byte  - future use
    uint32_t sequence_num;   // 4 bytes - packet sequencing
};
```

**Relay Server Architecture**:
- Stateless design with session state in Redis
- Geographic distribution with anycast routing
- Automatic failover between relay servers
- Bandwidth limits: 10 Mbps per session
- Connection limits: 10,000 concurrent sessions per server

**Performance Targets**:
- Latency overhead: < 10ms (same region)
- Throughput: 95% of direct connection
- Packet loss: < 0.01% under normal conditions

### **Section 4: Model B \- True Offline Ad-Hoc Multiplayer**

#### **4.1. Architecture and Cross-Platform Interoperability**

Model B's goal is to replicate the Switch's "Local Play" feature for Windows and Android. The primary challenge is ensuring interoperability between the different wireless technologies on each OS.

* **Mandated Discovery Protocol**: To guarantee robust cross-platform discovery, the implementation shall use **Multicast DNS (mDNS) with DNS-Service Discovery (DNS-SD)**. A single, shared, cross-platform C library, **mjansson/mdns**, will be integrated to ensure identical protocol behavior on both Windows and Android.  
* **DNS-SD Service Record Specification**: All hosts in Model B must advertise their session using a standardized DNS-SD service record (\_sudachi-ldn.\_tcp) containing metadata like game name, game ID, host name, and player count in the TXT record.

#### **4.2. Android Implementation (Target: C++ with JNI)**

**JNI Interface Design**:
```cpp
// jni/wifi_direct_wrapper.h
class WifiDirectWrapper {
public:
    struct GroupInfo {
        std::string ssid;
        std::string passphrase;
        std::string ip_address;
        bool is_group_owner;
    };
    
    bool Initialize(JNIEnv* env, jobject context);
    bool CreateGroup(const std::string& group_name);
    bool Connect(const std::string& device_address);
    bool Disconnect();
    GroupInfo GetGroupInfo();
    
private:
    jobject wifi_p2p_manager_;
    jobject channel_;
    jmethodID create_group_method_;
    jmethodID connect_method_;
};
```

**JNI Method Signatures**:
```cpp
// Key JNI calls
const char* kWifiP2pManagerClass = "android/net/wifi/p2p/WifiP2pManager";
const char* kCreateGroupSig = "(Landroid/net/wifi/p2p/WifiP2pManager$Channel;"
                              "Landroid/net/wifi/p2p/WifiP2pManager$ActionListener;)V";
const char* kConnectSig = "(Landroid/net/wifi/p2p/WifiP2pManager$Channel;"
                          "Landroid/net/wifi/p2p/WifiP2pConfig;"
                          "Landroid/net/wifi/p2p/WifiP2pManager$ActionListener;)V";
```

**Permission Handling**:
```java
// Java helper class for runtime permissions
public class PermissionHelper {
    private static final String[] REQUIRED_PERMISSIONS = {
        Manifest.permission.NEARBY_WIFI_DEVICES,
        Manifest.permission.ACCESS_FINE_LOCATION,
        Manifest.permission.ACCESS_WIFI_STATE,
        Manifest.permission.CHANGE_WIFI_STATE
    };
    
    public static boolean checkAndRequestPermissions(Activity activity) {
        // Check SDK version for NEARBY_WIFI_DEVICES (Android 13+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Request NEARBY_WIFI_DEVICES
        } else {
            // Request ACCESS_FINE_LOCATION for older versions
        }
    }
}
```

**Connection Flow Implementation**:
1. Request permissions on first use
2. Register BroadcastReceiver for P2P state changes
3. Initialize WifiP2pManager and obtain Channel
4. For hosting: createGroup() with custom config
5. For joining: discoverPeers() then connect()
6. Handle WIFI_P2P_CONNECTION_CHANGED_ACTION
7. Extract IP addresses from WifiP2pInfo

**Error Handling**:
- `PERMISSION_DENIED`: Show rationale and guide to settings
- `P2P_UNSUPPORTED`: Fallback to internet mode
- `BUSY`: Retry after delay
- `ERROR`: Log and notify user

#### **4.3. Windows Implementation (Target: C++/WinRT)**

**C++/WinRT Implementation**:
```cpp
// mobile_hotspot_manager.h
#include <winrt/Windows.Networking.NetworkOperators.h>
#include <winrt/Windows.Networking.Connectivity.h>

class MobileHotspotManager {
public:
    struct HotspotConfig {
        std::wstring ssid;
        std::wstring passphrase;
        uint32_t max_clients = 8;
        TetheringWiFiBand band = TetheringWiFiBand::Auto;
    };
    
    bool Initialize();
    bool StartHotspot(const HotspotConfig& config);
    bool StopHotspot();
    TetheringOperationalState GetState();
    std::wstring GetLastError();
    
private:
    NetworkOperatorTetheringManager tethering_manager_{nullptr};
    bool CheckCapabilities();
    bool RequestElevation();
};
```

**Implementation Details**:
```cpp
bool MobileHotspotManager::StartHotspot(const HotspotConfig& config) {
    try {
        // Get internet connection profile
        auto profile = NetworkInformation::GetInternetConnectionProfile();
        if (!profile) {
            last_error_ = L"No internet connection available";
            return false;
        }
        
        // Create tethering manager
        tethering_manager_ = NetworkOperatorTetheringManager::
            CreateFromConnectionProfile(profile);
        
        // Configure access point
        auto ap_config = tethering_manager_.GetCurrentAccessPointConfiguration();
        ap_config.Ssid(config.ssid);
        ap_config.Passphrase(config.passphrase);
        ap_config.Band(config.band);
        
        // Apply configuration
        auto result = co_await tethering_manager_.
            ConfigureAccessPointAsync(ap_config);
        
        if (result.Status() != TetheringOperationStatus::Success) {
            HandleError(result);
            return false;
        }
        
        // Start tethering
        auto start_result = co_await tethering_manager_.StartTetheringAsync();
        return start_result.Status() == TetheringOperationStatus::Success;
    }
    catch (hresult_error const& ex) {
        last_error_ = ex.message();
        return false;
    }
}
```

**Capability Requirements**:
```xml
<!-- Package.appxmanifest -->
<Package ...>
  <Capabilities>
    <DeviceCapability Name="wiFiControl"/>
    <!-- May require restricted capability for some operations -->
    <rescap:Capability Name="onDemandHotspotControl"/>
  </Capabilities>
</Package>
```

**Fallback Strategy**:
1. Check Windows version (1607+ required)
2. Verify Mobile Hotspot availability
3. Request admin elevation if needed
4. Fall back to Wi-Fi Direct if hotspot fails
5. Final fallback to internet mode

**Error Codes**:
- `E_ACCESSDENIED`: Need admin privileges
- `E_INVALIDARG`: Invalid configuration
- `E_NOTIMPL`: Feature not available
- `ERROR_NOT_SUPPORTED`: Hardware limitation

#### **4.4. User Experience (UX) Flow for Both Models**

The user experience is designed to be seamless and managed from a single point in the emulator's settings.

1. **Mode Selection**: The user navigates to the Sudachi Settings menu. They will find a "Multiplayer Mode" setting with two options:  
   * **Internet Multiplayer (Default)**  
   * **Offline Ad-Hoc Multiplayer**  
2. By default, **Internet Multiplayer** is selected. In this mode, all multiplayer actions within a game are automatically routed through the Model A (LDN-over-IP) system. The process is transparent to the user, with internet rooms appearing directly in the game's own multiplayer menu.  
3. If the user selects **Offline Ad-Hoc Multiplayer**:  
   * The first time this option is enabled, Sudachi will trigger the necessary OS-level permission prompts (e.g., Location and Nearby Devices on Android, administrative elevation for Wi-Fi control on Windows). The user must grant these permissions.  
   * With this mode active, all multiplayer actions are routed through the Model B backend.  
4. **Hosting a Game**: The user selects "Create Room" (or equivalent) in the game's menu. Sudachi's HLE module intercepts this.  
   * In **Internet Mode**, it calls the Central Room Server to list the room publicly.  
   * In **Offline Mode**, it creates the local Wi-Fi Direct group or Mobile Hotspot and begins the mDNS advertisement.  
5. **Joining a Game**: The user selects "Find Room" (or equivalent).  
   * In **Internet Mode**, Sudachi fetches and displays the public room list from the server within the game's UI.  
   * In **Offline Mode**, Sudachi performs an mDNS scan and displays discovered local games within the game's UI.  
6. **Connection Acceptance (Offline Mode on Android Host)**: If the host is an Android device in Offline Mode, they will receive a system-level notification to "Accept" or "Decline" the incoming connection request from a client. The host must manually accept this for the connection to succeed.

### **Section 5: Security and Configuration Management**

#### **5.1. Security Architecture**

**Authentication System**:
- Client authentication using JWT tokens with 24-hour expiry
- HMAC-SHA256 signature verification for all API requests
- Rate limiting: 100 requests per minute per client
- IP-based blocking for abuse prevention

**Encryption**:
```cpp
struct SecurityConfig {
    // Model A - Internet traffic encryption
    bool enable_tls = true;
    std::string tls_version = "1.3";
    std::vector<std::string> cipher_suites = {
        "TLS_AES_128_GCM_SHA256",
        "TLS_AES_256_GCM_SHA384"
    };
    
    // P2P communication security
    bool enable_noise_protocol = true;  // libp2p Noise protocol
    uint32_t key_rotation_hours = 24;
};
```

**Anti-Cheat Measures**:
- Packet integrity verification using CRC32
- Session replay prevention with nonces
- Host validation to prevent malicious rooms
- Game version enforcement

#### **5.2. Configuration Management**

**Configuration Schema (JSON)**:
```json
{
  "multiplayer": {
    "mode": "internet|adhoc",
    "username": "string",
    "room_server": {
      "primary_url": "wss://rooms.sudachi.org",
      "fallback_urls": [
        "wss://rooms-us.sudachi.org",
        "wss://rooms-eu.sudachi.org"
      ],
      "reconnect_interval_ms": 5000,
      "max_reconnect_attempts": 10
    },
    "p2p": {
      "enable_upnp": true,
      "enable_nat_pmp": true,
      "stun_servers": [
        "stun:stun.l.google.com:19302",
        "stun:stun.sudachi.org:3478"
      ],
      "preferred_transport": "quic|tcp|websocket"
    },
    "relay": {
      "max_bandwidth_mbps": 10,
      "preferred_regions": ["us-west", "eu-central"]
    },
    "adhoc": {
      "ssid_prefix": "Sudachi-",
      "channel": 6,
      "security": "wpa2"
    },
    "advanced": {
      "enable_diagnostics": false,
      "log_level": "info|debug|trace",
      "packet_capture": false
    }
  }
}
```

**Configuration Paths**:
- Windows: `%APPDATA%\Sudachi\config\multiplayer.json`
- Android: `/data/data/org.sudachi.sudachi_emu/files/config/multiplayer.json`
- Linux: `~/.config/sudachi/multiplayer.json`
- macOS: `~/Library/Application Support/Sudachi/multiplayer.json`

**Migration from Legacy Settings**:
```cpp
class ConfigMigration {
public:
    static bool MigrateFromLegacy() {
        // Check for old room-based settings
        if (HasLegacyConfig()) {
            // Extract username if present
            auto legacy = LoadLegacyConfig();
            
            // Create new config with sensible defaults
            MultiplayerConfig new_config;
            new_config.username = legacy.username;
            new_config.mode = MultiplayerMode::Internet;
            
            // Save and remove old config
            new_config.Save();
            RemoveLegacyConfig();
            return true;
        }
        return false;
    }
};
```

### **Section 6: Implementation Roadmap and Strategic Guidance**

#### **6.1. Parallel Development Strategy**

The analysis supports a parallel development path for Windows and Android to ensure both multiplayer models are built cohesively. The implementation will be phased to manage complexity and deliver value incrementally. Implementations for macOS and Linux for Model B are deferred indefinitely due to high complexity and risk.

#### **6.2. Unified Implementation Plan**

**Phase 1: Foundational Work & Core Infrastructure**

* **Goal**: Establish the core HLE module and the backend infrastructure for both models.  
* **Tasks**:  
  1. Implement the core nn::ldn HLE interface (Section 2.2) within Sudachi.  
  2. Develop and deploy the lightweight Central Room Server (WebSockets) and the simple TCP/UDP Proxy Server for Model A.  
  3. Integrate the mjansson/mdns C library into the Sudachi build system for Model B.  
  4. Implement the "Multiplayer Mode" toggle in the Sudachi settings UI for Windows and Android.

**Phase 2: Model A Rollout (Internet Multiplayer)**

* **Goal**: Achieve a stable, end-to-end internet multiplayer experience.  
* **Tasks**:  
  1. Implement the client-side logic to communicate with the Room Server, initially forcing all connections through the proxy server for a proof-of-concept.  
  2. Integrate the libp2p C++ library into the build system.  
  3. Implement the full P2P connection flow (Section 3.3) with automatic, graceful fallback to the proxy server upon P2P failure.  
* **Success Metric**: A high percentage of connection attempts between users on varied network types successfully establish a direct P2P connection. The system is stable in latency-sensitive titles.

**Phase 3: Model B Rollout (Offline Ad-Hoc Multiplayer)**

* **Goal**: Achieve stable, offline multiplayer between Windows and Android devices.  
* **Tasks**:  
  1. Implement the Android backend using JNI to control the WifiP2pManager APIs.  
  2. Implement the Windows backend using C++/WinRT to control the Mobile Hotspot feature.  
  3. Thoroughly test interoperability, ensuring a Windows host can be discovered and joined by an Android client, and vice-versa.  
  4. Refine the UX around permission requests and the Android connection acceptance prompt.  
* **Success Metric**: Users can successfully host and join offline games between any combination of supported Windows and Android devices.

**Phase 4: Advanced Features and Stabilization**

* **Goal**: Deliver a feature-complete, best-in-class experience.  
* **Tasks**:  
  1. Implement a local network discovery mode for Model A to allow Sudachi users to play with modified Nintendo Switch consoles running ldn\_mitm on the same LAN.  
  2. Investigate and implement more complex nn::ldn service features, such as host migration.  
  3. Refine UI/UX, improve error reporting, and harden the server infrastructure and client-side networking logic.

### **Section 7: Performance Requirements and Error Handling**

#### **7.1. Performance Requirements**

**Connection Establishment**:
- Initial connection: < 3 seconds (Model A with good network)
- P2P negotiation: < 5 seconds including fallback decision
- Ad-hoc discovery: < 2 seconds for local network scan
- Reconnection after disconnect: < 1 second

**Latency Targets**:
- Local Ad-hoc (Model B): < 5ms additional overhead
- P2P Direct (Model A): < 20ms additional overhead
- Relay Server (Model A): < 50ms additional overhead
- Maximum jitter: < 10ms for stable connections

**Throughput Requirements**:
- Minimum bandwidth: 256 Kbps per player
- Recommended bandwidth: 1 Mbps per player
- Maximum packet size: 1400 bytes (MTU safe)
- Packet frequency: Up to 60 Hz for real-time games

**Scalability Targets**:
- Room Server: 100,000 concurrent connections per instance
- Relay Server: 10,000 concurrent sessions per instance
- Maximum players per session: 8 (game dependent)
- Room list query: < 100ms response time

#### **7.2. Error Handling Framework**

**Error Categories and Recovery**:

```cpp
enum class ErrorCategory {
    NETWORK_CONNECTIVITY,    // Recoverable with retry
    PERMISSION_DENIED,       // Requires user action
    CONFIGURATION_ERROR,     // Requires settings change
    PROTOCOL_MISMATCH,      // Requires update
    RESOURCE_EXHAUSTED,     // Temporary limitation
    SECURITY_VIOLATION,     // Connection terminated
    HARDWARE_LIMITATION     // Feature unavailable
};

struct ErrorInfo {
    ErrorCategory category;
    uint32_t error_code;
    std::string message;
    std::optional<uint32_t> retry_after_seconds;
    std::vector<std::string> suggested_actions;
};
```

**Common Error Scenarios**:

| Error Code | Description | User Message | Recovery Action |
|------------|-------------|--------------|-----------------|
| NET_001 | Connection timeout | "Unable to connect to multiplayer service" | Retry with exponential backoff |
| NET_002 | NAT traversal failed | "Could not establish direct connection" | Automatic relay fallback |
| PERM_001 | Wi-Fi permission denied | "Wi-Fi permission required for ad-hoc mode" | Guide to settings |
| PERM_002 | Location permission denied | "Location permission required for Wi-Fi Direct" | Show rationale dialog |
| CONF_001 | Invalid room configuration | "Game settings incompatible" | Show configuration UI |
| PROTO_001 | Version mismatch | "Please update Sudachi to play" | Link to update |
| RES_001 | Room full | "This room is at capacity" | Suggest other rooms |
| SEC_001 | Authentication failed | "Could not verify your identity" | Re-authenticate |
| HW_001 | Wi-Fi Direct unsupported | "Your device doesn't support ad-hoc mode" | Switch to internet mode |

**User Notification System**:

```cpp
class UserNotification {
public:
    enum class Level {
        INFO,     // Transient toast
        WARNING,  // Persistent banner
        ERROR,    // Modal dialog
        CRITICAL  // Full screen blocking
    };
    
    void Show(Level level, const ErrorInfo& error) {
        switch (level) {
            case Level::INFO:
                ShowToast(error.message, 3000);  // 3 second toast
                break;
            case Level::WARNING:
                ShowBanner(error.message, error.suggested_actions);
                break;
            case Level::ERROR:
                ShowDialog(error.message, error.suggested_actions);
                break;
            case Level::CRITICAL:
                ShowBlockingError(error);
                break;
        }
    }
};
```

### **Section 8: Appendix**

#### **8.1. Key Third-Party Dependencies**

| Library | Purpose | License | Integration Method | Version |
| :---- | :---- | :---- | :---- | :---- |
| cpp-libp2p | P2P Networking, NAT Traversal (Model A) | MIT/Apache 2.0 | Hunter | Latest |
| mjansson/mdns | mDNS/DNS-SD Discovery (Model B) | Public Domain | Header-only | 1.4+ |
| Boost.Asio | Asynchronous Networking for Proxy Server | Boost License 1.0 | Already integrated | 1.82+ |
| nlohmann/json | JSON parsing for API messages | MIT | Already integrated | 3.11+ |
| websocketpp | WebSocket client communication | BSD 3-Clause | Conan/vcpkg | 0.8.2+ |
| jwt-cpp | JWT token generation/validation | MIT | Header-only | 0.6+ |
| spdlog | Structured logging | MIT | Already integrated | 1.12+ |

*Table 8.1: External Dependencies.*

#### **8.2. Platform API Reference**

| Platform | API Namespace / Class | Purpose |
| :---- | :---- | :---- |
| **Android** | android.net.wifi.p2p.WifiP2pManager | Creating, discovering, and connecting to Wi-Fi Direct groups. Accessed via JNI. |
| **Android** | android.net.wifi.p2p.WifiP2pConfig | Configuring connection parameters for a Wi-Fi Direct peer. |
| **Android** | android.content.BroadcastReceiver | Listening for system-level Wi-Fi Direct state changes. |
| **Windows** | Windows::Networking::NetworkOperators::NetworkOperatorTetheringManager | Programmatic control of the "Mobile Hotspot" feature for hosting. Accessed via C++/WinRT. |
| **Windows** | Windows::Devices::WiFiDirect::WiFiDirectDevice | Discovering and connecting to other Wi-Fi Direct devices as a client. Accessed via C++/WinRT. |
| **Windows** | Windows::Devices::WiFi::WiFiAdapter | General Wi-Fi network management for connecting to Soft APs. Accessed via C++/WinRT. |

*Table 8.2: Required Platform-Specific APIs.*
