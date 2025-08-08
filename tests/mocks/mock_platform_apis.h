// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>

namespace Sudachi::Multiplayer::Tests {

// Android Mock APIs
#ifdef ANDROID

/**
 * Mock JNI Environment for Android testing
 */
class MockJNIEnv {
public:
    MOCK_METHOD(jclass, FindClass, (const char* name));
    MOCK_METHOD(jmethodID, GetMethodID, (jclass clazz, const char* name, const char* sig));
    MOCK_METHOD(jobject, CallObjectMethod, (jobject obj, jmethodID methodID, ...));
    MOCK_METHOD(void, CallVoidMethod, (jobject obj, jmethodID methodID, ...));
    MOCK_METHOD(jboolean, CallBooleanMethod, (jobject obj, jmethodID methodID, ...));
    MOCK_METHOD(jint, CallIntMethod, (jobject obj, jmethodID methodID, ...));
    MOCK_METHOD(jstring, NewStringUTF, (const char* utf));
    MOCK_METHOD(const char*, GetStringUTFChars, (jstring str, jboolean* isCopy));
    MOCK_METHOD(void, ReleaseStringUTFChars, (jstring str, const char* chars));
    MOCK_METHOD(void, DeleteLocalRef, (jobject obj));
    MOCK_METHOD(jobject, NewGlobalRef, (jobject obj));
    MOCK_METHOD(void, DeleteGlobalRef, (jobject obj));
};

/**
 * Mock Android WiFi Direct Manager
 */
class MockWifiP2pManager {
public:
    enum class ActionResult {
        Success,
        P2PUnsupported,
        Error,
        Busy
    };

    struct DeviceInfo {
        std::string device_name;
        std::string device_address;
        bool is_group_owner;
        int status; // 0=connected, 1=invited, 2=failed, 3=available, 4=unavailable
    };

    MOCK_METHOD(void, Initialize, (MockJNIEnv* env, jobject context));
    MOCK_METHOD(void, CreateGroup, (std::function<void(ActionResult)> callback));
    MOCK_METHOD(void, RemoveGroup, (std::function<void(ActionResult)> callback));
    MOCK_METHOD(void, DiscoverPeers, (std::function<void(ActionResult)> callback));
    MOCK_METHOD(void, StopDiscovery, (std::function<void(ActionResult)> callback));
    MOCK_METHOD(void, Connect, (const std::string& device_address, std::function<void(ActionResult)> callback));
    MOCK_METHOD(void, CancelConnect, (std::function<void(ActionResult)> callback));
    MOCK_METHOD(std::vector<DeviceInfo>, GetPeerList, (), (const));
    MOCK_METHOD(std::optional<DeviceInfo>, GetConnectionInfo, (), (const));
    MOCK_METHOD(bool, IsWifiP2pEnabled, (), (const));

    // Test helpers
    void SimulatePeerDiscovered(const DeviceInfo& device) {
        discovered_peers_.push_back(device);
        if (on_peers_changed_) {
            on_peers_changed_(discovered_peers_);
        }
    }

    void SimulateConnectionEstablished(const std::string& device_address, bool as_group_owner) {
        connection_info_ = DeviceInfo{
            "Connected Device",
            device_address,
            as_group_owner,
            0 // connected
        };
        if (on_connection_changed_) {
            on_connection_changed_(connection_info_.value());
        }
    }

    void SetOnPeersChanged(std::function<void(const std::vector<DeviceInfo>&)> callback) {
        on_peers_changed_ = callback;
    }

    void SetOnConnectionChanged(std::function<void(const DeviceInfo&)> callback) {
        on_connection_changed_ = callback;
    }

private:
    std::vector<DeviceInfo> discovered_peers_;
    std::optional<DeviceInfo> connection_info_;
    std::function<void(const std::vector<DeviceInfo>&)> on_peers_changed_;
    std::function<void(const DeviceInfo&)> on_connection_changed_;
};

/**
 * Mock Android Permission Manager
 */
class MockAndroidPermissionManager {
public:
    MOCK_METHOD(bool, HasPermission, (const std::string& permission), (const));
    MOCK_METHOD(void, RequestPermissions, (const std::vector<std::string>& permissions, 
                                          std::function<void(const std::map<std::string, bool>&)> callback));
    MOCK_METHOD(bool, ShouldShowRationale, (const std::string& permission), (const));
    MOCK_METHOD(int, GetTargetSdkVersion, (), (const));

    // Test helper to simulate permission grant
    void SimulatePermissionResult(const std::string& permission, bool granted) {
        permission_results_[permission] = granted;
    }

private:
    std::map<std::string, bool> permission_results_;
};

#endif // ANDROID

// Windows Mock APIs
#ifdef _WIN32

/**
 * Mock Windows Network Operator Tethering Manager
 */
class MockNetworkOperatorTetheringManager {
public:
    enum class TetheringOperationStatus {
        Success,
        Unknown,
        MobileOperatorNotFound,
        WiFiDeviceTurnedOff,
        EntitlementCheckTimeout,
        EntitlementCheckFailure,
        OperationInProgress,
        BluetoothDeviceTurnedOff,
        NetworkLimitedConnectivity
    };

