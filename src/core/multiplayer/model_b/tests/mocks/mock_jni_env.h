// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace Core::Multiplayer::ModelB::Testing {

/**
 * Mock JNI Environment for testing Android Wi-Fi Direct wrapper
 * 
 * This mock simulates the JNI interface without requiring actual Android runtime.
 * It provides controllable behavior for testing various Android API scenarios.
 */
class MockJNIEnv {
public:
    // Core JNI operations
    MOCK_METHOD(jclass, FindClass, (const char* name), ());
    MOCK_METHOD(jmethodID, GetMethodID, (jclass clazz, const char* name, const char* sig), ());
    MOCK_METHOD(jmethodID, GetStaticMethodID, (jclass clazz, const char* name, const char* sig), ());
    MOCK_METHOD(jfieldID, GetFieldID, (jclass clazz, const char* name, const char* sig), ());
    MOCK_METHOD(jfieldID, GetStaticFieldID, (jclass clazz, const char* name, const char* sig), ());
    
    // Object operations
    MOCK_METHOD(jobject, NewObject, (jclass clazz, jmethodID methodID, ...), ());
    MOCK_METHOD(jobject, GetObjectField, (jobject obj, jfieldID fieldID), ());
    MOCK_METHOD(void, SetObjectField, (jobject obj, jfieldID fieldID, jobject value), ());
    MOCK_METHOD(jobject, CallObjectMethod, (jobject obj, jmethodID methodID, ...), ());
    MOCK_METHOD(jobject, CallStaticObjectMethod, (jclass clazz, jmethodID methodID, ...), ());
    
    // Boolean operations
    MOCK_METHOD(jboolean, GetBooleanField, (jobject obj, jfieldID fieldID), ());
    MOCK_METHOD(void, SetBooleanField, (jobject obj, jfieldID fieldID, jboolean value), ());
    MOCK_METHOD(jboolean, CallBooleanMethod, (jobject obj, jmethodID methodID, ...), ());
    MOCK_METHOD(jboolean, CallStaticBooleanMethod, (jclass clazz, jmethodID methodID, ...), ());
    
    // Integer operations
    MOCK_METHOD(jint, GetIntField, (jobject obj, jfieldID fieldID), ());
    MOCK_METHOD(void, SetIntField, (jobject obj, jfieldID fieldID, jint value), ());
    MOCK_METHOD(jint, CallIntMethod, (jobject obj, jmethodID methodID, ...), ());
    MOCK_METHOD(jint, CallStaticIntMethod, (jclass clazz, jmethodID methodID, ...), ());
    
    // String operations
    MOCK_METHOD(jstring, NewStringUTF, (const char* utf), ());
    MOCK_METHOD(const char*, GetStringUTFChars, (jstring str, jboolean* isCopy), ());
    MOCK_METHOD(void, ReleaseStringUTFChars, (jstring str, const char* chars), ());
    MOCK_METHOD(jsize, GetStringUTFLength, (jstring str), ());
    
    // Array operations
    MOCK_METHOD(jobjectArray, NewObjectArray, (jsize len, jclass clazz, jobject init), ());
    MOCK_METHOD(jobject, GetObjectArrayElement, (jobjectArray array, jsize index), ());
    MOCK_METHOD(void, SetObjectArrayElement, (jobjectArray array, jsize index, jobject val), ());
    MOCK_METHOD(jsize, GetArrayLength, (jarray array), ());
    
    // Exception handling
    MOCK_METHOD(jthrowable, ExceptionOccurred, (), ());
    MOCK_METHOD(void, ExceptionClear, (), ());
    MOCK_METHOD(void, ExceptionDescribe, (), ());
    MOCK_METHOD(jint, ThrowNew, (jclass clazz, const char* msg), ());
    
    // Reference management
    MOCK_METHOD(jobject, NewGlobalRef, (jobject lobj), ());
    MOCK_METHOD(void, DeleteGlobalRef, (jobject gref), ());
    MOCK_METHOD(jobject, NewLocalRef, (jobject ref), ());
    MOCK_METHOD(void, DeleteLocalRef, (jobject obj), ());
    
