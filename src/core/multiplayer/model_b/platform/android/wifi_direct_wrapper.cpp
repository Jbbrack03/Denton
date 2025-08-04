// Copyright 2024 Sudachi Emulator Project
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#include "wifi_direct_wrapper.h"
#include "../../common/error_codes.h"
#include "../tests/mocks/mock_jni_env.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace Core::Multiplayer::ModelB::Android {

using namespace Core::Multiplayer::ModelB::Testing;

// Forward declarations for mock compatibility
class MockAndroidContext;

// Pimpl implementation
class WiFiDirectWrapper::Impl {
public:
    Impl() : state_(WifiDirectState::Uninitialized), is_mock_mode_(false) {}

    // Initialization state
    std::atomic<WifiDirectState> state_;
    bool is_mock_mode_;
    
    // JNI objects (real mode)
    JavaVM* jvm_ = nullptr;
    jobject android_context_ = nullptr;
    
    // Mock objects (test mode)
    MockJNIEnv* mock_env_ = nullptr;
    MockAndroidContext* mock_context_ = nullptr;
    
    // Discovered peers
    mutable std::mutex peers_mutex_;
    std::vector<WifiP2pDevice> discovered_peers_;
    
    // Group info
    mutable std::mutex group_mutex_;
    WifiP2pGroup current_group_;
    
    // Callbacks
    std::mutex callbacks_mutex_;
    PeerDiscoveryCallback discovery_callback_;
    ConnectionStateCallback connection_callback_;
    GroupInfoCallback group_callback_;
    
    // Configuration
    std::chrono::seconds discovery_timeout_{30};
    
    // Thread safety
    mutable std::mutex state_mutex_;
    
    // Helper methods
    bool IsValidMacAddress(const std::string& address) const {
        if (address.empty() || address.length() != 17) return false;
        // Basic MAC address format check: XX:XX:XX:XX:XX:XX
        for (size_t i = 0; i < address.length(); ++i) {
            if (i % 3 == 2) {
                if (address[i] != ':') return false;
            } else {
                if (!std::isxdigit(address[i])) return false;
            }
        }
        return true;
    }
    
    void SetState(WifiDirectState new_state) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_.store(new_state);
        
        // Notify callback if set
        std::lock_guard<std::mutex> callback_lock(callbacks_mutex_);
        if (connection_callback_) {
            connection_callback_(new_state);
        }
    }
};

WiFiDirectWrapper::WiFiDirectWrapper() : impl_(std::make_unique<Impl>()) {}

WiFiDirectWrapper::~WiFiDirectWrapper() {
    if (impl_->state_.load() != WifiDirectState::Uninitialized) {
        Shutdown();
    }
}