    enum class TetheringOperationalState {
        Unknown,
        On,
        Off,
        InTransition
    };

    struct TetheringConfiguration {
        std::wstring ssid;
        std::wstring passphrase;
        uint32_t band; // 0=auto, 1=2.4GHz, 2=5GHz
        bool hidden_network;
    };

    MOCK_METHOD(void, Initialize, ());
    MOCK_METHOD(TetheringOperationStatus, StartTetheringAsync, ());
    MOCK_METHOD(TetheringOperationStatus, StopTetheringAsync, ());
    MOCK_METHOD(TetheringOperationStatus, ConfigureAccessPointAsync, (const TetheringConfiguration& config));
    MOCK_METHOD(TetheringOperationalState, GetCurrentState, (), (const));
    MOCK_METHOD(uint32_t, GetConnectedClientCount, (), (const));
    MOCK_METHOD(uint32_t, GetMaxClientCount, (), (const));
    MOCK_METHOD(bool, IsNoConnectionsTimeoutEnabled, (), (const));
    MOCK_METHOD(void, SetNoConnectionsTimeout, (std::chrono::minutes timeout));

    // Test helpers
    void SimulateHotspotStarted() {
        current_state_ = TetheringOperationalState::On;
        if (on_state_changed_) {
            on_state_changed_(current_state_);
        }
    }

    void SimulateHotspotStopped() {
        current_state_ = TetheringOperationalState::Off;
        connected_clients_ = 0;
        if (on_state_changed_) {
            on_state_changed_(current_state_);
        }
    }

    void SimulateClientConnected() {
        connected_clients_++;
        if (on_client_count_changed_) {
            on_client_count_changed_(connected_clients_);
        }
    }

    void SimulateClientDisconnected() {
        if (connected_clients_ > 0) {
            connected_clients_--;
            if (on_client_count_changed_) {
                on_client_count_changed_(connected_clients_);
            }
        }
    }

    void SetOnStateChanged(std::function<void(TetheringOperationalState)> callback) {
        on_state_changed_ = callback;
    }

    void SetOnClientCountChanged(std::function<void(uint32_t)> callback) {
        on_client_count_changed_ = callback;
    }

private:
    TetheringOperationalState current_state_ = TetheringOperationalState::Off;
    uint32_t connected_clients_ = 0;
    std::function<void(TetheringOperationalState)> on_state_changed_;
    std::function<void(uint32_t)> on_client_count_changed_;
};

/**
 * Mock Windows Network Information
 */
class MockWindowsNetworkInformation {
public:
    enum class NetworkConnectivityLevel {
        None,
        LocalAccess,
        ConstrainedInternetAccess,
        InternetAccess
    };

    struct NetworkAdapter {
        std::wstring name;
        std::wstring description;
        std::wstring guid;
        bool is_wifi;
        bool supports_hosted_network;
        bool is_connected;
    };

    MOCK_METHOD(NetworkConnectivityLevel, GetInternetConnectionLevel, (), (const));
    MOCK_METHOD(std::vector<NetworkAdapter>, GetNetworkAdapters, (), (const));
    MOCK_METHOD(bool, HasInternetConnection, (), (const));
    MOCK_METHOD(std::optional<std::wstring>, GetConnectedSSID, (), (const));
    MOCK_METHOD(bool, IsMeteredConnection, (), (const));

    // Test helpers
    void SimulateInternetConnectionChange(NetworkConnectivityLevel level) {
        connectivity_level_ = level;
        if (on_connectivity_changed_) {
            on_connectivity_changed_(level);
        }
    }

    void AddMockAdapter(const NetworkAdapter& adapter) {
        adapters_.push_back(adapter);
    }

    void SetOnConnectivityChanged(std::function<void(NetworkConnectivityLevel)> callback) {
        on_connectivity_changed_ = callback;
    }

private:
    NetworkConnectivityLevel connectivity_level_ = NetworkConnectivityLevel::InternetAccess;
    std::vector<NetworkAdapter> adapters_;
    std::function<void(NetworkConnectivityLevel)> on_connectivity_changed_;
};

/**
 * Mock Windows Version Helper
 */
class MockWindowsVersionHelper {
public:
    struct VersionInfo {
        uint32_t major;
        uint32_t minor;
        uint32_t build;
        std::wstring edition; // "Home", "Pro", "Enterprise", etc.
    };

