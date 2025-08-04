# Sudachi Multiplayer Implementation Plan v3.0

## Overview

This is the comprehensive implementation plan for the Sudachi multiplayer subsystem. It provides specific, actionable tasks with clear references to detailed specifications. Each task includes explicit prompts, file paths, and documentation references to ensure correct implementation without guesswork.

**Last Updated:** August 2, 2025  
**Status:** Ready for implementation  
**Package Manager Decision:** vcpkg (already integrated in Sudachi)  
**Server Stack:** Node.js with TypeScript  

## Phase 0: Legacy Code Removal and Environment Setup ✅ COMPLETED

### Task 0.1: Remove Legacy Multiplayer Code ✅ COMPLETED
**Coding Agent Prompt:**
"Remove all legacy multiplayer code from the Sudachi codebase. This is a critical first step that must be completed before any new implementation work."

**Reference Documentation:**
- See: `LEGACY_CODE_REMOVAL_PLAN.md` - Complete file list and removal strategy

**Specific Actions:**
1. **Remove Network Infrastructure:**
   ```bash
   git rm -r src/network/
   ```
   This removes the entire legacy room-based multiplayer system.

2. **Remove Qt Multiplayer UI:**
   ```bash
   git rm -r src/sudachi/multiplayer/
   ```
   This removes all legacy UI components (chat rooms, lobbies, etc.).

3. **Remove Common Header:**
   ```bash
   git rm src/common/announce_multiplayer_room.h
   ```

4. **Update CMake Files:**
   - Edit `src/CMakeLists.txt`: Remove line `add_subdirectory(network)`
   - Edit `src/sudachi/CMakeLists.txt`: Remove all references to multiplayer UI files
   - Search for any other CMake files that might reference these modules

5. **Update Main Window:**
   - Edit `src/sudachi/main.cpp` and `src/sudachi/main.ui`: Remove multiplayer menu items and actions
   - Remove any includes of multiplayer headers
   - Remove multiplayer state management code

6. **Verify Clean Removal:**
   ```bash
   # Search for any remaining references
   grep -r "Network::RoomNetwork" src/
   grep -r "announce_multiplayer_room" src/
   grep -r "multiplayer/" src/
   ```

**Success Criteria:**
- All legacy files removed
- Project builds without errors
- No orphaned includes or references remain

### Task 0.2: Configure Package Manager Decision

**Coding Agent Prompt:**
"Sudachi already uses vcpkg as its package manager (vcpkg.json exists in the project root). We will continue using vcpkg instead of Hunter to maintain consistency with the existing build system."

**Package Manager Decision Rationale:**
- Sudachi already has vcpkg integrated and configured
- vcpkg has 1381 official packages vs Hunter's smaller catalog
- vcpkg is actively maintained by Microsoft with regular updates
- Better cross-platform support (Windows, Linux, macOS, Android)
- Simpler integration with existing CMake configuration

**Implementation Actions:**

1. **Update vcpkg.json to include multiplayer dependencies:**
   ```json
   {
     "name": "sudachi",
     "version-string": "1.0.0",
     "dependencies": [
       // ... existing dependencies ...
       "websocketpp",
       "jwt-cpp",
       "nlohmann-json",
       "openssl",
       "spdlog"
     ]
   }
   ```

2. **Handle cpp-libp2p separately** (not available in vcpkg):
   ```cmake
   # In CMakeLists.txt or a separate cmake file
   include(FetchContent)
   FetchContent_Declare(
     cpp-libp2p
     GIT_REPOSITORY https://github.com/libp2p/cpp-libp2p
     GIT_TAG dad84a03a9651c7c2bb8a8f17d0e5ea67bd10b4f
   )
   FetchContent_MakeAvailable(cpp-libp2p)
   ```

3. **Handle mjansson/mdns** (header-only library):
   ```cmake
   # Download as part of the build
   file(DOWNLOAD 
     https://raw.githubusercontent.com/mjansson/mdns/main/mdns.h
     ${CMAKE_CURRENT_SOURCE_DIR}/externals/mdns/mdns.h
   )
   ```

**Success Criteria:**
- All dependencies properly configured in vcpkg.json
- cpp-libp2p integrated via FetchContent
- mjansson/mdns header downloaded
- Build system successfully finds all dependencies

### Task 0.3: Create Project Structure
**Coding Agent Prompt:**
"Create the new multiplayer directory structure with proper CMake integration. Each directory should have its own CMakeLists.txt file with appropriate source file listings and dependencies."

**Directory Structure to Create:**
```
src/core/multiplayer/
├── CMakeLists.txt
├── common/
│   ├── CMakeLists.txt
│   ├── packet_protocol.h
│   ├── packet_protocol.cpp
│   └── error_codes.h
├── model_a/
│   ├── CMakeLists.txt
│   ├── room_client.h
│   ├── room_client.cpp
│   ├── p2p_network.h
│   └── p2p_network.cpp
└── model_b/
    ├── CMakeLists.txt
    ├── mdns_discovery.h
    ├── mdns_discovery.cpp
    ├── platform/
    │   ├── android/
    │   └── windows/
```

## Phase 1: Core Infrastructure Implementation ✅ COMPLETED

### Task 1.1: Implement HLE Interface ✅ COMPLETED
**Coding Agent Prompt:**
"Implement the nn::ldn HLE interface exactly as specified in the PRD. Create a new service class `LdnService` that inherits from `ServiceFramework` and implements all IPC commands listed in Table 2.1. Each command must validate inputs and return appropriate ResultCodes."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 2.2 "The nn::ldn HLE Interface"
- See: `MULTIPLAYER_PROJECT_OVERVIEW.md` - Section "HLE Module Design"

**Primary Implementation Files:**
- `src/core/hle/service/ldn_new/ldn_service.h`
- `src/core/hle/service/ldn_new/ldn_service.cpp`

**Key Implementation Details:**
```cpp
// Follow this exact interface from PRD Section 2.2
class LdnService : public ServiceFramework<LdnService> {
public:
    explicit LdnService(Core::System& system);
    
private:
    // IPC Commands - implement each with validation
    void Initialize(HLERequestContext& ctx);
    void CreateNetwork(HLERequestContext& ctx);
    void Scan(HLERequestContext& ctx);
    // ... implement all commands from Table 2.1
};
```

### Task 1.2: Implement Configuration System ✅ COMPLETED
**Coding Agent Prompt:**
"Create a configuration management system for multiplayer settings. Implement JSON-based configuration with the exact schema specified in the PRD. Include migration logic from legacy settings and platform-specific config paths."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 5.2 "Configuration Management"
- Configuration Schema: PRD page contains complete JSON schema
- Migration Logic: See `ConfigMigration` class in PRD

**Implementation Files:**
- `src/core/multiplayer/common/config.h`
- `src/core/multiplayer/common/config.cpp`

