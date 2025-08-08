// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Core::Multiplayer::ModelB::Testing {

/**
 * Enums matching Windows API types for testing
 */
enum class TetheringCapability {
    Enabled,
    DisabledByGroupPolicy,
    DisabledByHardwareLimitation,
    DisabledByOperator,
    DisabledBySku,
    DisabledByRequiredAppNotInstalled
};

enum class TetheringOperationalState {
    Unknown,
    Off,
    On,
    InTransition
};

enum class NetworkInterfaceType {
    Unknown,
    Ethernet,
    Wireless80211,
    Bluetooth,
    Loopback,
    Cellular
};

enum class OperationalStatus {
    Up,
    Down,
    Testing,
    Unknown,
    Dormant,
    NotPresent,
    LowerLayerDown
};

enum class ServiceStatus {
    Unknown,
    Stopped,
    Running,
    Disabled,
    StartPending,
    StopPending
};

enum class WiFiBand {
    TwoPointFourGHz,
    FiveGHz,
    SixGHz
};

enum class HotspotState {
    Uninitialized,
    Initialized,
    Configured,
    Active,
    Error
};

enum class FallbackMode {
    WiFiDirect,
    InternetMode,
    BluetoothTethering
};

enum class OperationMode {
    MobileHotspot,
    WiFiDirectFallback,
    InternetFallback,
    BluetoothFallback
};

enum class TetheringSupport {
    None,
    Partial,
    Full
};

/**
 * Data structures for testing
 */
struct WindowsVersionInfo {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t build_number;
    std::string version_name;
};

struct WiFiAdapterInfo {
    std::string name;
    std::string manufacturer;
    std::string driver_version;
    bool supports_hosted_network;
    bool supports_wifi_direct;
    std::vector<WiFiBand> supported_bands;
};

struct NetworkAdapterInfo {
    std::string name;
    NetworkInterfaceType interface_type;
    bool is_connected;
    OperationalStatus operational_status;
};

struct ClientInfo {
    std::string mac_address;
    std::string ip_address;
    std::string device_name;
};

struct HotspotConfiguration {
    std::string ssid;
    std::string passphrase;
    int max_clients = 8;
    WiFiBand band = WiFiBand::TwoPointFourGHz;
    int channel = 6;
};

struct HotspotStatus {
    bool is_active = false;
    std::string ssid;
    int connected_clients = 0;
    std::vector<ClientInfo> client_list;
};

struct InternetConnectivity {
    bool has_internet;
    NetworkInterfaceType primary_adapter_type;
    std::vector<NetworkAdapterInfo> connected_adapters;
    bool can_use_loopback_workaround;
};

struct ChannelAnalysis {
    int best_2_4ghz_channel;
    int best_5ghz_channel;
    bool is_5ghz_clear;
    std::map<int, int> channel_usage;
};

struct OptimalHotspotConfig {
    WiFiBand preferred_band;
    int max_clients;
    std::string suggested_ssid;
    int recommended_channel;
};

struct FallbackCapability {
    bool is_available;
    std::string reason;
};

struct PolicyRestrictions {
    bool checked;
    bool group_policy_allows;
    bool mdm_allows;
    bool ics_service_available;
    bool edition_supports;
};

struct CapabilityReport {
    WindowsVersionInfo windows_version;
    std::vector<WiFiAdapterInfo> wifi_adapters;
    bool has_wifi_adapters;
    TetheringSupport tethering_support;
    InternetConnectivity connectivity;
    std::map<FallbackMode, FallbackCapability> fallback_modes;
    PolicyRestrictions policy_restrictions;
};

struct OperationModeRecommendation {
    OperationMode primary_mode;
    std::vector<FallbackMode> fallback_modes;
    std::string reason;
};

/**
 * Mock NetworkOperatorTetheringManager
 */
class MockNetworkOperatorTetheringManager {
public:
    MOCK_METHOD(TetheringCapability, GetTetheringCapability, (), ());
    MOCK_METHOD(TetheringOperationalState, GetTetheringOperationalState, (), ());
    MOCK_METHOD(bool, StartTethering, (const HotspotConfiguration& config), ());
    MOCK_METHOD(bool, StopTethering, (), ());
    MOCK_METHOD(int, GetClientCount, (), ());
    MOCK_METHOD(std::vector<ClientInfo>, GetConnectedClients, (), ());
};

