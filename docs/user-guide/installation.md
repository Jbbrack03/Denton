# Sudachi Multiplayer Installation Guide

## System Requirements

### Minimum Requirements
- **Operating System**: Windows 10 (1903+), Android 8.0 (API 26+), Linux (Ubuntu 20.04+), macOS 11.0+
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 100MB additional space for multiplayer components
- **Network**: Internet connection for Model A multiplayer

### Platform-Specific Requirements

#### Windows
- **Windows SDK**: Version 10.0.19041.0 or later (for WinRT APIs)
- **Visual C++ Redistributable**: 2019 or later
- **Windows Features**: Wi-Fi Direct capability (for offline multiplayer)
- **Firewall**: Ports 7777-7787 (configurable) must be accessible

#### Android
- **API Level**: 26 (Android 8.0) minimum, 33+ recommended
- **Permissions**: Location, Wi-Fi state changes, nearby Wi-Fi devices
- **Hardware**: Wi-Fi Direct support required for offline multiplayer
- **Network**: Mobile data or Wi-Fi connection

#### Linux
- **Packages**: `libboost-all-dev`, `libssl-dev`, `libspdlog-dev`
- **Network Tools**: `hostapd`, `wpa_supplicant` (for offline multiplayer)
- **Kernel**: 4.4+ with nl80211 support

#### macOS
- **SDK**: macOS 11.0 SDK or later
- **Frameworks**: Network.framework, SystemConfiguration.framework
- **Network**: Wi-Fi adapter with AP mode support (limited)

## Installation Methods

### Method 1: Pre-built Releases (Recommended)