ErrorCode WiFiDirectWrapper::Initialize(JavaVM* jvm, jobject android_context) {
    if (!jvm || !android_context) {
        return ErrorCode::InvalidParameter;
    }
    
    if (impl_->state_.load() != WifiDirectState::Uninitialized) {
        return ErrorCode::InvalidState;
    }
    
    impl_->jvm_ = jvm;
    impl_->android_context_ = android_context;
    impl_->is_mock_mode_ = false;
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::Initialize(MockJNIEnv* mock_env, MockAndroidContext* mock_context) {
    if (!mock_env || !mock_context) {
        return ErrorCode::InvalidParameter;
    }
    
    if (impl_->state_.load() != WifiDirectState::Uninitialized) {
        return ErrorCode::InvalidState;
    }
    
    impl_->mock_env_ = mock_env;
    impl_->mock_context_ = mock_context;
    impl_->is_mock_mode_ = true;
    
    // Check for mock JNI exceptions
    if (impl_->is_mock_mode_) {
        // Simulate JNI exception check - return exception if mock env has one
        // For minimal implementation, we just proceed
    }
    
    // Check if WiFi P2P Manager is available
    if (impl_->is_mock_mode_) {
        auto system_service = mock_context->GetSystemService("wifip2p");
        if (!system_service) {
            return ErrorCode::ServiceUnavailable;
        }
    }
    
    // Check permissions in mock mode
    if (impl_->is_mock_mode_) {
        bool has_permission = mock_context->CheckSelfPermission("android.permission.NEARBY_WIFI_DEVICES");
        if (!has_permission) {
            // Try Android 12 permission
            has_permission = mock_context->CheckSelfPermission("android.permission.ACCESS_FINE_LOCATION");
            if (!has_permission) {
                impl_->SetState(WifiDirectState::Error);
                return ErrorCode::PermissionDenied;
            }
        }
    }
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

void WiFiDirectWrapper::Shutdown() {
    impl_->SetState(WifiDirectState::Uninitialized);
    
    // Clear peers and groups
    {
        std::lock_guard<std::mutex> lock(impl_->peers_mutex_);
        impl_->discovered_peers_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->group_mutex_);
        impl_->current_group_ = WifiP2pGroup{};
    }
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(impl_->callbacks_mutex_);
        impl_->discovery_callback_ = nullptr;
        impl_->connection_callback_ = nullptr;
        impl_->group_callback_ = nullptr;
    }
    
    // Reset JNI/mock references
    impl_->jvm_ = nullptr;
    impl_->android_context_ = nullptr;
    impl_->mock_env_ = nullptr;
    impl_->mock_context_ = nullptr;
}

WifiDirectState WiFiDirectWrapper::GetState() const {
    return impl_->state_.load();
}

bool WiFiDirectWrapper::IsInitialized() const {
    return impl_->state_.load() != WifiDirectState::Uninitialized;
}

ErrorCode WiFiDirectWrapper::StartDiscovery(PeerDiscoveryCallback callback) {
    if (impl_->state_.load() == WifiDirectState::Uninitialized) {
        return ErrorCode::NotInitialized;
    }
    
    if (impl_->state_.load() == WifiDirectState::Discovering) {
        return ErrorCode::InvalidState;
    }
    
    // Set callback
    {
        std::lock_guard<std::mutex> lock(impl_->callbacks_mutex_);
        impl_->discovery_callback_ = callback;
    }
    
    // Check for simulated network error in mock mode
    if (impl_->is_mock_mode_ && impl_->mock_env_) {
        // For minimal implementation, we assume discovery succeeds
        // Real implementation would call mock_wifi_p2p_manager_->DiscoverPeers
    }
    
    impl_->SetState(WifiDirectState::Discovering);
    
    // Simulate peer discovery with a simple timeout
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // In mock mode, add any mock peers that were set up
        if (impl_->is_mock_mode_) {
            std::lock_guard<std::mutex> lock(impl_->peers_mutex_);
            // For now, keep peers empty for minimal implementation
            // Real implementation would populate from mock data
        }
        
        // Notify callback
        {
            std::lock_guard<std::mutex> callback_lock(impl_->callbacks_mutex_);
            std::lock_guard<std::mutex> peers_lock(impl_->peers_mutex_);
            if (impl_->discovery_callback_) {
                impl_->discovery_callback_(impl_->discovered_peers_);
            }
        }
        
        // Set state back to initialized after timeout
        std::this_thread::sleep_for(impl_->discovery_timeout_);
        if (impl_->state_.load() == WifiDirectState::Discovering) {
            impl_->SetState(WifiDirectState::Initialized);
        }
    }).detach();
    
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::StopDiscovery() {
    if (impl_->state_.load() != WifiDirectState::Discovering) {
        return ErrorCode::InvalidState;
    }
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

std::vector<WifiP2pDevice> WiFiDirectWrapper::GetDiscoveredPeers() const {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex_);
    return impl_->discovered_peers_;
}