    // Helper methods for test setup
    void SetupWifiP2pManagerMock();
    void SetupPermissionCheckMocks();
    void SetupPeerDiscoveryMocks();
    void SetupGroupCreationMocks();
    void SetupConnectionMocks();
    void SetupExceptionScenarios();
    
    // Test data management
    void AddMockPeer(const std::string& device_name, const std::string& device_address);
    void SetPermissionGranted(const std::string& permission, bool granted);
    void SetAPILevel(int api_level);
    void SimulateNetworkError(bool should_error);
    void SimulateDiscoveryFailure(bool should_fail);
    
private:
    std::vector<std::pair<std::string, std::string>> mock_peers_;
    std::unordered_map<std::string, bool> mock_permissions_;
    int mock_api_level_ = 33; // Default to Android 13
    bool simulate_network_error_ = false;
    bool simulate_discovery_failure_ = false;
};

/**
 * Mock Android Context for permission and system service access
 */
class MockAndroidContext {
public:
    MOCK_METHOD(jobject, GetSystemService, (const std::string& service_name), ());
    MOCK_METHOD(bool, CheckSelfPermission, (const std::string& permission), ());
    MOCK_METHOD(int, GetSDKVersion, (), ());
    MOCK_METHOD(std::string, GetPackageName, (), ());
    MOCK_METHOD(void, StartActivity, (jobject intent), ());
    MOCK_METHOD(void, RegisterReceiver, (jobject receiver, jobject filter), ());
    MOCK_METHOD(void, UnregisterReceiver, (jobject receiver), ());
};

/**
 * Mock WifiP2pManager for testing Wi-Fi Direct operations
 */
class MockWifiP2pManager {
public:
    // Core Wi-Fi Direct operations
    MOCK_METHOD(void, Initialize, (jobject context, jobject looper, jobject listener), ());
    MOCK_METHOD(void, DiscoverPeers, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, StopPeerDiscovery, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, RequestPeers, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, Connect, (jobject channel, jobject config, jobject listener), ());
    MOCK_METHOD(void, CancelConnect, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, CreateGroup, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, RemoveGroup, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, RequestConnectionInfo, (jobject channel, jobject listener), ());
    MOCK_METHOD(void, RequestGroupInfo, (jobject channel, jobject listener), ());
    
    // Test helpers
    void SimulateDiscoveryResult(const std::vector<std::pair<std::string, std::string>>& peers);
    void SimulateConnectionResult(bool success, const std::string& group_owner_address = "");
    void SimulateGroupCreation(bool success, const std::string& group_name = "");
    void SimulatePermissionError();
    void SimulateNetworkError();
};

/**
 * Mock Wi-Fi P2P Device for peer representation
 */
class MockWifiP2pDevice {
public:
    MOCK_METHOD(std::string, GetDeviceName, (), (const));
    MOCK_METHOD(std::string, GetDeviceAddress, (), (const));
    MOCK_METHOD(int, GetStatus, (), (const));
    MOCK_METHOD(bool, IsGroupOwner, (), (const));
    MOCK_METHOD(bool, IsServiceDiscoveryCapable, (), (const));
    MOCK_METHOD(int, GetWpsConfigMethods, (), (const));
};

/**
 * Mock Wi-Fi P2P Group Info for connection details
 */
class MockWifiP2pInfo {
public:
    MOCK_METHOD(bool, IsGroupOwner, (), (const));
    MOCK_METHOD(std::string, GetGroupOwnerAddress, (), (const));
    MOCK_METHOD(bool, IsGroupFormed, (), (const));
};

/**
 * Mock Wi-Fi P2P Group for group management
 */
class MockWifiP2pGroup {
public:
    MOCK_METHOD(std::string, GetNetworkName, (), (const));
    MOCK_METHOD(std::string, GetPassphrase, (), (const));
    MOCK_METHOD(jobject, GetOwner, (), (const));
    MOCK_METHOD(std::vector<jobject>, GetClientList, (), (const));
    MOCK_METHOD(bool, IsGroupOwner, (), (const));
    MOCK_METHOD(std::string, GetInterface, (), (const));
};

} // namespace Core::Multiplayer::ModelB::Testing