    MOCK_METHOD(VersionInfo, GetWindowsVersion, (), (const));
    MOCK_METHOD(bool, IsWindows10OrGreater, (), (const));
    MOCK_METHOD(bool, IsWindowsVersionOrGreater, (uint32_t major, uint32_t minor, uint32_t build), (const));
    MOCK_METHOD(bool, IsWindowsServer, (), (const));

    // Test helper
    void SetWindowsVersion(uint32_t major, uint32_t minor, uint32_t build, const std::wstring& edition = L"Pro") {
        version_info_ = {major, minor, build, edition};
    }

private:
    VersionInfo version_info_ = {10, 0, 19041, L"Pro"}; // Windows 10 20H1 by default
};

#endif // _WIN32

/**
 * Cross-platform mock network interface
 */
class MockNetworkInterface {
public:
    struct InterfaceInfo {
        std::string name;
        std::string description;
        std::vector<std::string> ipv4_addresses;
        std::vector<std::string> ipv6_addresses;
        std::string mac_address;
        bool is_up;
        bool is_loopback;
        bool is_wireless;
        size_t mtu;
    };

    MOCK_METHOD(std::vector<InterfaceInfo>, GetNetworkInterfaces, (), (const));
    MOCK_METHOD(std::optional<InterfaceInfo>, GetInterfaceByName, (const std::string& name), (const));
    MOCK_METHOD(std::optional<InterfaceInfo>, GetDefaultInterface, (), (const));
    MOCK_METHOD(bool, IsInterfaceUp, (const std::string& name), (const));

    // Test helpers
    void AddMockInterface(const InterfaceInfo& interface) {
        interfaces_.push_back(interface);
    }

    void SetInterfaceStatus(const std::string& name, bool is_up) {
        for (auto& iface : interfaces_) {
            if (iface.name == name) {
                iface.is_up = is_up;
                break;
            }
        }
    }

private:
    std::vector<InterfaceInfo> interfaces_;
};

/**
 * Test fixture for platform-specific tests
 */
class PlatformTestFixture {
public:
    PlatformTestFixture() {
        SetupDefaultBehavior();
    }

    void SetupDefaultBehavior() {
        using ::testing::_;
        using ::testing::Return;
        using ::testing::Invoke;

        // Setup mock network interface
        mock_network_interface_ = std::make_unique<MockNetworkInterface>();
        
        // Default network interface behavior
        ON_CALL(*mock_network_interface_, GetNetworkInterfaces())
            .WillByDefault(Return(std::vector<MockNetworkInterface::InterfaceInfo>{
                {"lo", "Loopback", {"127.0.0.1"}, {"::1"}, "00:00:00:00:00:00", true, true, false, 65536},
                {"eth0", "Ethernet", {"192.168.1.100"}, {"fe80::1"}, "aa:bb:cc:dd:ee:ff", true, false, false, 1500},
                {"wlan0", "Wi-Fi", {"192.168.1.101"}, {"fe80::2"}, "11:22:33:44:55:66", true, false, true, 1500}
            }));

#ifdef _WIN32
        mock_tethering_manager_ = std::make_unique<MockNetworkOperatorTetheringManager>();
        mock_network_info_ = std::make_unique<MockWindowsNetworkInformation>();
        mock_version_helper_ = std::make_unique<MockWindowsVersionHelper>();
        
        // Default Windows behavior
        ON_CALL(*mock_network_info_, HasInternetConnection())
            .WillByDefault(Return(true));
        ON_CALL(*mock_version_helper_, IsWindows10OrGreater())
            .WillByDefault(Return(true));
#endif

#ifdef ANDROID
        mock_jni_env_ = std::make_unique<MockJNIEnv>();
        mock_wifi_p2p_manager_ = std::make_unique<MockWifiP2pManager>();
        mock_permission_manager_ = std::make_unique<MockAndroidPermissionManager>();
        
        // Default Android behavior
        ON_CALL(*mock_wifi_p2p_manager_, IsWifiP2pEnabled())
            .WillByDefault(Return(true));
        ON_CALL(*mock_permission_manager_, GetTargetSdkVersion())
            .WillByDefault(Return(33)); // Android 13
#endif
    }

protected:
    std::unique_ptr<MockNetworkInterface> mock_network_interface_;
    
#ifdef _WIN32
    std::unique_ptr<MockNetworkOperatorTetheringManager> mock_tethering_manager_;
    std::unique_ptr<MockWindowsNetworkInformation> mock_network_info_;
    std::unique_ptr<MockWindowsVersionHelper> mock_version_helper_;
#endif

#ifdef ANDROID
    std::unique_ptr<MockJNIEnv> mock_jni_env_;
    std::unique_ptr<MockWifiP2pManager> mock_wifi_p2p_manager_;
    std::unique_ptr<MockAndroidPermissionManager> mock_permission_manager_;
#endif
};

} // namespace Sudachi::Multiplayer::Tests
