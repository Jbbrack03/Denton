# Changelog

All notable changes to the Sudachi Multiplayer project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-08-03

### Added
- Initial project structure for dual-mode multiplayer subsystem
- nn::ldn HLE interface implementation with dual-backend support
  - Full IPC command support (Initialize, CreateNetwork, Scan, Connect, etc.)
  - Proper state machine with validation
  - Mock backend support for testing
  - Comprehensive test suite (26+ test cases)
- JSON-based configuration management system
  - Platform-specific path handling (Windows, Linux, macOS, Android)
  - Configuration validation and migration support
  - Cross-platform import/export capabilities
  - 292 comprehensive test cases
- Binary packet protocol implementation
  - IEEE 802.3 CRC32 with lookup table
  - Little-endian serialization for cross-platform compatibility
  - Performance optimized (>100MB/s CRC32, <100ms for 32KB packets)
  - Comprehensive error handling and validation
- Test infrastructure using Catch2 framework
- Error handling framework with comprehensive error codes
- CMake build system integration

### Changed
- Updated vcpkg.json with multiplayer dependencies (websocketpp, openssl, spdlog)

### Removed
- Legacy room-based multiplayer implementation from src/network/
- Qt multiplayer UI components from src/sudachi/multiplayer/
- Common announce_multiplayer_room header

### Security
- Implemented packet integrity verification using CRC32 checksums
- Added configuration validation to prevent invalid settings

## [0.2.0] - 2025-08-03

### Added
- WebSocket Room Client implementation for Model A (Internet multiplayer)
  - 49+ comprehensive test cases across 4 test files
  - Thread-safe message handling with queue system
  - Automatic reconnection with exponential backoff
  - JSON message serialization/deserialization for all message types
  - Support for all room server protocol messages
- P2P Network module with cpp-libp2p integration design
  - 68+ test scenarios for comprehensive coverage
  - Mock-based P2PNetwork implementation for testing
  - Support for TCP and WebSocket transports
  - NAT traversal detection and relay fallback logic
  - Thread-safe message handling
- Relay Client implementation for proxy server fallback
  - RelayProtocol with 12-byte header (PRD Section 3.4 compliant)
  - Session management with JWT authentication support
  - Bandwidth limiting (10 Mbps per session) with token bucket algorithm
  - P2P to relay fallback mechanism
  - Thread-safe operations with proper synchronization
- Comprehensive test infrastructure with 100+ tests total
- Mock interfaces for all network components

### Changed
- Refactored WebSocket Room Client to use proper JSON parsing (security improvement)
- Extracted ExponentialBackoff utility class for reusability
- Updated CMakeLists.txt files to include all new components

### Security
- Replaced manual string parsing with secure JSON library (nlohmann/json)
- Added input validation for all network messages
- Implemented bandwidth limiting to prevent resource exhaustion

## [Unreleased]
### TODO
- mDNS Discovery implementation for Model B (Ad-hoc multiplayer)
- Android Wi-Fi Direct support via JNI
- Windows Mobile Hotspot support via C++/WinRT
- UI components for multiplayer settings
- Integration testing with actual games
- Performance benchmarking