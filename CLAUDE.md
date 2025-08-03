# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the Sudachi Multiplayer project, implementing a comprehensive dual-mode multiplayer subsystem for the Sudachi Nintendo Switch emulator. The project enables both internet-based and offline ad-hoc multiplayer capabilities across Windows and Android platforms.

### Core Architecture

The multiplayer implementation follows a dual-track development strategy:

1. **Model A (Internet Multiplayer - LDN-over-IP)**: Global play by tunneling local wireless traffic over internet connections
   - Central Room Server (WebSocket-based discovery)
   - P2P connections using cpp-libp2p
   - Proxy fallback for failed P2P connections

2. **Model B (Offline Ad-Hoc Multiplayer)**: Platform-specific device-to-device wireless networking
   - mDNS/DNS-SD discovery using mjansson/mdns
   - Android: Wi-Fi Direct via JNI
   - Windows: Mobile Hotspot via C++/WinRT

## Development Commands

### Building (Linux)
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DSUDACHI_USE_BUNDLED_VCPKG=ON -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=x86-64-v2" \
    -DSUDACHI_USE_BUNDLED_FFMPEG=ON \
    -DSUDACHI_ENABLE_LTO=ON \
    -DSUDACHI_ROOM=OFF \
    -GNinja

# Build
cmake --build . --config Release
```

### Building (Windows)
```bash
# Use Visual Studio or vcpkg for dependency management
# Configure with CMake GUI or command line
cmake -B build -S . -DSUDACHI_USE_BUNDLED_VCPKG=ON
cmake --build build --config Release
```

### Testing
```bash
# Run unit tests (from build directory)
ctest --output-on-failure

# Run specific test category
ctest -R "network" --output-on-failure
```

### Android Development
```bash
# From src/android directory
./gradlew build
./gradlew assembleDebug
./gradlew installDebug
```

## Project Structure

### Key Directories

- **`src/core/hle/service/ldn/`**: LDN (Local Discovery and Networking) HLE implementation
  - This is where the nn::ldn service emulation lives
  - Currently contains legacy implementation that needs replacement

- **`src/core/multiplayer/`**: NEW multiplayer implementation directory
  - `common/`: Shared components (packet protocol, error codes, config)
  - `model_a/`: Internet multiplayer implementation
  - `model_b/`: Offline ad-hoc multiplayer with platform subdirectories

- **`externals/`**: Third-party dependencies
  - Add cpp-libp2p and mjansson/mdns here during implementation

### Removed Directories (Phase 0 Completed)
- ~~`src/network/`~~: Legacy network infrastructure (REMOVED)
- ~~`src/sudachi/multiplayer/`~~: Qt UI for multiplayer (REMOVED)

### Important Files

- **`MULTIPLAYER_PROJECT_OVERVIEW.md`**: Technical architecture and implementation details
- **`Sudachi Multiplayer PRD.md`**: Complete product requirements and specifications
- **`SERVER_ARCHITECTURE.md`**: Room server and relay server specifications
- **`LEGACY_CODE_REMOVAL_PLAN.md`**: Plan for removing existing multiplayer code
- **`IMPLEMENTATION PLAN.md`**: Phased development approach

## Key Implementation Details

### HLE Module Interface
The core nn::ldn HLE interface must implement:
- `Initialize()`: Initialize the multiplayer backend
- `CreateNetwork()`: Host a game session
- `Scan()`: Discover available sessions
- `Connect()`: Join a session
- `SendPacket()`/`ReceivePacket()`: Data transmission

### Network Protocol
Custom binary protocol with header structure:
```cpp
struct LdnPacketHeader {
    uint16_t magic = 0x4C44;  // "LD"
    uint16_t version = 1;
    uint32_t session_id;
    uint16_t packet_type;
    uint16_t payload_size;
    // ... additional fields
};
```

### Platform-Specific Considerations

#### Android (JNI)
- Requires Wi-Fi Direct permissions in AndroidManifest.xml
- JNI wrapper for WifiP2pManager
- Runtime permission handling

#### Windows (C++/WinRT)
- Mobile Hotspot control via NetworkOperatorTetheringManager
- May require elevated privileges
- Package.appxmanifest capabilities configuration

## Development Guidelines

### Code Standards
- Follow existing Sudachi coding conventions
- Use modern C++17 features
- Maintain compatibility with existing emulator architecture
- Write comprehensive unit tests for all new components

### Testing Requirements
- Unit tests using Google Test framework
- Integration tests with multiple emulator instances
- Network condition simulation (packet loss, latency)
- Game-specific compatibility testing

### Dependencies to Add
| Library | Purpose | Integration Method |
|---------|---------|-------------------|
| cpp-libp2p | P2P networking | FetchContent (not in vcpkg) |
| mjansson/mdns | mDNS discovery | Header-only download |
| websocketpp | WebSocket client | vcpkg (already added) |
| openssl | SSL/TLS support | vcpkg (already added) |
| spdlog | Logging | vcpkg (already added) |

## Current Development Phase

The project has completed Phase 0 and is ready for Phase 1 implementation.

### Completed Tasks (Phase 0):
- ✅ **Legacy Code Removal**: Removed all room-based multiplayer implementation
- ✅ **Package Manager Configuration**: Updated vcpkg.json with multiplayer dependencies
- ✅ **Directory Structure**: Created new multiplayer directory structure with CMake integration
- ✅ **Initial Stubs**: Created error codes and packet protocol header files

### Next Priority Tasks (Phase 1):
1. **HLE Interface**: Implement new nn::ldn HLE module with dual-backend support
2. **Configuration System**: Create JSON-based configuration management
3. **Packet Protocol**: Complete the binary packet protocol implementation

### Future Phases:
- **Model A Implementation**: Room server integration and P2P networking
- **Model B Implementation**: Platform-specific ad-hoc networking
- **Testing Infrastructure**: Comprehensive test suite development

## Common Issues and Solutions

### CMake Configuration
- Ensure all dependencies are available via vcpkg or manually installed
- Use bundled vcpkg option for Windows builds
- Disable SUDACHI_ROOM option as it's incompatible with new implementation

### Network Testing
- Use Docker containers for isolated network testing
- Simulate various network conditions with tc/netem on Linux
- Test NAT traversal with different router configurations

### Platform-Specific Debugging
- Android: Use adb logcat for JNI debugging
- Windows: Enable ETW tracing for network diagnostics
- Use Wireshark for packet-level debugging

## Development Log

### 2025-08-03: Phase 0 Completion
- Removed legacy multiplayer code from src/network/ and src/sudachi/multiplayer/
- Cleaned up all references in CMake files, main.cpp, main.h, and main.ui
- Updated vcpkg.json with websocketpp, openssl, and spdlog dependencies
- Created new multiplayer directory structure under src/core/multiplayer/
- Added initial stub files for error codes and packet protocol
- Ready to begin Phase 1 implementation