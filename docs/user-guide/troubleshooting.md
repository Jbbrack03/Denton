# Sudachi Multiplayer Troubleshooting Guide

## Quick Diagnostic Tools

### Built-in Network Test
Before troubleshooting complex issues, use Sudachi's built-in diagnostic tools:

1. **Open Sudachi** → Settings → Multiplayer → "Test Network Connection"
2. **Review test results**:
   - Internet connectivity ✓/✗
   - Port accessibility ✓/✗
   - UPnP functionality ✓/✗
   - P2P capability ✓/✗
   - Relay server connectivity ✓/✗

### Log Collection
To help diagnose issues, collect Sudachi logs:

**Windows**:
```cmd
# Navigate to log directory
cd %APPDATA%\Sudachi\logs\
# Find latest log file
dir *.log /o-d
```

**Linux/macOS**:
```bash
# Navigate to log directory
cd ~/.local/share/sudachi/logs/
# Find latest log file
ls -lt *.log
```

**Android**:
```bash
# Use ADB to collect logs
adb logcat -s Sudachi:D
```

## Common Connection Issues

### "Unable to Connect to Internet"

**Symptoms**: Cannot establish internet multiplayer sessions, network test fails
**Possible Causes**: Network connectivity, DNS issues, proxy settings

**Solutions**:
1. **Check basic connectivity**:
   ```bash
   # Test internet connection
   ping google.com
   ping 8.8.8.8
   ```

2. **Verify DNS resolution**:
   ```bash
   # Test DNS resolution
   nslookup sudachi.org
   nslookup room.sudachi.org
   ```

3. **Check proxy settings**:
   - Windows: Settings → Network & Internet → Proxy
   - macOS: System Preferences → Network → Advanced → Proxies
   - Linux: Check `http_proxy` and `https_proxy` environment variables

4. **Reset network stack** (Windows):
   ```cmd
   # Run as Administrator
   netsh winsock reset
   netsh int ip reset
   ipconfig /flushdns
   # Restart computer
   ```

### "Connection Timeout" Errors

**Symptoms**: Sessions found but cannot connect, timeout after 30 seconds
**Possible Causes**: Firewall blocking, NAT issues, server overload

**Solutions**:
1. **Configure firewall**:

   **Windows Firewall**:
   ```cmd
   # Run as Administrator
   netsh advfirewall firewall add rule name="Sudachi Multiplayer" dir=in action=allow protocol=TCP localport=7777-7787
   netsh advfirewall firewall add rule name="Sudachi Multiplayer OUT" dir=out action=allow protocol=TCP localport=7777-7787
   ```

   **Linux (UFW)**:
   ```bash
   sudo ufw allow 7777:7787/tcp
   sudo ufw allow 7777:7787/udp
   ```

   **macOS**:
   - System Preferences → Security & Privacy → Firewall → Options
   - Click "+" and add Sudachi application
   - Select "Allow incoming connections"

2. **Check NAT type**:
   - Use network test to determine NAT type
   - Types: Open < Moderate < Strict < Blocked
   - Strict/Blocked NAT may require port forwarding

3. **Configure router port forwarding**:
   - Access router admin panel (usually 192.168.1.1)
   - Forward ports 7777-7787 TCP/UDP to your device
   - Enable UPnP if available

### "No Sessions Found"

**Symptoms**: Session list appears empty, cannot see hosted games
**Possible Causes**: Region mismatch, game version incompatibility, server issues

**Solutions**:
1. **Verify game compatibility**:
   - Ensure both players have the same game version
   - Check [Supported Games List](supported-games.md)
   - Verify game update status

2. **Check region settings**:
   - Settings → Multiplayer → Advanced → Region
   - Try "Global" instead of specific region
   - Restart multiplayer service after changes

3. **Clear session cache**:
   ```bash
   # Delete cache files (Windows)
   del "%APPDATA%\Sudachi\cache\multiplayer\*"
   
   # Linux/macOS
   rm -rf ~/.cache/sudachi/multiplayer/*
   ```