**Platform-Specific Paths:**
```cpp
// Correct implementation for platform-specific paths
std::filesystem::path GetMultiplayerConfigPath() {
#ifdef _WIN32
    // Use SHGetKnownFolderPath or getenv for APPDATA
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / "Sudachi" / "config" / "multiplayer.json";
    }
    return std::filesystem::path("C:/Users/Default/AppData/Roaming/Sudachi/config/multiplayer.json");
#elif defined(__ANDROID__)
    // Android paths are provided by the Java layer via JNI
    return std::filesystem::path("/data/data/org.sudachi.sudachi_emu/files/config/multiplayer.json");
#elif defined(__linux__)
    // Expand home directory properly
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / "sudachi" / "multiplayer.json";
    }
    return std::filesystem::path("/home/user/.config/sudachi/multiplayer.json");
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "Sudachi" / "multiplayer.json";
    }
    return std::filesystem::path("/Users/user/Library/Application Support/Sudachi/multiplayer.json");
#endif
}
```

### Task 1.3: Create Packet Protocol ✅ COMPLETED
**Coding Agent Prompt:**
"Implement the binary packet protocol for LDN data transmission. Use the exact packet header structure from the Project Overview document. Include CRC32 checksum calculation and packet type definitions."

**Reference Documentation:**
- See: `MULTIPLAYER_PROJECT_OVERVIEW.md` - Section "Network Protocol Design"
- Packet Header: Use `LdnPacketHeader` struct exactly as specified

**Implementation Files:**
- `src/core/multiplayer/common/packet_protocol.h`
- `src/core/multiplayer/common/packet_protocol.cpp`

**Critical Implementation:**
```cpp
// Use this exact structure from the documentation
struct LdnPacketHeader {
    uint16_t magic = 0x4C44;  // "LD" 
    uint16_t version = 1;
    // ... rest of fields from documentation
};
```

## Phase 2: Model A - Internet Multiplayer

### Task 2.1: Implement WebSocket Room Client
**Coding Agent Prompt:**
"Create a WebSocket client that connects to the room server using the JSON message schemas from the PRD. Implement all message types including error handling and automatic reconnection with exponential backoff."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 3.2 "Central Room Server Specification"
- Message Schemas: Complete JSON schemas in PRD
- See: `SERVER_ARCHITECTURE.md` - WebSocket protocol details

**Implementation Files:**
- `src/core/multiplayer/model_a/room_client.h`
- `src/core/multiplayer/model_a/room_client.cpp`

**Required Message Handlers:**
```cpp
// Implement handlers for each message type in PRD Section 3.2
void HandleRegisterResponse(const json& message);
void HandleRoomCreated(const json& message);
void HandleRoomList(const json& message);
void HandleJoinResponse(const json& message);
void HandleP2PInfo(const json& message);
void HandleUseProxy(const json& message);
void HandleError(const json& message);
```

### Task 2.2: Integrate cpp-libp2p
**Coding Agent Prompt:**
"Integrate cpp-libp2p for P2P networking. Configure the library with TCP, QUIC, and WebSocket transports. Implement the connection flow described in PRD Section 3.3, including NAT traversal and automatic fallback to relay."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 3.3 "Client-Side P2P Networking"
- Configuration: Use `P2PConfig` struct from PRD

**Implementation Files:**
- `src/core/multiplayer/model_a/p2p_network.h`
- `src/core/multiplayer/model_a/p2p_network.cpp`

**Updated CMake Integration:**
```cmake
# Since cpp-libp2p is not in vcpkg, use FetchContent
include(FetchContent)

# cpp-libp2p is a C++20 library
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
  cpp-libp2p
  GIT_REPOSITORY https://github.com/libp2p/cpp-libp2p
  GIT_TAG dad84a03a9651c7c2bb8a8f17d0e5ea67bd10b4f
)
FetchContent_MakeAvailable(cpp-libp2p)

# Link the libraries
target_link_libraries(sudachi_multiplayer 
  libp2p::core
  libp2p::transport_tcp
  libp2p::transport_websocket
  libp2p::muxer_mplex
  libp2p::security_noise
)
```

**Key Implementation Notes:**
- cpp-libp2p is a C++20 implementation (ensure compiler support)
- Includes built-in NAT traversal via AutoNAT and Circuit Relay v2
- Supports Noise protocol for encryption by default
- Uses Hunter internally for its own dependencies

### Task 2.3: Implement Relay Client
**Coding Agent Prompt:**
"Create a relay client that connects to the proxy server when P2P fails. Implement the relay protocol with the exact header structure from PRD Section 3.4. Include session management and bandwidth limiting."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 3.4 "Proxy Server Fallback System"
- Protocol: Use `RelayHeader` struct exactly as specified
- See: `SERVER_ARCHITECTURE.md` - Relay server implementation details

**Implementation Files:**
- `src/core/multiplayer/model_a/relay_client.h`
- `src/core/multiplayer/model_a/relay_client.cpp`

## Phase 3: Model B - Offline Ad-Hoc Multiplayer

### Task 3.1: Implement mDNS Discovery
**Coding Agent Prompt:**
"Integrate the mjansson/mdns library for cross-platform service discovery. Implement service advertisement and scanning using the DNS-SD record format specified in the PRD. Use the exact service type `_sudachi-ldn._tcp`."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 4.1 "Architecture and Cross-Platform Interoperability"
- See: `MULTIPLAYER_PROJECT_OVERVIEW.md` - mDNS protocol details

**Implementation Files:**
- `src/core/multiplayer/model_b/mdns_discovery.h`
- `src/core/multiplayer/model_b/mdns_discovery.cpp`

**Integration Details:**
```cmake
# Download the header-only library
file(DOWNLOAD 
  https://raw.githubusercontent.com/mjansson/mdns/main/mdns.h
  ${CMAKE_CURRENT_SOURCE_DIR}/externals/mdns/mdns.h
  EXPECTED_HASH SHA256=<add_hash_here>
)
```

**Key Library Features:**
- Public domain, header-only C library
- Zero memory allocations (user provides buffers)
- Based on RFC 6762 and RFC 6763
- Cross-platform: Windows, Linux, macOS, Android
- Supports both discovery and service advertisement

**Service Record Format:**
```cpp
// Service type for Sudachi LDN
const char* SERVICE_TYPE = "_sudachi-ldn._tcp.local.";

// TXT record fields (DNS-SD format)
struct ServiceInfo {
    std::string game_id;     // 16 hex characters
    std::string version;     // Sudachi version
    uint8_t players;         // Current player count
    uint8_t max_players;     // Maximum players
    bool has_password;       // Password protected
};

// Convert to TXT record format
std::vector<uint8_t> CreateTxtRecord(const ServiceInfo& info) {
    // Format: key=value pairs with length prefix
    // Example: "\x07game_id=0100000000010000"
}
```

### Task 3.2: Android Wi-Fi Direct Implementation
**Coding Agent Prompt:**
"Implement Android Wi-Fi Direct support using JNI. Create the wrapper class exactly as specified in PRD Section 4.2, including all method signatures and permission handling. Implement the complete connection flow with proper error handling."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 4.2 "Android Implementation"
- Architecture: JNI bridges Java WifiP2pManager API to C++ code

**Implementation Files:**
- `src/core/multiplayer/model_b/platform/android/wifi_direct_wrapper.h`
- `src/core/multiplayer/model_b/platform/android/wifi_direct_wrapper.cpp`
- `src/android/app/src/main/java/org/sudachi/sudachi_emu/features/multiplayer/WifiDirectHelper.java`