1. **Download the latest release**:
   - Visit [Sudachi Releases](https://github.com/sudachi-emulator/sudachi/releases)
   - Download the appropriate build for your platform
   - Multiplayer is included by default in all releases

2. **Install platform dependencies**:

   **Windows**:
   ```cmd
   # Run as Administrator if Windows SDK is missing
   winget install Microsoft.WindowsSDK.10.0.19041
   ```

   **Android**:
   ```bash
   # Install via Google Play Store or sideload APK
   # Permissions will be requested on first run
   ```

   **Linux (Ubuntu/Debian)**:
   ```bash
   sudo apt update
   sudo apt install libboost-all-dev libssl-dev libspdlog-dev
   ```

   **macOS**:
   ```bash
   # Install via Homebrew
   brew install boost openssl spdlog
   ```

3. **Verify installation**:
   - Launch Sudachi
   - Navigate to Settings → Multiplayer
   - Verify both "Internet Multiplayer" and "Offline Multiplayer" options are available

### Method 2: Build from Source

See [Building from Source](../developer-guide/building-from-source.md) for detailed instructions.

## Network Configuration

### Firewall Configuration

#### Windows Firewall
```cmd
# Run as Administrator
netsh advfirewall firewall add rule name="Sudachi Multiplayer" dir=in action=allow protocol=TCP localport=7777-7787
netsh advfirewall firewall add rule name="Sudachi Multiplayer UDP" dir=in action=allow protocol=UDP localport=7777-7787
```

#### Linux (UFW)
```bash
sudo ufw allow 7777:7787/tcp
sudo ufw allow 7777:7787/udp
sudo ufw reload
```

#### macOS
```bash
# Add rules via System Preferences → Security & Privacy → Firewall → Options
# Or use pfctl (advanced users)
```

### Router Configuration (Port Forwarding)

For hosting multiplayer sessions, configure your router to forward ports:

1. **Access your router's admin panel** (typically 192.168.1.1 or 192.168.0.1)
2. **Navigate to Port Forwarding** settings
3. **Add rules**:
   - **Service Name**: Sudachi Multiplayer
   - **Protocol**: TCP & UDP
   - **External Port Range**: 7777-7787
   - **Internal Port Range**: 7777-7787
   - **Internal IP**: Your computer's local IP address

### UPnP Configuration

Sudachi supports automatic port forwarding via UPnP:

1. **Enable UPnP** in Sudachi settings:
   - Settings → Multiplayer → Advanced → Enable UPnP
2. **Verify UPnP** is enabled on your router
3. **Test connection** using the built-in network test

## Configuration

### Initial Setup

1. **Launch Sudachi** and complete the initial setup wizard
2. **Configure multiplayer settings**:
   - Settings → Multiplayer
   - Choose default mode: "Internet Multiplayer" (recommended)
   - Set display name (used in multiplayer sessions)
   - Configure network preferences

### Multiplayer Modes

#### Internet Multiplayer (Model A)
- **Default mode** for most users
- **Global reach**: Play with users worldwide
- **Requirements**: Internet connection, forwarded ports (optional for clients)
- **Latency**: Typically 20-100ms depending on distance

#### Offline Multiplayer (Model B)
- **Local network**: Direct device-to-device connections
- **No internet required**: Perfect for travel or areas with poor connectivity
- **Requirements**: Wi-Fi Direct or Mobile Hotspot capability
- **Latency**: 1-10ms (local network)

### Advanced Configuration

#### Network Settings
```json
{
  "multiplayer": {
    "mode": "internet",
    "port_range": {
      "min": 7777,
      "max": 7787
    },
    "connection_timeout": 30000,
    "retry_attempts": 3,
    "enable_upnp": true,
    "relay_fallback": true
  }
}
```

#### Security Settings
```json
{
  "security": {
    "enable_encryption": true,
    "require_authentication": false,
    "rate_limiting": {
      "enabled": true,
      "max_connections_per_ip": 5,
      "max_bandwidth_mbps": 10
    }
  }
}
```

## Platform-Specific Setup

### Windows Setup

1. **Install Visual C++ Redistributable**:
   ```cmd
   # Download from Microsoft or use winget
   winget install Microsoft.VCRedist.2015+.x64
   ```

2. **Enable Wi-Fi Direct** (for offline multiplayer):
   - Settings → Network & Internet → Wi-Fi → Manage known networks
   - Ensure Wi-Fi Direct is supported and enabled

3. **Configure Windows Defender**:
   - Add Sudachi to Windows Defender exclusions
   - Allow through Windows Defender Firewall

### Android Setup

1. **Grant required permissions**:
   - Location (for Wi-Fi Direct discovery)
   - Wi-Fi state changes
   - Nearby Wi-Fi devices (Android 13+)

2. **Optimize battery settings**:
   - Disable battery optimization for Sudachi
   - Allow background activity

3. **Configure mobile data** (optional):
   - Enable mobile data usage for Sudachi
   - Set data usage limits if needed

### Linux Setup

1. **Install system dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt install build-essential cmake libboost-all-dev libssl-dev libspdlog-dev

   # Fedora/RHEL
   sudo dnf install gcc-c++ cmake boost-devel openssl-devel spdlog-devel

   # Arch Linux
   sudo pacman -S base-devel cmake boost openssl spdlog
   ```

2. **Configure networking** (for offline multiplayer):
   ```bash
   # Install network tools
   sudo apt install hostapd wpa-supplicant

   # Add user to netdev group
   sudo usermod -a -G netdev $USER
   ```

### macOS Setup

1. **Install dependencies**:
   ```bash
   # Install Homebrew if not present
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

   # Install required libraries
   brew install boost openssl spdlog cmake ninja
   ```

2. **Configure security**:
   - System Preferences → Security & Privacy → Privacy
   - Allow Sudachi access to network connections
   - Grant location access if using Wi-Fi features

## Verification

### Connection Test

1. **Launch Sudachi**
2. **Navigate to Settings → Multiplayer**
3. **Click "Test Network Connection"**
4. **Verify all tests pass**:
   - Internet connectivity
   - Port accessibility
   - UPnP functionality (if enabled)
   - P2P capability

### Multiplayer Test

1. **Start a supported game** (see [Supported Games](supported-games.md))
2. **Access local multiplayer** from the game menu
3. **Verify session creation**:
   - Internet mode: Should appear in global room list
   - Offline mode: Should be discoverable locally
4. **Test with second device** or have a friend join

## Troubleshooting

### Common Issues

**"Network connection failed"**
- Check internet connectivity
- Verify firewall settings
- Test with UPnP disabled

**"No sessions found"**
- Ensure game versions match
- Check region compatibility
- Verify NAT type (use network test)

**"Connection timeout"**
- Increase timeout values in settings
- Check for NAT/firewall blocking
- Try relay fallback mode

**Platform-specific issues**
- See [Troubleshooting Guide](troubleshooting.md) for detailed solutions

## Support

### Community Support
- **Discord**: [Sudachi Community](https://discord.gg/sudachi)
- **Reddit**: [r/SudachiEmulator](https://reddit.com/r/SudachiEmulator)
- **Forums**: [Official Forums](https://community.sudachi-emulator.org)

### Technical Support
- **Issues**: [GitHub Issues](https://github.com/sudachi-emulator/sudachi/issues)
- **Documentation**: [Developer Guide](../developer-guide/README.md)
- **FAQ**: [Frequently Asked Questions](troubleshooting.md#faq)

## Next Steps

After installation:
1. **Read the Getting Started guide**: [Getting Started](getting-started.md)
2. **Check supported games**: [Supported Games](supported-games.md)
3. **Configure advanced settings** as needed
4. **Join the community** for tips and support

---

*Last updated: August 4, 2025*
*For technical support, see [Troubleshooting](troubleshooting.md)*
