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

## [Unreleased]
### TODO
- WebSocket Room Client for Model A (Internet multiplayer)
- cpp-libp2p integration for P2P networking
- mDNS Discovery implementation for Model B (Ad-hoc multiplayer)
- Android Wi-Fi Direct support via JNI
- Windows Mobile Hotspot support via C++/WinRT