**Android Architecture Overview:**
- SDK (Java API) → JNI Layer → HAL → wpa_supplicant → Kernel
- WifiP2pManager communicates with WifiP2pService via AsyncChannel binder
- State machine in WifiStateMachine handles driver communication

**Updated AndroidManifest.xml Requirements:**
```xml
<!-- Add these permissions -->
<uses-permission android:name="android.permission.NEARBY_WIFI_DEVICES" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.CHANGE_WIFI_STATE" />
<uses-permission android:name="android.permission.INTERNET" />

<!-- For Android 13+ -->
<uses-permission android:name="android.permission.NEARBY_WIFI_DEVICES"
                 android:usesPermissionFlags="neverForLocation" />
```

**Critical JNI Implementation:**
```cpp
// JNI method signatures
const char* kWifiP2pManagerClass = "android/net/wifi/p2p/WifiP2pManager";
const char* kCreateGroupSig = "(Landroid/net/wifi/p2p/WifiP2pManager$Channel;"
                              "Landroid/net/wifi/p2p/WifiP2pManager$ActionListener;)V";
const char* kConnectSig = "(Landroid/net/wifi/p2p/WifiP2pManager$Channel;"
                          "Landroid/net/wifi/p2p/WifiP2pConfig;"
                          "Landroid/net/wifi/p2p/WifiP2pManager$ActionListener;)V";

// Handle Android version differences
bool CheckAndRequestPermissions(JNIEnv* env, jobject activity) {
    // Android 13+ requires NEARBY_WIFI_DEVICES
    // Android 12 and below require ACCESS_FINE_LOCATION
    int sdk_version = android_get_device_api_level();
    if (sdk_version >= 33) { // Android 13+
        // Request NEARBY_WIFI_DEVICES
    } else {
        // Request ACCESS_FINE_LOCATION
    }
}
```

### Task 3.3: Windows Mobile Hotspot Implementation
**Coding Agent Prompt:**
"Implement Windows Mobile Hotspot control using C++/WinRT. Create the MobileHotspotManager class exactly as specified in PRD Section 4.3. Include capability checking, elevation handling, and the complete fallback strategy."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 4.3 "Windows Implementation"
- Note: NetworkOperatorTetheringManager is available for desktop apps (not just UWP)

**Implementation Files:**
- `src/core/multiplayer/model_b/platform/windows/mobile_hotspot_manager.h`
- `src/core/multiplayer/model_b/platform/windows/mobile_hotspot_manager.cpp`

**Important Windows Considerations:**
1. **API Availability:** 
   - Requires Windows 10 version 1607 or later
   - NetworkOperatorTetheringManager has DualApiPartition attribute (usable in desktop apps)
   - Mobile Hotspot and Wi-Fi Direct can't run simultaneously

2. **WLAN HostedNetwork Deprecated:**
   - Microsoft deprecated WLAN HostedNetwork in Windows 10
   - New model based on Wi-Fi Direct kernel integration
   - For legacy support, need Windows 8.1 or earlier drivers

3. **Implementation Approach for Desktop Apps:**
   ```cpp
   // Include Windows Runtime headers
   #include <winrt/Windows.Networking.NetworkOperators.h>
   #include <winrt/Windows.Networking.Connectivity.h>
   
   using namespace winrt;
   using namespace Windows::Networking::NetworkOperators;
   using namespace Windows::Networking::Connectivity;
   
   // For desktop apps, initialize Windows Runtime
   winrt::init_apartment();
   
   // Check for internet connection first
   auto profile = NetworkInformation::GetInternetConnectionProfile();
   if (!profile) {
       // No internet connection - Mobile Hotspot requires one
       return false;
   }
   
   // Create tethering manager
   auto manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
   ```

4. **Fallback Strategy:**
   - Primary: Mobile Hotspot via NetworkOperatorTetheringManager
   - Secondary: Wi-Fi Direct (if Mobile Hotspot unavailable)
   - Tertiary: Switch to Internet Multiplayer mode

**Note:** Since Sudachi is not a UWP app, manifest capabilities don't apply. Admin elevation may be required for some operations.

## Phase 4: Integration and Testing

### Task 4.1: Create Test Infrastructure
**Coding Agent Prompt:**
"Set up comprehensive testing infrastructure using Google Test and Google Mock. Create mock implementations for all network components and implement Docker-based integration test environment."

**Test Framework Setup:**
1. **Add Google Test to vcpkg.json:**
   ```json
   {
     "dependencies": [
       "gtest",
       "gmock"
     ]
   }
   ```

2. **CMake Configuration:**
   ```cmake
   enable_testing()
   find_package(GTest CONFIG REQUIRED)
   
   add_executable(sudachi_multiplayer_tests
     ${TEST_SOURCES}
   )
   
   target_link_libraries(sudachi_multiplayer_tests
     PRIVATE
     GTest::gtest
     GTest::gmock
     sudachi_multiplayer
   )
   
   include(GoogleTest)
   gtest_discover_tests(sudachi_multiplayer_tests)
   ```

**Test Structure:**
```
tests/
├── unit/
│   ├── test_hle_interface.cpp
│   ├── test_p2p_network.cpp
│   ├── test_mdns_discovery.cpp
│   └── test_packet_protocol.cpp
├── integration/
│   ├── test_model_a_flow.cpp
│   ├── test_model_b_flow.cpp
│   └── test_game_integration.cpp
├── mocks/
│   ├── mock_websocket_server.h
│   ├── mock_p2p_network.h
│   └── mock_platform_apis.h
└── docker/
    └── docker-compose.test.yml
```

**Docker Test Environment:**
```yaml
# docker-compose.test.yml
version: '3.8'
services:
  room-server:
    build: ./room-server
    ports:
      - "8080:8080"
    environment:
      - NODE_ENV=test
      
  relay-server:
    build: ./relay-server
    ports:
      - "3478:3478"
      
  test-runner:
    build: .
    depends_on:
      - room-server
      - relay-server
    volumes:
      - ./test-results:/results
    command: ctest --output-on-failure
```

**Mock Implementations:**
```cpp
// mock_websocket_server.h
class MockWebSocketServer : public IWebSocketServer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string& client_id, const json& message));
    MOCK_METHOD(void, OnConnect, (const std::string& client_id));
    MOCK_METHOD(void, OnDisconnect, (const std::string& client_id));
};

// mock_p2p_network.h
class MockP2PNetwork : public IP2PNetwork {
public:
    MOCK_METHOD(bool, Connect, (const std::string& peer_id));
    MOCK_METHOD(void, SendData, (const std::vector<uint8_t>& data));
    MOCK_METHOD(void, SetOnDataReceived, (std::function<void(const std::vector<uint8_t>&)>));
};
```

### Task 4.2: Implement UI Components
**Coding Agent Prompt:**
"Create all UI components exactly as specified in the UI/UX documentation. Implement the multiplayer mode toggle in Qt settings, connection status overlay, and all error dialogs with the exact visual designs provided."

**Reference Documentation:**
- See: `UI_UX_SPECIFICATIONS.md` - Complete visual specifications
- Settings UI: Section "Multiplayer Mode Toggle"
- Status Overlay: Section "Connection Status Overlay"
- Error Dialogs: Section "Error Notifications"