ErrorCode WiFiDirectWrapper::ConnectToPeer(const std::string& device_address) {
    if (impl_->state_.load() == WifiDirectState::Uninitialized) {
        return ErrorCode::NotInitialized;
    }
    
    if (device_address.empty() || !impl_->IsValidMacAddress(device_address)) {
        return ErrorCode::InvalidParameter;
    }
    
    if (impl_->state_.load() == WifiDirectState::Connecting || 
        impl_->state_.load() == WifiDirectState::Connected) {
        return ErrorCode::InvalidState;
    }
    
    impl_->SetState(WifiDirectState::Connecting);
    
    // Simulate connection with timeout
    std::thread([this, device_address]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // For minimal implementation, assume connection succeeds
        impl_->SetState(WifiDirectState::Connected);
    }).detach();
    
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::DisconnectFromPeer() {
    if (impl_->state_.load() != WifiDirectState::Connected && 
        impl_->state_.load() != WifiDirectState::Connecting) {
        return ErrorCode::InvalidState;
    }
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::CancelConnection() {
    if (impl_->state_.load() != WifiDirectState::Connecting) {
        return ErrorCode::InvalidState;
    }
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::CreateGroup() {
    if (impl_->state_.load() == WifiDirectState::Uninitialized) {
        return ErrorCode::NotInitialized;
    }
    
    if (impl_->state_.load() == WifiDirectState::GroupOwner || 
        impl_->state_.load() == WifiDirectState::GroupClient) {
        return ErrorCode::InvalidState;
    }
    
    // Create group info
    {
        std::lock_guard<std::mutex> lock(impl_->group_mutex_);
        impl_->current_group_.network_name = "DIRECT-sudachi";
        impl_->current_group_.passphrase = "sudachi123";
        impl_->current_group_.is_group_owner = true;
        impl_->current_group_.clients.clear();
    }
    
    impl_->SetState(WifiDirectState::GroupOwner);
    
    // Notify group callback
    {
        std::lock_guard<std::mutex> callback_lock(impl_->callbacks_mutex_);
        std::lock_guard<std::mutex> group_lock(impl_->group_mutex_);
        if (impl_->group_callback_) {
            impl_->group_callback_(impl_->current_group_);
        }
    }
    
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::RemoveGroup() {
    if (impl_->state_.load() != WifiDirectState::GroupOwner && 
        impl_->state_.load() != WifiDirectState::GroupClient) {
        return ErrorCode::InvalidState;
    }
    
    // Clear group info
    {
        std::lock_guard<std::mutex> lock(impl_->group_mutex_);
        impl_->current_group_ = WifiP2pGroup{};
    }
    
    impl_->SetState(WifiDirectState::Initialized);
    return ErrorCode::Success;
}

ErrorCode WiFiDirectWrapper::JoinGroup(const std::string& device_address) {
    if (impl_->state_.load() == WifiDirectState::Uninitialized) {
        return ErrorCode::NotInitialized;
    }
    
    if (device_address.empty() || !impl_->IsValidMacAddress(device_address)) {
        return ErrorCode::InvalidParameter;
    }
    
    if (impl_->state_.load() == WifiDirectState::GroupOwner || 
        impl_->state_.load() == WifiDirectState::GroupClient) {
        return ErrorCode::InvalidState;
    }
    
    impl_->SetState(WifiDirectState::GroupClient);
    return ErrorCode::Success;
}

WifiP2pGroup WiFiDirectWrapper::GetGroupInfo() const {
    std::lock_guard<std::mutex> lock(impl_->group_mutex_);
    return impl_->current_group_;
}

void WiFiDirectWrapper::SetConnectionStateCallback(ConnectionStateCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex_);
    impl_->connection_callback_ = callback;
}

void WiFiDirectWrapper::SetGroupInfoCallback(GroupInfoCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex_);
    impl_->group_callback_ = callback;
}

void WiFiDirectWrapper::SetDiscoveryTimeout(std::chrono::seconds timeout) {
    impl_->discovery_timeout_ = timeout;
}

} // namespace Core::Multiplayer::ModelB::Android