/**
 * Mock Network Adapter
 */
class MockNetworkAdapter {
public:
    MOCK_METHOD(NetworkInterfaceType, GetInterfaceType, (), ());
    MOCK_METHOD(bool, IsConnected, (), ());
    MOCK_METHOD(OperationalStatus, GetOperationalStatus, (), ());
    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(std::string, GetDescription, (), ());
};

/**
 * Mock Windows Registry
 */
class MockWindowsRegistry {
public:
    MOCK_METHOD(bool, ReadDWORD, (const std::string& key, const std::string& value, uint32_t& result), ());
    MOCK_METHOD(bool, ReadString, (const std::string& key, const std::string& value, std::string& result), ());
    MOCK_METHOD(bool, WriteDWORD, (const std::string& key, const std::string& value, uint32_t data), ());
    MOCK_METHOD(bool, WriteString, (const std::string& key, const std::string& value, const std::string& data), ());
};

/**
 * Mock WMI Provider for hardware detection
 */
class MockWMIProvider {
public:
    MOCK_METHOD(std::vector<WiFiAdapterInfo>, QueryWiFiAdapters, (), ());
    MOCK_METHOD(std::vector<NetworkAdapterInfo>, QueryNetworkAdapters, (), ());
    MOCK_METHOD(std::string, QuerySystemInfo, (const std::string& property), ());
};

/**
 * Mock WinRT APIs
 */
class MockWinRTAPIs {
public:
    // Version detection
    MOCK_METHOD(WindowsVersionInfo, GetWindowsVersion, (), ());
    MOCK_METHOD(bool, IsElevated, (), ());
    
    // Tethering manager
    MOCK_METHOD(MockNetworkOperatorTetheringManager*, CreateTetheringManager, (), ());
    
    // Network enumeration
    MOCK_METHOD(std::vector<MockNetworkAdapter*>, EnumerateNetworkAdapters, (), ());
    MOCK_METHOD(std::vector<NetworkAdapterInfo>, GetNetworkAdapters, (), ());
    
    // WiFi scanning
    MOCK_METHOD(std::map<int, int>, ScanNearbyNetworks, (), ());
    
    // Service management
    MOCK_METHOD(ServiceStatus, GetServiceStatus, (const std::string& service_name), ());
    
    // Bluetooth
    MOCK_METHOD(bool, IsBluetoothAvailable, (), ());
    MOCK_METHOD(std::string, GetBluetoothVersion, (), ());
    
    // WiFi Direct
    MOCK_METHOD(bool, IsWiFiDirectSupported, (), ());
    
    // Test helpers
    void SetWindowsVersion(uint32_t major, uint32_t minor, uint32_t build) {
        version_.major_version = major;
        version_.minor_version = minor;
        version_.build_number = build;
        
        // Set version name based on build number
        if (major == 11) {
            version_.version_name = "Windows 11";
        } else if (major == 10) {
            if (build >= 19041) {
                version_.version_name = "Windows 10 2004";
            } else if (build >= 18362) {
                version_.version_name = "Windows 10 1903";
            } else if (build >= 17763) {
                version_.version_name = "Windows 10 1809";
            } else if (build >= 17134) {
                version_.version_name = "Windows 10 1803";
            } else if (build >= 16299) {
                version_.version_name = "Windows 10 1709";
            } else if (build >= 15063) {
                version_.version_name = "Windows 10 1703";
            } else if (build >= 14393) {
                version_.version_name = "Windows 10 1607";
            } else if (build >= 10586) {
                version_.version_name = "Windows 10 1511";
            } else {
                version_.version_name = "Windows 10 RTM";
            }
        } else if (major == 6 && minor == 3) {
            version_.version_name = "Windows 8.1";
        } else if (major == 6 && minor == 1) {
            version_.version_name = "Windows 7";
        } else {
            version_.version_name = "Unknown Windows Version";
        }
        
        // Setup mock to return this version
        ON_CALL(*this, GetWindowsVersion())
            .WillByDefault(testing::Return(version_));
    }
    
    void SetWiFiDirectSupported(bool supported) {
        ON_CALL(*this, IsWiFiDirectSupported())
            .WillByDefault(testing::Return(supported));
    }
    
private:
    WindowsVersionInfo version_{10, 0, 14393, "Windows 10 1607"};
};

} // namespace Core::Multiplayer::ModelB::Testing