**Implementation Files:**
- `src/sudachi/configuration/configure_network.ui`
- `src/sudachi/multiplayer_status_widget.h`
- `src/sudachi/multiplayer_status_widget.cpp`

### Task 4.3: Error Handling Implementation
**Coding Agent Prompt:**
"Implement the complete error handling framework from PRD Section 7.2. Create the ErrorInfo structure, implement all error codes, and integrate the user notification system with appropriate UI elements."

**Reference Documentation:**
- See: `Sudachi Multiplayer PRD.md` - Section 7.2 "Error Handling Framework"
- Error Table: Complete list of error codes and messages
- UI Integration: See `UI_UX_SPECIFICATIONS.md` - Error notifications

**Implementation Files:**
- `src/core/multiplayer/common/error_handling.h`
- `src/core/multiplayer/common/error_handling.cpp`

## Testing and Validation

### Task 5.1: Game-Specific Testing
**Coding Agent Prompt:**
"Implement game-specific integration tests for popular Nintendo Switch titles that support LDN multiplayer. Create automated tests that verify multiplayer functionality for each supported game."

**Test Games Priority List:**
1. **Animal Crossing: New Horizons** (0100620011960000)
   - Local play: up to 8 players
   - Test: Island visits, item trading
   
2. **Super Smash Bros. Ultimate** (01006A800016E000)
   - Local wireless: up to 8 players
   - Test: Battle arenas, character sync
   
3. **Mario Kart 8 Deluxe** (0100152000022000)
   - Local wireless: up to 8 players
   - Test: Race synchronization, item state
   
4. **Splatoon 3** (0100C2500FC20000)
   - Local wireless: up to 8 players
   - Test: Salmon Run, Turf War

**Test Implementation:**
```cpp
class GameIntegrationTest : public ::testing::Test {
protected:
    void SetUpGame(const std::string& game_id) {
        // Load game-specific save state
        // Initialize LDN service
        // Set up network mocks
    }
    
    void TestMultiplayerSession(int num_players) {
        // Create host
        // Connect clients
        // Verify game state synchronization
        // Test gameplay actions
    }
};

TEST_F(GameIntegrationTest, AnimalCrossingLocalPlay) {
    SetUpGame("0100620011960000");
    TestMultiplayerSession(4);
}
```

### Task 5.2: Performance Benchmarking
**Coding Agent Prompt:**
"Implement performance benchmarks using Google Benchmark. Measure connection time, packet processing, and ensure all metrics meet the requirements from PRD Section 7.1."

**Benchmark Setup:**
```cpp
// Add to vcpkg.json
"benchmark"

// benchmark_multiplayer.cpp
#include <benchmark/benchmark.h>

static void BM_P2PConnection(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        // Perform P2P connection
        ConnectP2P("test_peer_id");
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(
            std::chrono::duration<double>(end - start).count()
        );
    }
}
BENCHMARK(BM_P2PConnection)->UseManualTime();

static void BM_PacketProcessing(benchmark::State& state) {
    std::vector<uint8_t> packet(1400); // MTU-sized packet
    for (auto _ : state) {
        ProcessPacket(packet);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PacketProcessing);
```

**Performance Targets to Validate:**
- P2P Connection: < 5 seconds (measure 95th percentile)
- Local Ad-hoc Latency: < 5ms overhead (measure with loopback)
- Relay Latency: < 50ms overhead (measure round-trip time)
- Packet Processing: 60Hz capability (16.67ms per frame)

## Critical Implementation Notes

1. **Legacy Code Removal First**: The legacy multiplayer code MUST be removed before any new implementation work begins. This is non-negotiable.

2. **Package Manager**: Use vcpkg (already integrated) instead of Hunter. Update vcpkg.json for new dependencies.

3. **Server Stack**: Use Node.js with TypeScript for room and relay servers as shown in SERVER_ARCHITECTURE.md.

4. **Platform-Specific APIs**:
   - Android: Use JNI to bridge to Java WifiP2pManager APIs
   - Windows: Use C++/WinRT for NetworkOperatorTetheringManager (works in desktop apps)

5. **Error Handling**: Implement the complete error framework from PRD Section 7.2 with unified error codes.

6. **Configuration**: Use proper path expansion for platform-specific config paths (no hardcoded environment variables).

7. **Performance Considerations**:
   - Local ad-hoc < 5ms overhead is challenging but achievable with zero-copy and careful optimization
   - Use io_uring on Linux for relay server performance
   - Implement packet coalescing for better throughput

8. **Security**: 
   - JWT tokens with 24-hour expiry
   - Use Noise protocol in libp2p for encryption
   - Implement rate limiting at all levels

## Server Infrastructure Notes

1. **Room Server**: Node.js + TypeScript + Socket.IO + Redis + PostgreSQL
2. **Relay Server**: High-performance C++ with Boost.Asio
3. **Deployment**: Kubernetes with horizontal scaling
4. **Monitoring**: Prometheus + Grafana setup required

## Completion Checklist

### Phase 0: Setup
- [ ] All legacy code removed (src/network/, src/sudachi/multiplayer/)
- [ ] vcpkg.json updated with multiplayer dependencies
- [ ] cpp-libp2p integrated via FetchContent
- [ ] mjansson/mdns header downloaded
- [ ] New directory structure created

### Phase 1: Core
- [ ] HLE interface matches PRD Table 2.1 exactly
- [ ] Configuration system with proper path handling
- [ ] Binary packet protocol with CRC32 checksums

### Phase 2: Internet Multiplayer
- [ ] WebSocket client with all message handlers
- [ ] cpp-libp2p integration with NAT traversal
- [ ] Relay client with bandwidth limiting

### Phase 3: Ad-Hoc Multiplayer
- [ ] mDNS discovery with _sudachi-ldn._tcp
- [ ] Android Wi-Fi Direct via JNI
- [ ] Windows Mobile Hotspot via C++/WinRT

### Phase 4: Integration
- [ ] Google Test/Mock infrastructure
- [ ] UI components matching specifications
- [ ] Complete error handling framework

### Phase 5: Validation
- [ ] Game-specific tests for 4+ titles
- [ ] Performance benchmarks meeting targets
- [ ] Security audit passed
- [ ] Documentation complete

## Known Issues to Address

1. **"Zero-Configuration" Contradiction**: The system requires mode selection and permissions, contradicting the zero-config claim. Consider defaulting to Internet mode with automatic fallback.

2. **Cross-Platform Discovery**: mDNS alone may not be sufficient. Consider adding a backup discovery mechanism.

3. **Performance Target Realism**: The < 5ms overhead for local ad-hoc is extremely aggressive. Consider revising to < 10ms.

4. **IPv6 Support**: Not mentioned in documentation but essential for future-proofing.

5. **Mobile Hotspot Limitations**: Windows API requires internet connection, which may not always be available for ad-hoc mode.

## Solutions to Known Issues

### 1. Zero-Configuration Contradiction Solution

**Issue**: System requires mode selection and permissions, contradicting zero-config claim.

