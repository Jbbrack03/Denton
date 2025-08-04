// Simple test to verify Android Wi-Fi Direct implementation works

#include <iostream>
#include <cassert>
#include "../platform/android/wifi_direct_wrapper.h"
#include "../platform/android/wifi_direct_permission_manager.h"
#include "../tests/mocks/mock_jni_env.h"

using namespace Core::Multiplayer::ModelB::Android;
using namespace Core::Multiplayer;

int main() {
    std::cout << "Running Android Wi-Fi Direct implementation test..." << std::endl;
    
    try {
        // Test permission manager with mock
        std::cout << "\nTesting Permission Manager..." << std::endl;
        
        MockAndroidContext mock_context;
        MockPermissionHandler mock_handler;
        
        WiFiDirectPermissionManager perm_manager;
        auto result = perm_manager.Initialize(&mock_context, &mock_handler);
        assert(result == ErrorCode::Success);
        
        // Test API level detection
        int api_level = perm_manager.GetApiLevel();
        std::cout << "API Level: " << api_level << std::endl;
        assert(api_level > 0);
        
        // Test required permissions
        auto required_perms = perm_manager.GetRequiredPermissions();
        assert(!required_perms.empty());
        std::cout << "Required permissions: " << required_perms.size() << std::endl;
        
        // Check permission based on API level
        if (api_level >= 33) {
            // Android 13+ should require NEARBY_WIFI_DEVICES
            bool found_nearby = false;
            for (const auto& perm : required_perms) {
                if (perm == PermissionConstants::NEARBY_WIFI_DEVICES) {
                    found_nearby = true;
                    break;
                }
            }
            assert(found_nearby);
            std::cout << "✓ Android 13+ uses NEARBY_WIFI_DEVICES" << std::endl;
        } else {
            // Android 12- should require ACCESS_FINE_LOCATION
            bool found_location = false;
            for (const auto& perm : required_perms) {
                if (perm == PermissionConstants::ACCESS_FINE_LOCATION) {
                    found_location = true;
                    break;
                }
            }
            assert(found_location);
            std::cout << "✓ Android 12- uses ACCESS_FINE_LOCATION" << std::endl;
        }
        
        // Test Wi-Fi Direct wrapper with mock
        std::cout << "\nTesting Wi-Fi Direct Wrapper..." << std::endl;
        
        MockJNIEnv mock_env;
        WiFiDirectWrapper wrapper;
        
        result = wrapper.Initialize(&mock_env, &mock_context);
        assert(result == ErrorCode::Success);
        assert(wrapper.IsInitialized());
        assert(wrapper.GetState() == WifiDirectState::Initialized);
        std::cout << "✓ Wrapper initialized successfully" << std::endl;
        
        // Test discovery
        result = wrapper.StartDiscovery([](const std::vector<WifiP2pDevice>& peers) {
            std::cout << "Discovery callback called with " << peers.size() << " peers" << std::endl;
        });
        assert(result == ErrorCode::Success);
        std::cout << "✓ Discovery started successfully" << std::endl;
        
        // Test group creation
        result = wrapper.CreateGroup();
        assert(result == ErrorCode::Success);
        std::cout << "✓ Group creation successful" << std::endl;
        
        // Test shutdown
        wrapper.Shutdown();
        assert(!wrapper.IsInitialized());
        assert(wrapper.GetState() == WifiDirectState::Uninitialized);
        std::cout << "✓ Shutdown successful" << std::endl;
        
        std::cout << "\nAll Android Wi-Fi Direct tests PASSED!" << std::endl;
        std::cout << "✓ Permission manager works correctly" << std::endl;
        std::cout << "✓ Wi-Fi Direct wrapper works correctly" << std::endl;
        std::cout << "✓ Mock JNI environment works" << std::endl;
        std::cout << "✓ API version handling works" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}