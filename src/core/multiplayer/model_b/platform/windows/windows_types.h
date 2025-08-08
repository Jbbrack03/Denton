// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Core::Multiplayer::ModelB::Windows {

/**
 * Enums matching Windows API types
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
 * Data structures
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

} // namespace Core::Multiplayer::ModelB::Windows