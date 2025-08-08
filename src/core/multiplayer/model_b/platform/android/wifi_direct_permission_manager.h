// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later


#pragma once

#include <string>
#include <vector>
#include <functional>
#include <jni.h>

#include "../../common/error_codes.h"

namespace Core::Multiplayer::ModelB::Android {

// Forward declarations
class MockAndroidContext;
class MockPermissionHandler;

// Permission request result
struct PermissionResult {
    std::string permission;
    bool granted;
    bool should_show_rationale;
};

// Callback for permission request results
using PermissionCallback = std::function<void(const std::vector<PermissionResult>&)>;

// Permission constants for different Android versions
class PermissionConstants {
public:
    // Android 13+ (API level 33+)
    static constexpr const char* NEARBY_WIFI_DEVICES = "android.permission.NEARBY_WIFI_DEVICES";
    
    // Android 12 and below
    static constexpr const char* ACCESS_FINE_LOCATION = "android.permission.ACCESS_FINE_LOCATION";
    
    // Common permissions
    static constexpr const char* ACCESS_WIFI_STATE = "android.permission.ACCESS_WIFI_STATE";
    static constexpr const char* CHANGE_WIFI_STATE = "android.permission.CHANGE_WIFI_STATE";
    static constexpr const char* INTERNET = "android.permission.INTERNET";
};

// Manages Wi-Fi Direct permissions for different Android versions
class WiFiDirectPermissionManager {
public:
    WiFiDirectPermissionManager();
    ~WiFiDirectPermissionManager();

    // Initialize with Android context
    [[nodiscard]] ErrorCode Initialize(jobject android_context);
    [[nodiscard]] ErrorCode Initialize(MockAndroidContext* mock_context, 
                                      MockPermissionHandler* mock_handler);

    // Get Android API level
    int GetApiLevel() const;

    // Check if Wi-Fi Direct is available
    bool IsWifiDirectAvailable() const;

    // Check if location services are enabled (required for Android 12-)
    bool IsLocationEnabled() const;

    // Get required permissions for current Android version
    std::vector<std::string> GetRequiredPermissions() const;

    // Check if a permission is granted
    bool IsPermissionGranted(const std::string& permission) const;

    // Check if all required permissions are granted
    bool AreAllPermissionsGranted() const;

    // Request permissions
    void RequestPermissions(const std::vector<std::string>& permissions,
                          PermissionCallback callback);

    // Check if should show permission rationale
    bool ShouldShowRationale(const std::string& permission) const;

    // Handle permission request results (called from JNI)
    void OnPermissionResult(const std::vector<PermissionResult>& results);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer::ModelB::Android