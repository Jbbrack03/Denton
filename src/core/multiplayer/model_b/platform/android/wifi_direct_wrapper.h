// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <jni.h>

#include "../../common/error_codes.h"

namespace Core::Multiplayer::ModelB::Android {

// Forward declarations
class MockJNIEnv;
class MockAndroidContext;

// Represents discovered Wi-Fi Direct peer information
struct WifiP2pDevice {
    std::string device_name;
    std::string device_address;
    int status; // WifiP2pDevice.CONNECTED, INVITED, FAILED, AVAILABLE, UNAVAILABLE
    bool is_group_owner;
};

// Represents Wi-Fi Direct group information
struct WifiP2pGroup {
    std::string network_name;
    std::string passphrase;
    bool is_group_owner;
    std::vector<WifiP2pDevice> clients;
};

// Wi-Fi Direct connection state
enum class WifiDirectState {
    Uninitialized,
    Initialized,
    Discovering,
    Connecting,
    Connected,
    GroupOwner,
    GroupClient,
    Error
};

// Callback types for Wi-Fi Direct events
using PeerDiscoveryCallback = std::function<void(const std::vector<WifiP2pDevice>&)>;
using ConnectionStateCallback = std::function<void(WifiDirectState)>;
using GroupInfoCallback = std::function<void(const WifiP2pGroup&)>;

// Main Wi-Fi Direct wrapper class
class WiFiDirectWrapper {
public:
    WiFiDirectWrapper();
    ~WiFiDirectWrapper();

    // Initialization and cleanup
    [[nodiscard]] ErrorCode Initialize(JavaVM* jvm, jobject android_context);
    [[nodiscard]] ErrorCode Initialize(MockJNIEnv* mock_env, MockAndroidContext* mock_context);
    void Shutdown();

    // State management
    [[nodiscard]] WifiDirectState GetState() const;
    bool IsInitialized() const;

    // Peer discovery
    [[nodiscard]] ErrorCode StartDiscovery(PeerDiscoveryCallback callback);
    [[nodiscard]] ErrorCode StopDiscovery();
    [[nodiscard]] std::vector<WifiP2pDevice> GetDiscoveredPeers() const;

    // Connection management
    [[nodiscard]] ErrorCode ConnectToPeer(const std::string& device_address);
    [[nodiscard]] ErrorCode DisconnectFromPeer();
    [[nodiscard]] ErrorCode CancelConnection();

    // Group management
    [[nodiscard]] ErrorCode CreateGroup();
    [[nodiscard]] ErrorCode RemoveGroup();
    [[nodiscard]] ErrorCode JoinGroup(const std::string& device_address);
    [[nodiscard]] WifiP2pGroup GetGroupInfo() const;

    // Callbacks
    void SetConnectionStateCallback(ConnectionStateCallback callback);
    void SetGroupInfoCallback(GroupInfoCallback callback);

    // Configuration
    void SetDiscoveryTimeout(std::chrono::seconds timeout);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer::ModelB::Android