**Solution**: Implement true zero-configuration networking following Zeroconf principles:
- **Default Mode**: Start with Internet multiplayer as default (most common use case)
- **Automatic Fallback**: If no internet connection detected, automatically switch to ad-hoc mode
- **Smart Detection**: Use connection profiling to remember user preferences per network
- **Permission Caching**: Store granted permissions to avoid repeated requests
- **Seamless Experience**: Hide technical details unless user explicitly opens advanced settings

**Implementation**:
```cpp
class MultiplayerModeSelector {
    MultiplayerMode SelectOptimalMode() {
        // 1. Check cached user preference for current network
        if (HasCachedPreference()) {
            return GetCachedPreference();
        }
        
        // 2. Check internet connectivity
        if (HasInternetConnection()) {
            return MultiplayerMode::Internet;
        }
        
        // 3. Check local network capabilities
        if (CanUseLocalNetwork()) {
            return MultiplayerMode::AdHoc;
        }
        
        // 4. Prompt user only if no automatic choice possible
        return PromptUserForMode();
    }
};
```

### 2. Cross-Platform Discovery Backup Mechanisms

**Issue**: mDNS alone may not be sufficient for reliable discovery across all platforms.

**Solution**: Implement multiple discovery mechanisms with automatic fallback:

1. **Primary: mDNS** (_sudachi-ldn._tcp.local)
   - Works on most local networks
   - Zero configuration required
   - Built into most operating systems

2. **Secondary: QR Code Exchange**
   - Generate QR code containing connection info (IP, port, session ID)
   - Cross-platform compatible
   - Works even when network discovery blocked
   
3. **Tertiary: Bluetooth Discovery**
   - For mobile devices (Android/iOS)
   - Low energy beacon advertisement
   - Fallback when WiFi unavailable

4. **Quaternary: Manual IP Entry**
   - Last resort option
   - Allow entering IP:port directly
   - Store recently used addresses

**Implementation**:
```cpp
class HybridDiscovery {
    std::vector<std::unique_ptr<IDiscoveryMethod>> methods = {
        std::make_unique<MdnsDiscovery>(),
        std::make_unique<QrCodeDiscovery>(),
        std::make_unique<BluetoothDiscovery>(),
        std::make_unique<ManualDiscovery>()
    };
    
    void DiscoverPeers() {
        for (auto& method : methods) {
            if (method->IsAvailable()) {
                method->StartDiscovery();
            }
        }
    }
};
```

### 3. Realistic Performance Targets for Local Ad-Hoc

**Issue**: < 5ms overhead target is extremely aggressive.

**Solution**: Revise performance targets based on real-world measurements:

**Updated Performance Targets**:
- **Local LAN (Ethernet)**: < 10ms total latency
- **Local WiFi Direct**: < 20ms total latency  
- **Local WiFi (via Router)**: < 30ms total latency
- **Internet (Regional)**: < 50ms total latency
- **Internet (Global)**: < 150ms total latency

**Optimization Strategies**:
1. **Zero-Copy Networking**: Use memory-mapped buffers
2. **Lock-Free Queues**: Minimize thread contention
3. **Packet Coalescing**: Batch small messages
4. **Direct Memory Access**: Bypass kernel when possible
5. **CPU Affinity**: Pin network threads to specific cores

**Benchmark Validation**:
```cpp
// Realistic benchmark targets
BENCHMARK(LocalLANLatency)->DenseRange(5, 15, 1);  // 5-15ms range
BENCHMARK(WiFiDirectLatency)->DenseRange(10, 25, 5); // 10-25ms range
BENCHMARK(WiFiRouterLatency)->DenseRange(15, 35, 5); // 15-35ms range
```

### 4. IPv6 Support Implementation

**Issue**: IPv6 support not mentioned but essential for future-proofing.

**Solution**: Implement dual-stack IPv4/IPv6 support throughout:

**Implementation Details**:
1. **libp2p**: Already supports IPv6 natively
   - Configure both /ip4/ and /ip6/ listen addresses
   - Prefer IPv4 for compatibility, but support IPv6

2. **mDNS**: Advertise both A (IPv4) and AAAA (IPv6) records
   ```cpp
   // Advertise both IPv4 and IPv6
   mdns_record_t records[] = {
       CreateARecord(ipv4_address),    // A record
       CreateAAAARecord(ipv6_address)  // AAAA record
   };
   ```

3. **Configuration**: Add IPv6 settings
   ```json
   {
     "multiplayer": {
       "networking": {
         "enable_ipv6": true,
         "ipv6_preferred": false,
         "dual_stack": true
       }
     }
   }
   ```

4. **Platform Considerations**:
   - Windows: IPv6 enabled by default since Vista
   - Linux: Check `/proc/sys/net/ipv6/conf/all/disable_ipv6`
   - macOS: IPv6 enabled by default
   - Android: IPv6 support varies by carrier

### 5. Windows Mobile Hotspot Workarounds

**Issue**: Windows Mobile Hotspot API requires internet connection.

**Solution**: Implement multiple fallback strategies:

**Primary Strategy: Microsoft Loopback Adapter**
```cpp
class WindowsHotspotManager {
    bool StartHotspotWithLoopback() {
        // 1. Create/verify loopback adapter exists
        auto loopback = CreateLoopbackAdapter();
        
        // 2. Create connection profile from loopback
        auto profile = CreateProfileFromAdapter(loopback);
        
        // 3. Start mobile hotspot using loopback profile
        auto manager = NetworkOperatorTetheringManager::
            CreateFromConnectionProfile(profile);
        
        return manager.StartTetheringAsync().get() == 
            TetheringOperationStatus::Success;
    }
};
```

**Fallback Strategies**:
1. **WiFi Direct API**: Use legacy mode for local connectivity
2. **Hosted Network**: Use netsh commands (requires elevation)
3. **Third-party Integration**: Support apps like "Hotspot Lite"
4. **Mode Switch**: Automatically suggest Internet mode if ad-hoc fails

**User Communication**:
```cpp
// Clear error message with workaround suggestion
if (!hotspot.Start()) {
    ShowError(
        "Windows requires an internet connection for Mobile Hotspot.\n\n"
        "Workarounds:\n"
        "1. Install Microsoft Loopback Adapter (recommended)\n"
        "2. Switch to Internet Multiplayer mode\n"
        "3. Use a third-party hotspot app\n"
        "[Help] [Switch Mode] [Cancel]"
    );
}
```

## Phase 6: Critical Integration and Security (NEW - REQUIRED FOR 100% COMPLETION)

### Task 6.1: HLE Service Integration - CRITICAL MISSING COMPONENT
**Coding Agent Prompt:**
"The legacy LDN service exists in `sudachi/src/core/hle/service/ldn/` but is NOT connected to the new multiplayer system. This is a critical blocking issue - games cannot access multiplayer without proper HLE integration. Replace the legacy implementations with new service classes that bridge to the multiplayer subsystem."

