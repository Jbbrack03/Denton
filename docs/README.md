# Sudachi Multiplayer Documentation

Welcome to the comprehensive documentation for Sudachi Multiplayer - the dual-mode multiplayer system for the Sudachi Nintendo Switch emulator.

## Documentation Structure

### 📚 User Guide
For end users setting up and using multiplayer functionality:

- **[Installation Guide](user-guide/installation.md)** - Complete setup instructions for all platforms
- **[Getting Started](user-guide/getting-started.md)** - Your first multiplayer session walkthrough  
- **[Troubleshooting](user-guide/troubleshooting.md)** - Common issues and solutions
- **[Supported Games](user-guide/supported-games.md)** - Compatible games and requirements

### 🔧 Developer Guide  
For developers contributing to or integrating with the multiplayer system:

- **[Architecture Overview](developer-guide/architecture.md)** - Technical architecture and component design
- **[API Reference](developer-guide/api-reference.md)** - Complete API documentation
- **[Building from Source](developer-guide/building-from-source.md)** - Development environment setup
- **[Contributing Guide](developer-guide/contributing.md)** - How to contribute to the project

### 🚀 Deployment
For system administrators deploying multiplayer infrastructure:

- **[Server Setup](deployment/server-setup.md)** - Room and relay server deployment
- **[Docker Deployment](deployment/docker-deployment.md)** - Containerized deployment guide
- **[Monitoring](deployment/monitoring.md)** - System monitoring and maintenance

### 🔒 Security
Security model, best practices, and vulnerability management:

- **[Security Model](security/security-model.md)** - Comprehensive security architecture
- **[Vulnerability Reporting](security/vulnerability-reporting.md)** - How to report security issues
- **[Audit Log](security/audit-log.md)** - Security assessment history

## Quick Start

### For Users
1. **Install**: Follow the [Installation Guide](user-guide/installation.md) for your platform
2. **Configure**: Set up your network according to the platform-specific instructions
3. **Play**: Check out [Getting Started](user-guide/getting-started.md) for your first session
4. **Troubleshoot**: If you encounter issues, see [Troubleshooting](user-guide/troubleshooting.md)

### For Developers
1. **Understand**: Read the [Architecture Overview](developer-guide/architecture.md)
2. **Build**: Follow [Building from Source](developer-guide/building-from-source.md)
3. **Develop**: Use the [API Reference](developer-guide/api-reference.md) for integration
4. **Contribute**: See [Contributing Guide](developer-guide/contributing.md) for guidelines

## System Overview

Sudachi Multiplayer implements a dual-mode architecture providing:

### 🌐 Internet Multiplayer (Model A)
- **Global connectivity** through centralized room discovery
- **P2P connections** with relay fallback for optimal performance
- **NAT traversal** using UPnP and STUN/TURN protocols
- **Cross-platform** support for Windows, Linux, macOS, Android

### 📡 Offline Multiplayer (Model B)  
- **Local device-to-device** networking without internet
- **Wi-Fi Direct** on Android, Mobile Hotspot on Windows
- **mDNS discovery** for automatic peer detection
- **Ultra-low latency** for competitive gaming

### 🎮 Nintendo Switch Game Compatibility
- **Native nn::ldn** HLE service emulation
- **Zero game modification** required
- **Transparent operation** - games work as if on real hardware
- **Comprehensive testing** with popular multiplayer titles

## Platform Support

| Platform | Internet Multiplayer | Offline Multiplayer | Status |
|----------|---------------------|--------------------| -------|
| **Windows 10+** | ✅ Full Support | ✅ Mobile Hotspot | Stable |
| **Android 8.0+** | ✅ Full Support | ✅ Wi-Fi Direct | Stable |
| **Linux** | ✅ Full Support | 🚧 Limited | Beta |
| **macOS 11+** | ✅ Full Support | 🚧 Limited | Beta |

## Key Features

### 🔒 Security First
- **End-to-end encryption** using TLS 1.3 and ChaCha20-Poly1305
- **Rate limiting** and DDoS protection
- **Input validation** and packet integrity checks
- **Privacy controls** with minimal data collection

### ⚡ Performance Optimized
- **< 5ms latency** for P2P connections
- **< 20ms latency** for relay connections  
- **Automatic quality adaptation** based on network conditions
- **Bandwidth optimization** for mobile users

### 🛠 Developer Friendly
- **Comprehensive test suite** with 300+ test cases
- **Mock interfaces** for dependency injection testing
- **Docker test environment** for integration testing
- **Continuous integration** with automated testing

## Getting Help

### Community Support
- **Discord**: [Sudachi Community Server](https://discord.gg/sudachi) (#multiplayer-help)
- **Reddit**: [r/SudachiEmulator](https://reddit.com/r/SudachiEmulator)
- **Forums**: [Official Community Forums](https://community.sudachi-emulator.org)

### Technical Support
- **GitHub Issues**: [Report bugs and request features](https://github.com/sudachi-emulator/sudachi/issues)
- **Documentation**: Comprehensive guides in this repository
- **Developer Chat**: Real-time developer support on Discord

### Security Issues
For security vulnerabilities, please follow our [Vulnerability Reporting](security/vulnerability-reporting.md) process. **Do not** report security issues in public GitHub issues.

## Contributing

We welcome contributions from the community! See our [Contributing Guide](developer-guide/contributing.md) for:

- Code contribution guidelines
- Development environment setup
- Testing requirements
- Code review process
- Community standards

## License

This project is licensed under the GPL-3.0-or-later license. See the [LICENSE](../LICENSE) file for details.

The multiplayer system is designed as an integral part of Sudachi and follows the same licensing terms.

## Acknowledgments

### Special Thanks
- **Nintendo** for the original LDN (Local Discovery and Networking) API design
- **cpp-libp2p contributors** for the P2P networking library
- **mjansson** for the mDNS library
- **Sudachi development team** for the emulator foundation
- **Community contributors** for testing and feedback

### Third-Party Libraries
- [cpp-libp2p](https://github.com/libp2p/cpp-libp2p) - P2P networking
- [mjansson/mdns](https://github.com/mjansson/mdns) - mDNS service discovery
- [websocketpp](https://github.com/zaphoyd/websocketpp) - WebSocket client
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [spdlog](https://github.com/gabime/spdlog) - Logging framework

---

## Documentation Status

| Section | Status | Last Updated |
|---------|--------|--------------|
| User Guide | ✅ Complete | 2025-08-04 |
| Developer Guide | ✅ Complete | 2025-08-04 |
| Deployment | 📝 In Progress | 2025-08-04 |
| Security | ✅ Complete | 2025-08-04 |

*Documentation is continuously updated with each release. Please check the individual file timestamps for the most recent updates.*

---

**Need immediate help?** Join our [Discord community](https://discord.gg/sudachi) for real-time support from developers and experienced users.
