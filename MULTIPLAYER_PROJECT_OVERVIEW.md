# Sudachi Multiplayer Project Overview

## Project Summary

This project implements a comprehensive dual-mode multiplayer subsystem for the Sudachi Nintendo Switch emulator, enabling both internet-based and offline ad-hoc multiplayer capabilities across Windows and Android platforms.

## Existing Code Analysis

### Current Multiplayer Implementation (To Be Removed)

The Sudachi codebase contains legacy multiplayer code inherited from Yuzu/Citra that must be removed to make way for our new implementation:

#### 1. Network Infrastructure (`src/network/`)
- **Files**: room.h/cpp, room_member.h/cpp, packet.h/cpp, announce_multiplayer_session.h/cpp, verify_user.h/cpp
- **Purpose**: Room-based multiplayer system from Citra for 3DS emulation
- **Status**: Incompatible with our LDN-based approach, must be removed

#### 2. LDN Service Implementation (`src/core/hle/service/ldn/`)
- **Current Files**: 
  - lan_discovery.h/cpp
  - ldn.h/cpp
  - ldn_results.h, ldn_types.h
  - monitor_service.h/cpp, sf_monitor_service.h/cpp
  - sf_service.h/cpp, sf_service_monitor.h/cpp
  - system_local_communication_service.h/cpp
  - user_local_communication_service.h/cpp
- **Status**: Existing implementation uses the room-based network infrastructure
- **Action**: Will be completely rewritten to support our dual-mode architecture

#### 3. Qt Multiplayer UI (`src/sudachi/multiplayer/`)
- **All Files**: chat_room, client_room, direct_connect, host_room, lobby, message, moderation_dialog, state, validation
- **Purpose**: GUI for room-based multiplayer
- **Status**: Incompatible with LDN approach, must be removed

## New Architecture Overview

### Model A: Internet-Based Multiplayer (LDN-over-IP)

**Components:**
1. **Central Room Server** - WebSocket-based discovery server
2. **P2P Connections** - Using cpp-libp2p for NAT traversal
3. **Proxy Fallback** - TCP/UDP relay for failed P2P connections

**Key Technologies:**
- **cpp-libp2p**: C++17 implementation of libp2p for P2P networking
- **WebSocket**: For room server communication
- **JSON**: For API message formatting

### Model B: Offline Ad-Hoc Multiplayer

**Components:**
1. **Discovery Protocol** - mDNS/DNS-SD using mjansson/mdns
2. **Android Implementation** - Wi-Fi Direct via JNI
3. **Windows Implementation** - Mobile Hotspot via C++/WinRT

**Key Technologies:**
- **mjansson/mdns**: Public domain C library for cross-platform mDNS
- **Android WifiP2pManager**: Accessed via JNI for Wi-Fi Direct
- **Windows NetworkOperatorTetheringManager**: C++/WinRT API for Mobile Hotspot

## Technical Clarifications

### 1. cpp-libp2p Integration
- Repository: https://github.com/libp2p/cpp-libp2p
- Build System: Hunter package manager
- Requirements: C++17 compiler, CMake 3.15+
- Key Components:
  - Transport: TCP, QUIC, WebSocket
  - Security: Noise protocol encryption
  - Muxing: mplex for stream multiplexing
  - NAT Traversal: AutoNAT, Circuit Relay v2

### 2. mDNS Cross-Platform Implementation
- Library: mjansson/mdns v1.4+ (public domain)
- Integration: Single header + source file
- Memory Model: Zero allocations, user-provided buffers
- Discovery Protocol:
  ```
  Service: _sudachi-ldn._tcp.local
  TXT Records: game_id, version, players, password_protected
  Port: Dynamic (49152-65535)
  ```

### 3. Platform-Specific Implementation Details

#### Android (JNI Integration)
```cpp
// JNI Wrapper Structure
class AndroidMultiplayer {
    JavaVM* jvm;
    jobject wifi_manager;
    jmethodID create_group_id;
    jmethodID connect_id;
    
    // Callbacks from Java
    static void JNICALL OnConnectionStateChanged(JNIEnv*, jobject, jint state);
    static void JNICALL OnPeersAvailable(JNIEnv*, jobject, jobjectArray peers);
};

// Required Permissions (AndroidManifest.xml)
<uses-permission android:name="android.permission.NEARBY_WIFI_DEVICES" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.CHANGE_WIFI_STATE" />
```

#### Windows (C++/WinRT)
```cpp
// Mobile Hotspot Control
winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringManager manager;
auto profile = NetworkInformation::GetInternetConnectionProfile();
manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);

// Required Capabilities (Package.appxmanifest)
<DeviceCapability Name="wiFiControl"/>
<rescap:Capability Name="onDemandHotspotControl"/> // If needed
```

## Architecture Decisions