**Critical Integration Required:**
1. **Replace Legacy LDN Services:**
   ```cpp
   // Replace src/core/hle/service/ldn/user_local_communication_service.cpp
   class UserLocalCommunicationService : public ServiceFramework<UserLocalCommunicationService> {
   private:
       std::shared_ptr<Core::Multiplayer::MultiplayerBackend> multiplayer_backend_;
       
       // Bridge IPC commands to new multiplayer system
       void Initialize(HLERequestContext& ctx) {
           // Call into multiplayer_backend_->Initialize()
       }
       
       void Scan(HLERequestContext& ctx) {
           // Call into multiplayer_backend_->StartDiscovery()
       }
       
       void CreateNetwork(HLERequestContext& ctx) {
           // Call into multiplayer_backend_->CreateSession()
       }
   };
   ```

2. **Create Unified Multiplayer Backend Interface:**
   ```cpp
   // New file: src/core/multiplayer/multiplayer_backend.h
   class MultiplayerBackend {
   public:
       virtual Result Initialize() = 0;
       virtual Result CreateSession(const NetworkInfo& network_info) = 0;
       virtual Result StartDiscovery() = 0;
       virtual Result Connect(const NodeInfo& node) = 0;
       virtual Result SendData(const std::vector<uint8_t>& data) = 0;
       virtual void SetReceiveCallback(ReceiveDataCallback callback) = 0;
   };
   
   class ModelABackend : public MultiplayerBackend { /* Internet implementation */ };
   class ModelBBackend : public MultiplayerBackend { /* Ad-hoc implementation */ };
   ```

3. **Integration Points:**
   - Update `src/core/hle/service/ldn/ldn.cpp` to register new service classes
   - Modify `src/core/core.cpp` to initialize multiplayer subsystem
   - Add multiplayer backend selection based on configuration

**Success Criteria:**
- Games can successfully call LDN IPC commands
- LDN service routes calls to appropriate multiplayer backend (Model A or B)
- No legacy code remains in LDN service implementation

### Task 6.2: cpp-libp2p Build Integration - CRITICAL MISSING DEPENDENCY
**Coding Agent Prompt:**
"cpp-libp2p is referenced but not actually integrated into the build system. Based on research, cpp-libp2p requires specific build configuration and has complex dependencies. Implement proper integration using FetchContent with Hunter support."

**Research-Based Integration:**
1. **CMake Integration (Corrected):**
   ```cmake
   # cpp-libp2p requires C++17 minimum (not C++20 as assumed)
   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   
   # cpp-libp2p uses Hunter package manager internally
   include(FetchContent)
   
   FetchContent_Declare(
     cpp-libp2p
     GIT_REPOSITORY https://github.com/libp2p/cpp-libp2p
     GIT_TAG v0.1.0  # Use specific release tag
     CMAKE_ARGS 
       -DTESTING=OFF 
       -DEXAMPLES=OFF
       -DCLANG_TIDY=OFF  # Disable for faster builds
   )
   
   # Set Hunter package manager cache
   set(HUNTER_CACHE_SERVERS "https://github.com/cpp-pm/hunter-cache")
   
   FetchContent_MakeAvailable(cpp-libp2p)
   ```

2. **Required System Dependencies:**
   ```bash
   # Ubuntu/Debian
   apt-get install build-essential cmake ninja-build
   apt-get install libssl-dev libboost-all-dev
   
   # macOS
   brew install cmake ninja boost openssl
   
   # Windows (vcpkg)
   vcpkg install boost openssl
   ```

3. **Library Linking (Corrected):**
   ```cmake
   target_link_libraries(sudachi_multiplayer_model_a PRIVATE
     p2p::p2p_basic_host
     p2p::p2p_tcp_transport  
     p2p::p2p_websocket_transport
     p2p::p2p_mplex
     p2p::p2p_noise
     p2p::p2p_autonat
     p2p::p2p_relay
   )
   ```

**Success Criteria:**
- cpp-libp2p builds successfully on all target platforms
- P2P networking functionality actually works (not just stubs/mocks)
- NAT traversal and relay fallback operational

### Task 6.3: Network Security Implementation - CRITICAL SECURITY GAPS
**Coding Agent Prompt:**
"The network components lack essential security features including input validation, rate limiting, and DDoS protection. Based on C++ security best practices research, implement comprehensive security framework."

**Research-Based Security Implementation:**

1. **Input Validation Framework:**
   ```cpp
   class NetworkInputValidator {
   public:
       static bool ValidatePacket(const std::vector<uint8_t>& data) {
           // Size validation
           if (data.size() > MAX_PACKET_SIZE || data.size() < MIN_PACKET_SIZE) {
               return false;
           }
           
           // Header validation
           if (data.size() >= sizeof(LdnPacketHeader)) {
               auto header = reinterpret_cast<const LdnPacketHeader*>(data.data());
               if (header->magic != 0x4C44 || header->version != 1) {
                   return false;
               }
           }
           
           // Content validation using regex/pattern matching
           return ValidatePacketContent(data);
       }
       
       static bool ValidateJsonMessage(const std::string& json_str) {
           // Prevent JSON bomb attacks
           if (json_str.size() > MAX_JSON_SIZE) return false;
           
           // Validate JSON structure
           try {
               auto j = nlohmann::json::parse(json_str);
               return ValidateJsonSchema(j);
           } catch (...) {
               return false;
           }
       }
   };
   ```

2. **Rate Limiting Implementation:**
   ```cpp
   class TokenBucketRateLimit {
   private:
       std::chrono::steady_clock::time_point last_refill_;
       double tokens_;
       const double capacity_;
       const double refill_rate_;  // tokens per second
       mutable std::mutex mutex_;
       
   public:
       TokenBucketRateLimit(double capacity, double rate) 
           : capacity_(capacity), refill_rate_(rate), tokens_(capacity) {}
       
       bool TryConsume(double tokens = 1.0) {
           std::lock_guard<std::mutex> lock(mutex_);
           Refill();
           
           if (tokens_ >= tokens) {
               tokens_ -= tokens;
               return true;
           }
           return false;
       }
   };
   
   // Per-client rate limiting
   class ClientRateManager {
       std::unordered_map<std::string, TokenBucketRateLimit> client_limits_;
   public:
       bool CheckRateLimit(const std::string& client_id) {
           auto& limiter = client_limits_[client_id];
           return limiter.TryConsume();
       }
   };
   ```

3. **DDoS Protection:**
   ```cpp
   class DDoSProtection {
   private:
       std::atomic<size_t> active_connections_{0};
       std::unordered_map<std::string, size_t> connection_counts_;
       std::mutex protection_mutex_;
       
   public:
       bool AllowNewConnection(const std::string& ip_address) {
           std::lock_guard<std::mutex> lock(protection_mutex_);
           
           // Global connection limit
           if (active_connections_ >= MAX_TOTAL_CONNECTIONS) {
               LOG_WARNING("Connection rejected: global limit reached");
               return false;
           }
           
           // Per-IP connection limit
           if (connection_counts_[ip_address] >= MAX_CONNECTIONS_PER_IP) {
               LOG_WARNING("Connection rejected: IP limit reached for {}", ip_address);
               return false;
           }
           
           active_connections_++;
           connection_counts_[ip_address]++;
           return true;
       }
   };
   ```

**Success Criteria:**
- All network inputs validated before processing
- Rate limiting prevents abuse from individual clients
- DDoS protection limits resource exhaustion
- Security audit passes with no critical vulnerabilities