4. **Verify server status**:
   - Check [Status Page](https://status.sudachi.org)
   - Try different server regions
   - Wait for server maintenance to complete

## Performance Issues

### High Latency (Lag)

**Symptoms**: Delayed responses, choppy gameplay, > 100ms latency
**Possible Causes**: Network congestion, routing issues, hardware limitations

**Solutions**:
1. **Optimize network connection**:
   - Use wired ethernet instead of Wi-Fi
   - Close bandwidth-heavy applications
   - Enable QoS on router for gaming traffic
   - Switch to 5GHz Wi-Fi if using wireless

2. **Check for interference**:
   ```bash
   # Windows: Check Wi-Fi interference
   netsh wlan show profiles
   netsh wlan show profile "YourNetwork" key=clear
   
   # Linux: Scan for Wi-Fi networks
   iwlist scan | grep ESSID
   
   # macOS: Option+Click Wi-Fi icon for details
   ```

3. **Adjust Sudachi settings**:
   - Settings → Multiplayer → Advanced → Connection Priority → "Stability"
   - Reduce "Max Bandwidth" if on limited connection
   - Enable "Relay Fallback" for problematic connections

4. **Change server region**:
   - Test different regions in network test
   - Choose region with lowest latency
   - Consider using relay servers for consistent routing

### Packet Loss / Connection Drops

**Symptoms**: Intermittent disconnections, "Connection Lost" messages
**Possible Causes**: Unstable internet, Wi-Fi interference, overheating

**Solutions**:
1. **Monitor connection stability**:
   ```bash
   # Continuous ping test
   ping -t google.com  # Windows
   ping google.com     # Linux/macOS
   ```

2. **Check hardware**:
   - Ensure router/modem are properly ventilated
   - Restart network equipment
   - Update network drivers
   - Check cable connections

3. **Adjust connection settings**:
   - Increase connection timeout: Settings → Advanced → Timeout → 60 seconds
   - Enable automatic reconnection
   - Use relay servers for stability

### Poor Audio/Video Quality

**Symptoms**: Choppy audio, frame drops, sync issues
**Possible Causes**: Insufficient bandwidth, CPU limitations, encoding settings

**Solutions**:
1. **Check system resources**:
   - Monitor CPU/RAM usage during gameplay
   - Close unnecessary applications
   - Lower Sudachi graphics settings
   - Enable frame limiting

2. **Optimize bandwidth usage**:
   - Settings → Multiplayer → Bandwidth Limit → "Conservative"
   - Disable streaming/downloads during gameplay
   - Use wired connection for host

## Platform-Specific Issues

### Windows Issues

#### "Windows SDK not found" Error
**Cause**: Missing Windows SDK required for WinRT APIs
**Solution**:
```cmd
# Install Windows SDK
winget install Microsoft.WindowsSDK.10.0.19041
# Or download from Microsoft Developer site
```

#### Wi-Fi Direct Not Available
**Cause**: Hardware doesn't support Wi-Fi Direct or drivers outdated
**Solutions**:
1. **Check Wi-Fi Direct support**:
   ```cmd
   # Check adapter capabilities
   netsh wlan show drivers
   # Look for "Hosted network supported: Yes"
   ```

2. **Update Wi-Fi drivers**:
   - Device Manager → Network adapters
   - Right-click Wi-Fi adapter → Update driver
   - Or download from manufacturer website

#### "Access Denied" Mobile Hotspot Error
**Cause**: Insufficient permissions for Mobile Hotspot control
**Solutions**:
1. **Run as Administrator** (temporary)
2. **Enable Developer Mode**:
   - Settings → Update & Security → For developers → Developer mode
3. **Check Group Policy** (Enterprise environments)

### Android Issues

#### "Location Permission Required"
**Cause**: Android requires location permission for Wi-Fi scanning
**Solution**:
1. **Grant location permission**:
   - Settings → Apps → Sudachi → Permissions → Location → Allow
2. **Enable "Precise Location"** (Android 12+)
3. **Disable battery optimization**:
   - Settings → Battery → Battery optimization → Sudachi → Don't optimize

#### Wi-Fi Direct Connection Fails
**Cause**: Android Wi-Fi Direct limitations, interference
**Solutions**:
1. **Clear Wi-Fi Direct data**:
   ```bash
   # ADB commands (developer mode required)
   adb shell pm clear com.android.server.wifi
   adb shell reboot
   ```

2. **Reset network settings**:
   - Settings → System → Reset → Reset network settings
   - Re-configure Wi-Fi after reset

3. **Check device compatibility**:
   - Verify Wi-Fi Direct support in device specifications
   - Some budget devices have limited P2P support

#### "Nearby devices permission denied" (Android 13+)
**Cause**: New permission requirement for nearby device discovery
**Solution**:
- Settings → Apps → Sudachi → Permissions → Nearby devices → Allow

### Linux Issues

#### "Permission denied" Network Operations
**Cause**: Insufficient privileges for network operations
**Solutions**:
1. **Add user to netdev group**:
   ```bash
   sudo usermod -a -G netdev $USER
   # Log out and back in
   ```

2. **Configure sudo for network tools**:
   ```bash
   # Edit sudoers file
   sudo visudo
   # Add line:
   %netdev ALL=(ALL) NOPASSWD: /usr/sbin/hostapd, /usr/sbin/wpa_supplicant
   ```

#### Missing Network Tools
**Cause**: Required network utilities not installed
**Solution**:
```bash
# Ubuntu/Debian
sudo apt install hostapd wpa-supplicant iw

# Fedora/RHEL
sudo dnf install hostapd wpa_supplicant iw

# Arch Linux
sudo pacman -S hostapd wpa_supplicant iw
```

#### "Device or resource busy" Wi-Fi Error
**Cause**: NetworkManager interfering with manual Wi-Fi control
**Solution**:
```bash
# Temporarily disable NetworkManager for specific interface
sudo nmcli device set wlan0 managed no
# Re-enable after use
sudo nmcli device set wlan0 managed yes
```

### macOS Issues

#### "Network framework not available"
**Cause**: Incompatible macOS version or SDK
**Solution**:
- Update to macOS 11.0 or later
- Rebuild Sudachi with newer SDK

#### Soft AP Creation Fails
**Cause**: macOS limitations on software access points
**Solutions**:
1. **Use Internet Sharing**:
   - System Preferences → Sharing → Internet Sharing
   - Enable manually, then start Sudachi
2. **Third-party tools**: Consider commercial solutions
3. **External hardware**: USB Wi-Fi adapter with AP mode

## Error Code Reference

### Connection Errors (1000-1999)
- **1001**: Connection timeout - Check firewall/NAT settings
- **1002**: Connection refused - Verify target device is hosting
- **1003**: Authentication failed - Check passwords/credentials
- **1004**: Already connected - Disconnect before joining new session
- **1005**: Not connected - Establish connection before sending data

### Session Errors (2000-2999)
- **2001**: Room not found - Verify session ID or refresh list
- **2002**: Room full - Wait for slot to open or try different session
- **2003**: Password required - Enter correct session password
- **2004**: Invalid password - Check password spelling/case
- **2005**: Already in room - Leave current session first

### Network Errors (3000-3999)
- **3001**: Network unreachable - Check internet connectivity
- **3002**: Host unreachable - Verify target device network
- **3003**: DNS resolution failed - Check DNS settings
- **3004**: SSL/TLS error - Verify system time/certificates
- **3005**: Proxy error - Check proxy configuration

### Platform Errors (4000-4999)
- **4001**: Platform API error - Check system permissions
- **4002**: Feature unavailable - Hardware doesn't support feature
- **4003**: Permission denied - Grant required permissions
- **4004**: Resource exhausted - Close other applications

## Advanced Troubleshooting

### Network Packet Analysis

For advanced users, packet analysis can help diagnose complex issues:

**Windows (using Wireshark)**:
1. Install Wireshark
2. Capture on active network interface
3. Filter for Sudachi traffic: `tcp.port >= 7777 and tcp.port <= 7787`
4. Look for connection patterns, retransmissions, errors

**Linux (using tcpdump)**:
```bash
# Capture Sudachi traffic
sudo tcpdump -i any -n port 7777-7787
# Save to file for analysis
sudo tcpdump -i any -n port 7777-7787 -w sudachi-capture.pcap
```

### Performance Profiling

**Monitor system resources**:
```bash
# Windows
perfmon.exe
# Monitor: Processor Time, Network Utilization, Memory Usage

# Linux
htop
iotop
nethogs

# macOS
Activity Monitor
# Network tab for bandwidth usage
```

### Database Debugging

**Clear corrupted session data**:
```sql
-- SQLite database location varies by platform
-- Windows: %APPDATA%\Sudachi\multiplayer.db
-- Linux: ~/.local/share/sudachi/multiplayer.db

-- Clear all sessions
DELETE FROM multiplayer_sessions;

-- Reset configuration
DELETE FROM multiplayer_config WHERE key LIKE 'session_%';
```

## Getting Help

### Before Requesting Support

1. **Run network diagnostic test**
2. **Collect log files** from problematic session
3. **Document exact error messages**
4. **List system specifications** (OS, network setup, etc.)
5. **Try basic troubleshooting steps** from this guide

### Support Channels

#### Community Support
- **Discord**: [Sudachi Community](https://discord.gg/sudachi) - Real-time help
- **Reddit**: [r/SudachiEmulator](https://reddit.com/r/SudachiEmulator) - Community discussion
- **Forums**: [Official Forums](https://community.sudachi-emulator.org) - Structured support

#### Technical Support
- **GitHub Issues**: [Report Bugs](https://github.com/sudachi-emulator/sudachi/issues)
- **Developer Documentation**: [Technical Guides](../developer-guide/)
- **Security Issues**: [Security Contact](../security/vulnerability-reporting.md)

### Information to Include in Support Requests

#### System Information
- Operating system and version
- Sudachi version and build
- Network configuration (router model, ISP, etc.)
- Hardware specifications

#### Problem Description
- Exact error messages
- Steps to reproduce the issue
- Expected vs actual behavior
- When the problem started occurring

#### Log Files
- Sudachi application logs
- System event logs (if relevant)
- Network diagnostic output
- Screenshots of error dialogs

## FAQ

### General Questions

**Q: Why is multiplayer not working with my game?**
A: Check the [Supported Games](supported-games.md) list. Not all games support local wireless multiplayer, and some may have specific requirements.

**Q: Can I play with users on different platforms?**
A: Yes, Internet Multiplayer (Model A) supports cross-platform play. Offline Multiplayer (Model B) requires compatible hardware on all devices.

**Q: Is there a player limit for multiplayer sessions?**
A: This depends on the game. Most Nintendo Switch games support 2-8 players in local wireless mode.

**Q: Do I need to port forward for multiplayer?**
A: Not required for clients. Hosts may benefit from port forwarding for optimal connectivity, but UPnP can handle this automatically in many cases.

### Technical Questions

**Q: What ports does Sudachi multiplayer use?**
A: Default range is 7777-7787 TCP/UDP, but this is configurable in settings.

**Q: How much bandwidth does multiplayer use?**
A: Varies by game, but typically 100-500 KB/s per player. Racing games and action games may use more.

**Q: Can I use VPN with Sudachi multiplayer?**
A: Yes, but VPN may increase latency. Some VPNs may block P2P connections.

**Q: Is multiplayer data encrypted?**
A: Yes, all traffic is encrypted using modern TLS/SSL protocols.

### Performance Questions

**Q: Why is my connection slow/laggy?**
A: Check network latency using the built-in test. Wired connections, 5GHz Wi-Fi, and closing other network applications can help.

**Q: Can I improve connection quality?**
A: Enable relay fallback, adjust bandwidth limits, and ensure good Wi-Fi signal strength. Consider geographic distance to other players.

**Q: My game runs slowly in multiplayer but fine single-player. Why?**
A: Multiplayer adds network and synchronization overhead. Lower graphics settings and ensure stable network connection.

---

*Last updated: August 4, 2025*
*Still having issues? Join our [Discord Community](https://discord.gg/sudachi) for real-time support*