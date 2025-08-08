// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later


#include "wifi_direct_permission_manager.h"
#include "../../common/error_codes.h"
#include "../tests/mocks/mock_jni_env.h"
#include <mutex>
#include <unordered_map>
#include <algorithm>

namespace Core::Multiplayer::ModelB::Android {

using namespace Core::Multiplayer::ModelB::Testing;

// Forward declarations for mock compatibility
class MockPermissionHandler;

// Pimpl implementation
class WiFiDirectPermissionManager::Impl {
public:
    Impl() : is_mock_mode_(false), api_level_(33) {}

    // Mode detection
    bool is_mock_mode_;
    
    // JNI objects (real mode)
    jobject android_context_ = nullptr;
    
    // Mock objects (test mode)
    MockAndroidContext* mock_context_ = nullptr;
    MockPermissionHandler* mock_handler_ = nullptr;
    
    // API level
    int api_level_;
    
    // Permission state tracking
    mutable std::mutex permissions_mutex_;
    std::unordered_map<std::string, bool> permission_cache_;
    
    // Callback for permission results
    std::mutex callback_mutex_;
    PermissionCallback current_callback_;
    
    // Helper methods
    std::vector<std::string> GetRequiredPermissionsForApiLevel(int api_level) const {
        std::vector<std::string> permissions;
        
        // Always required permissions
        permissions.push_back(PermissionConstants::ACCESS_WIFI_STATE);
        permissions.push_back(PermissionConstants::CHANGE_WIFI_STATE);
        permissions.push_back(PermissionConstants::INTERNET);
        
        // API level specific permissions
        if (api_level >= 33) { // Android 13+
            permissions.push_back(PermissionConstants::NEARBY_WIFI_DEVICES);
        } else { // Android 12 and below
            permissions.push_back(PermissionConstants::ACCESS_FINE_LOCATION);
        }
        
        return permissions;
    }
    
    bool CheckPermissionInternal(const std::string& permission) const {
        if (is_mock_mode_ && mock_context_) {
            return mock_context_->CheckSelfPermission(permission);
        }
        
        // In real mode, would use JNI to check permission
        // For minimal implementation, return false
        return false;
    }
    
    int GetApiLevelInternal() const {
        if (is_mock_mode_ && mock_context_) {
            return mock_context_->GetSDKVersion();
        }
        
        // In real mode, would use JNI to get Build.VERSION.SDK_INT
        return api_level_;
    }
};

WiFiDirectPermissionManager::WiFiDirectPermissionManager() : impl_(std::make_unique<Impl>()) {}

WiFiDirectPermissionManager::~WiFiDirectPermissionManager() = default;

ErrorCode WiFiDirectPermissionManager::Initialize(jobject android_context) {
    if (!android_context) {
        return ErrorCode::InvalidParameter;
    }
    
    impl_->android_context_ = android_context;
    impl_->is_mock_mode_ = false;
    
    // Get API level
    impl_->api_level_ = impl_->GetApiLevelInternal();
    
    return ErrorCode::Success;
}

ErrorCode WiFiDirectPermissionManager::Initialize(MockAndroidContext* mock_context, 
                                                  MockPermissionHandler* mock_handler) {
    if (!mock_context) {
        return ErrorCode::InvalidParameter;
    }
    
    impl_->mock_context_ = mock_context;
    impl_->mock_handler_ = mock_handler;
    impl_->is_mock_mode_ = true;
    
    // Get API level from mock
    impl_->api_level_ = impl_->GetApiLevelInternal();
    
    return ErrorCode::Success;
}

int WiFiDirectPermissionManager::GetApiLevel() const {
    return impl_->api_level_;
}

bool WiFiDirectPermissionManager::IsWifiDirectAvailable() const {
    // For minimal implementation, assume Wi-Fi Direct is available
    // Real implementation would check PackageManager.hasSystemFeature("android.hardware.wifi.direct")
    return true;
}

bool WiFiDirectPermissionManager::IsLocationEnabled() const {
    // For minimal implementation, assume location is enabled
    // Real implementation would check Settings.Secure.LOCATION_MODE
    return true;
}

std::vector<std::string> WiFiDirectPermissionManager::GetRequiredPermissions() const {
    return impl_->GetRequiredPermissionsForApiLevel(impl_->api_level_);
}

bool WiFiDirectPermissionManager::IsPermissionGranted(const std::string& permission) const {
    if (permission.empty()) {
        return false;
    }
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(impl_->permissions_mutex_);
        auto it = impl_->permission_cache_.find(permission);
        if (it != impl_->permission_cache_.end()) {
            return it->second;
        }
    }
    
    // Check actual permission
    bool granted = impl_->CheckPermissionInternal(permission);
    
    // Update cache
    {
        std::lock_guard<std::mutex> lock(impl_->permissions_mutex_);
        impl_->permission_cache_[permission] = granted;
    }
    
    return granted;
}

bool WiFiDirectPermissionManager::AreAllPermissionsGranted() const {
    auto required_permissions = GetRequiredPermissions();
    
    for (const auto& permission : required_permissions) {
        if (!IsPermissionGranted(permission)) {
            return false;
        }
    }
    
    return true;
}

void WiFiDirectPermissionManager::RequestPermissions(const std::vector<std::string>& permissions,
                                                     PermissionCallback callback) {
    if (permissions.empty()) {
        // Call callback with empty results
        if (callback) {
            callback({});
        }
        return;
    }
    
    // Store callback
    {
        std::lock_guard<std::mutex> lock(impl_->callback_mutex_);
        impl_->current_callback_ = callback;
    }
    
    // In mock mode, simulate permission request
    if (impl_->is_mock_mode_) {
        std::vector<PermissionResult> results;
        
        for (const auto& permission : permissions) {
            PermissionResult result;
            result.permission = permission;
            result.granted = IsPermissionGranted(permission);
            result.should_show_rationale = !result.granted; // Simple logic for mock
            results.push_back(result);
        }
        
        // Notify callback immediately for mock
        if (callback) {
            callback(results);
        }
    } else {
        // In real mode, would start permission request activity
        // For minimal implementation, just call callback with denied results
        std::vector<PermissionResult> results;
        
        for (const auto& permission : permissions) {
            PermissionResult result;
            result.permission = permission;
            result.granted = false; // Default to denied in real mode
            result.should_show_rationale = true;
            results.push_back(result);
        }
        
        if (callback) {
            callback(results);
        }
    }
}

bool WiFiDirectPermissionManager::ShouldShowRationale(const std::string& permission) const {
    if (permission.empty()) {
        return false;
    }
    
    // For minimal implementation, show rationale if permission is not granted
    return !IsPermissionGranted(permission);
}

void WiFiDirectPermissionManager::OnPermissionResult(const std::vector<PermissionResult>& results) {
    // Update permission cache
    {
        std::lock_guard<std::mutex> lock(impl_->permissions_mutex_);
        for (const auto& result : results) {
            impl_->permission_cache_[result.permission] = result.granted;
        }
    }
    
    // Notify callback
    {
        std::lock_guard<std::mutex> lock(impl_->callback_mutex_);
        if (impl_->current_callback_) {
            impl_->current_callback_(results);
            impl_->current_callback_ = nullptr;
        }
    }
}

} // namespace Core::Multiplayer::ModelB::Android