### Task 6.4: Windows Platform Integration - MISSING WINRT DEPENDENCIES
**Coding Agent Prompt:**
"Windows Mobile Hotspot implementation uses C++/WinRT but the build system lacks proper Windows SDK integration. Based on research, implement proper WinRT build configuration and capability detection."

**Research-Based Windows Integration:**

1. **CMake Windows SDK Configuration:**
   ```cmake
   if(WIN32)
       # Require Windows 10 SDK version 1903 or later for C++/WinRT
       set(CMAKE_SYSTEM_VERSION "10.0.18362.0")
       
       # Find Windows SDK
       find_package(WindowsSDK REQUIRED)
       
       # C++/WinRT configuration
       find_program(CPPWINRT_EXE cppwinrt)
       if(NOT CPPWINRT_EXE)
           message(FATAL_ERROR "C++/WinRT tool not found. Install Windows SDK.")
       endif()
       
       # Add WinRT libraries
       target_link_libraries(sudachi_multiplayer_model_b_windows PRIVATE
           windowsapp  # WinRT core
           ws2_32      # Winsock
           iphlpapi    # IP Helper API
       )
       
       # Add Windows-specific compile definitions
       target_compile_definitions(sudachi_multiplayer_model_b_windows PRIVATE
           WINRT_LEAN_AND_MEAN
           WIN32_LEAN_AND_MEAN
           NOMINMAX
       )
   endif()
   ```

2. **Capability Detection:**
   ```cpp
   class WindowsCapabilityDetector {
   public:
       static bool IsWindowsVersionSupported() {
           OSVERSIONINFOEX osvi = {};
           osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
           osvi.dwMajorVersion = 10;
           osvi.dwMinorVersion = 0;
           osvi.dwBuildNumber = 18362;  // Version 1903
           
           DWORDLONG dwlConditionMask = 0;
           VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
           VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);  
           VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
           
           return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, dwlConditionMask);
       }
       
       static bool IsWinRTAvailable() {
           try {
               winrt::init_apartment();
               return true;
           } catch (...) {
               return false;
           }
       }
   };
   ```

3. **Loopback Adapter Workaround (Research-Based):**
   ```cpp
   class LoopbackAdapterManager {
   public:
       bool InstallLoopbackAdapter() {
           // Use pnputil to install Microsoft Loopback Adapter
           std::wstring command = L"pnputil /add-driver \"";
           command += GetLoopbackInfPath();
           command += L"\" /install";
           
           STARTUPINFOW si = {};
           PROCESS_INFORMATION pi = {};
           si.cb = sizeof(si);
           
           if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, 
                            FALSE, 0, nullptr, nullptr, &si, &pi)) {
               WaitForSingleObject(pi.hProcess, INFINITE);
               DWORD exit_code;
               GetExitCodeProcess(pi.hProcess, &exit_code);
               CloseHandle(pi.hProcess);
               CloseHandle(pi.hThread);
               return exit_code == 0;
           }
           return false;
       }
   };
   ```

**Success Criteria:**
- Windows builds include proper WinRT support
- Mobile Hotspot functionality works with loopback adapter
- Graceful fallback when Windows features unavailable

## Phase 7: Build System Integration and Deployment (NEW - CRITICAL FOR USABILITY)

### Task 7.1: Main Build System Integration
**Coding Agent Prompt:**
"The multiplayer system has fragmented build directories and is not integrated into the main Sudachi build. This prevents actual usage. Integrate all components into the main CMake build system and ensure multiplayer loads with the emulator."

**Integration Requirements:**

1. **Main CMakeLists.txt Integration:**
   ```cmake
   # Add to sudachi/CMakeLists.txt
   option(ENABLE_MULTIPLAYER "Enable multiplayer support" ON)
   
   if(ENABLE_MULTIPLAYER)
       add_subdirectory(src/core/multiplayer)
       
       # Link multiplayer to core
       target_link_libraries(sudachi_core PRIVATE
           sudachi_multiplayer
       )
       
       # Add compile definition
       target_compile_definitions(sudachi_core PRIVATE
           ENABLE_MULTIPLAYER=1
       )
   endif()
   ```

2. **Core System Integration:**
   ```cpp
   // Modify src/core/core.cpp
   #ifdef ENABLE_MULTIPLAYER
   #include "core/multiplayer/multiplayer_backend.h"
   #endif
   
   class System::Impl {
   #ifdef ENABLE_MULTIPLAYER
       std::unique_ptr<Multiplayer::MultiplayerBackend> multiplayer_backend;
   #endif
   
       void Initialize() {
   #ifdef ENABLE_MULTIPLAYER
           multiplayer_backend = Multiplayer::CreateBackend();
           multiplayer_backend->Initialize();
   #endif
       }
   };
   ```

3. **Remove Build Fragmentation:**
   ```bash
   # Remove separate build directories
   rm -rf src/core/multiplayer/model_a/tests/standalone_test/build/
   rm -rf build_test/
   rm -rf build_ui_tests/
   
   # Integrate all tests into main test suite
   # Add to sudachi/src/tests/CMakeLists.txt
   if(ENABLE_MULTIPLAYER)
       add_subdirectory(../core/multiplayer/tests)
   endif()
   ```

**Success Criteria:**
- Single unified build system
- Multiplayer functionality available in main Sudachi executable
- All tests run as part of main test suite

### Task 7.2: Dependency Resolution and Distribution
**Coding Agent Prompt:**
"Ensure all dependencies are properly resolved and the system can be built and distributed on target platforms. Based on research, cpp-libp2p has complex dependencies that need careful handling."

**Distribution Strategy:**

1. **Dependency Packaging:**
   ```cmake
   # Create multiplayer dependencies package
   set(CPACK_COMPONENT_MULTIPLAYER_DISPLAY_NAME "Multiplayer Support")
   set(CPACK_COMPONENT_MULTIPLAYER_DESCRIPTION "Internet and Ad-hoc multiplayer functionality")
   
   install(TARGETS sudachi_multiplayer
       COMPONENT multiplayer
       RUNTIME DESTINATION bin
       LIBRARY DESTINATION lib
       ARCHIVE DESTINATION lib
   )
   
   # Include cpp-libp2p runtime dependencies
   install(FILES $<TARGET_RUNTIME_DLLS:p2p::p2p_basic_host>
       COMPONENT multiplayer
       DESTINATION bin
   )
   ```

2. **Platform-Specific Packaging:**
   ```cmake
   if(WIN32)
       # Include Visual C++ redistributables
       include(InstallRequiredSystemLibraries)
       
       # Windows-specific runtime
       install(FILES 
           "${CMAKE_CURRENT_BINARY_DIR}/vcruntime140.dll"
           "${CMAKE_CURRENT_BINARY_DIR}/msvcp140.dll"
           COMPONENT multiplayer
           DESTINATION bin
       )
   endif()
   ```

3. **Automated CI/CD Integration:**
   ```yaml
   # .github/workflows/build-multiplayer.yml
   name: Build with Multiplayer
   on: [push, pull_request]
   
   jobs:
     build:
       strategy:
         matrix:
           os: [ubuntu-latest, windows-latest, macos-latest]
           
       steps:
       - uses: actions/checkout@v3
         with:
           submodules: recursive
           
       - name: Install dependencies
         run: |
           vcpkg install websocketpp openssl nlohmann-json spdlog
           
       - name: Configure CMake
         run: |
           cmake -B build -DENABLE_MULTIPLAYER=ON
           
       - name: Build
         run: |
           cmake --build build --config Release
           
       - name: Test
         run: |
           cd build && ctest --output-on-failure
   ```