### 1. HLE Module Design
```cpp
namespace HleLdn {
    // Core interface matching nn::ldn IPC commands
    class LdnService : public ServiceFramework<LdnService> {
        void Initialize(HLERequestContext& ctx);
        void CreateNetwork(HLERequestContext& ctx);
        void Scan(HLERequestContext& ctx);
        void Connect(HLERequestContext& ctx);
        
    private:
        std::unique_ptr<IMultiplayerBackend> backend;
        MultiplayerMode current_mode;
    };
    
    // Backend interface for Model A/B switching
    class IMultiplayerBackend {
        virtual ResultCode CreateNetwork(const NetworkConfig&) = 0;
        virtual ResultCode Scan(const ScanFilter&) = 0;
        virtual ResultCode Connect(const NetworkInfo&) = 0;
    };
}
```

### 2. Network Protocol Design
```cpp
// Packet Structure (using custom binary protocol)
struct LdnPacketHeader {
    uint16_t magic = 0x4C44;  // "LD" 
    uint16_t version = 1;
    uint32_t session_id;
    uint16_t packet_type;
    uint16_t payload_size;
    uint32_t sequence_num;
    uint32_t timestamp_ms;
    uint8_t  player_id;
    uint8_t  flags;
    uint16_t checksum;
};

enum class PacketType : uint16_t {
    GAME_DATA = 0x0001,
    KEEP_ALIVE = 0x0002,
    STATE_SYNC = 0x0003,
    PLAYER_JOIN = 0x0004,
    PLAYER_LEAVE = 0x0005,
    HOST_MIGRATION = 0x0006
};
```

### 3. Comprehensive Testing Infrastructure
- **Unit Testing**: Google Test + Google Mock
  - Mock network interfaces
  - Simulated packet loss/latency
  - Platform API mocking
  
- **Integration Testing**: 
  - Docker-based test environment
  - Multiple emulator instances
  - Network condition simulation (tc/netem)
  
- **Game-Specific Testing**:
  - Automated test suites per game
  - Save state loading for reproducible tests
  - Performance regression tracking

## Dependencies Summary

| Library | Purpose | License | Integration Method |
|---------|---------|---------|-------------------|
| cpp-libp2p | P2P networking (Model A) | MIT/Apache 2.0 | Hunter |
| mjansson/mdns | mDNS discovery (Model B) | Public Domain | Header-only |
| Boost.Asio | Proxy server networking | Boost License | Already in project |
| nlohmann/json | JSON parsing | MIT | Already in project |
| websocketpp | WebSocket client | MIT | Conan/vcpkg |

## Known Challenges

1. **Android Wi-Fi Direct Limitations**
   - Requires user approval for each connection
   - JNI complexity for C++ integration
   - Permission handling at runtime

2. **Windows Mobile Hotspot**
   - Requires administrative privileges
   - May need restricted capability declaration
   - Conflicts with Wi-Fi Direct usage

3. **Cross-Platform Interoperability**
   - Ensuring Windows/Android devices can discover each other
   - Handling different network configurations
   - Testing across various device combinations

## Server Infrastructure Details

### Room Server Architecture
```javascript
// Technology Stack
- Node.js 18+ with TypeScript
- Socket.IO for WebSocket management
- Redis for session state and pub/sub
- PostgreSQL for persistent data
- Docker + Kubernetes for deployment

// Scalability Design
- Horizontal scaling with sticky sessions
- Redis Sentinel for HA
- PostgreSQL read replicas
- CDN for static assets
```

### Relay Server Implementation
```cpp
// High-performance C++ relay using Boost.Asio
class RelayServer {
    boost::asio::io_context io_context;
    std::unordered_map<uint32_t, SessionPair> sessions;
    
    // io_uring support on Linux for maximum performance
    #ifdef __linux__
    boost::asio::linux::io_uring_executor executor;
    #endif
};

// Deployment: Multiple regions with GeoDNS
// Capacity: 10,000 concurrent sessions per instance
// Bandwidth: 10 Gbps per region
```

## Build System Integration

### Hunter Configuration
```cmake
# HunterGate setup
include(HunterGate)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.24.18.tar.gz"
    SHA1 "4a3d7a2c17e69c6e31c23df67e321c5d475b4c8e"
)

# Dependencies
hunter_add_package(Boost COMPONENTS system asio)
hunter_add_package(libp2p)
hunter_add_package(nlohmann_json)
hunter_add_package(OpenSSL)
```

### Android JNI Build
```cmake
# Android-specific configuration
if(ANDROID)
    find_library(log-lib log)
    target_link_libraries(sudachi_multiplayer ${log-lib})
    
    # JNI sources
    target_sources(sudachi_multiplayer PRIVATE
        android/jni/wifi_direct_wrapper.cpp
        android/jni/permission_helper.cpp
    )
endif()
```

## Next Steps

1. **Phase 0**: Environment setup and legacy removal (Week 1)
2. **Phase 1**: Core infrastructure and HLE interface (Weeks 2-4)
3. **Phase 2**: Model A implementation with P2P/Relay (Weeks 5-8)
4. **Phase 3**: Model B implementation for Windows/Android (Weeks 9-12)
5. **Phase 4**: Integration testing and optimization (Weeks 13-16)