**Success Criteria:**
- Clean build on all target platforms
- All dependencies properly bundled for distribution
- CI/CD validates multiplayer functionality

## Phase 8: Production Hardening and Documentation (NEW - REQUIRED FOR MAINTENANCE)

### Task 8.1: Error Recovery and Resilience
**Coding Agent Prompt:**
"Based on security research, implement comprehensive error recovery, connection resilience, and graceful degradation when components fail. The current system lacks robust error handling for production use."

**Resilience Implementation:**

1. **Connection Recovery Framework:**
   ```cpp
   class ConnectionRecoveryManager {
   private:
       std::chrono::milliseconds base_delay_{1000};
       double backoff_multiplier_{2.0};
       std::chrono::milliseconds max_delay_{30000};
       int max_retries_{5};
       
   public:
       template<typename Operation>
       bool RetryWithBackoff(Operation&& op, const std::string& operation_name) {
           for (int attempt = 0; attempt < max_retries_; ++attempt) {
               try {
                   if (op()) {
                       LOG_INFO("Operation {} succeeded on attempt {}", operation_name, attempt + 1);
                       return true;
                   }
               } catch (const std::exception& e) {
                   LOG_WARNING("Operation {} failed on attempt {}: {}", 
                             operation_name, attempt + 1, e.what());
               }
               
               if (attempt < max_retries_ - 1) {
                   auto delay = CalculateBackoffDelay(attempt);
                   std::this_thread::sleep_for(delay);
               }
           }
           
           LOG_ERROR("Operation {} failed after {} attempts", operation_name, max_retries_);
           return false;
       }
   };
   ```

2. **Circuit Breaker Pattern:**
   ```cpp
   class CircuitBreaker {
   public:
       enum class State { Closed, Open, HalfOpen };
       
       template<typename Operation>
       Result<typename std::invoke_result_t<Operation>> Execute(Operation&& op) {
           if (state_ == State::Open) {
               if (ShouldAttemptReset()) {
                   state_ = State::HalfOpen;
               } else {
                   return Error{"Circuit breaker is open"};
               }
           }
           
           try {
               auto result = op();
               OnSuccess();
               return result;
           } catch (const std::exception& e) {
               OnFailure();
               return Error{e.what()};
           }
       }
   };
   ```

3. **Graceful Degradation:**
   ```cpp
   class MultiplayerModeManager {
   public:
       MultiplayerMode SelectBestAvailableMode() {
           // Try Internet mode first
           if (TestInternetConnectivity() && cpp_libp2p_available_) {
               return MultiplayerMode::Internet;
           }
           
           // Fall back to ad-hoc
           if (TestLocalNetwork() && mdns_available_) {
               return MultiplayerMode::AdHoc;
           }
           
           // Last resort: offline mode with saved network configurations
           return MultiplayerMode::OfflineWithSaved;
       }
   };
   ```

**Success Criteria:**
- System recovers gracefully from network failures
- Users get clear feedback about connectivity issues
- Automatic fallback between multiplayer modes works reliably

### Task 8.2: Comprehensive Documentation and User Guides
**Coding Agent Prompt:**
"Create complete documentation covering installation, configuration, troubleshooting, and API reference. Include user-facing guides and developer documentation for maintainability."

**Documentation Structure:**
```
docs/
├── user-guide/
│   ├── installation.md
│   ├── getting-started.md
│   ├── troubleshooting.md
│   └── supported-games.md
├── developer-guide/
│   ├── architecture.md
│   ├── api-reference.md
│   ├── building-from-source.md
│   └── contributing.md
├── deployment/
│   ├── server-setup.md
│   ├── docker-deployment.md
│   └── monitoring.md
└── security/
    ├── security-model.md
    ├── vulnerability-reporting.md
    └── audit-log.md
```

**User Guide Content:**
1. **Installation Guide:** Step-by-step installation with dependency resolution
2. **Configuration Guide:** Network settings, firewall configuration, port forwarding
3. **Troubleshooting:** Common issues with solutions and diagnostic tools
4. **Game Compatibility:** Tested games list with known issues

**Developer Documentation:**
1. **Architecture Overview:** Component relationships and data flow
2. **API Reference:** Complete function/class documentation
3. **Build Instructions:** Platform-specific build requirements
4. **Testing Guide:** How to run tests and add new test cases

**Success Criteria:**
- Users can set up multiplayer without technical expertise
- Developers can contribute without extensive ramp-up time
- Support team can resolve issues using documentation

## Updated Completion Checklist for True 100% Completion

### Phase 6: Critical Integration and Security
- [ ] **HLE Service Integration:** Legacy LDN service replaced with new implementation
- [ ] **cpp-libp2p Build:** P2P networking actually works (not mocks)
- [ ] **Network Security:** Input validation, rate limiting, DDoS protection implemented
- [ ] **Windows Platform:** WinRT dependencies and capabilities properly integrated

### Phase 7: Build System Integration
- [ ] **Main Build Integration:** Multiplayer integrated into main Sudachi build
- [ ] **Dependency Resolution:** All dependencies properly packaged for distribution
- [ ] **CI/CD Pipeline:** Automated testing validates multiplayer functionality

### Phase 8: Production Hardening
- [ ] **Error Recovery:** Connection resilience and graceful degradation implemented
- [ ] **Documentation:** Complete user and developer guides created
- [ ] **Security Audit:** External security review passed
- [ ] **Performance Validation:** All performance targets met in production environment

### Critical Success Criteria for 100% Completion
1. **✅ Functional:** Games can actually use multiplayer (not just tests pass)
2. **✅ Secure:** Production-ready security with no critical vulnerabilities
3. **✅ Integrated:** Single build system, no fragmented components
4. **✅ Documented:** Users and developers can use/maintain the system
5. **✅ Resilient:** System recovers gracefully from failures
6. **✅ Performant:** Meets all latency and throughput requirements

## Estimated Additional Effort for True Completion

### Phase 6: ~4-6 weeks
- HLE Integration: 2 weeks (complex bridge between systems)
- cpp-libp2p Integration: 1-2 weeks (dependency resolution challenges)
- Security Implementation: 1-2 weeks (comprehensive validation framework)
- Windows Platform Fix: 1 week (WinRT build configuration)

### Phase 7: ~2-3 weeks  
- Build Integration: 1 week (CMake restructuring)
- Dependency Packaging: 1 week (platform-specific packaging)
- CI/CD Setup: 1 week (automated validation)

### Phase 8: ~3-4 weeks
- Error Recovery: 1-2 weeks (resilience patterns implementation)
- Documentation: 1-2 weeks (comprehensive guides)
- Security Audit: 1 week (external review and fixes)

**Total Additional Effort: 9-13 weeks for true 100% completion**

The current implementation represents approximately 60-70% completion. The additional phases address the critical gaps that prevent real-world usage.