// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>

#include "mocks/mock_jni_env.h"
#include "core/multiplayer/model_b/platform/android/wifi_direct_permission_manager.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelB::Testing;
using namespace Core::Multiplayer::ModelB::Android;

namespace {

class WiFiDirectPermissionTest : public Test {
protected:
    void SetUp() override {
        mock_env = std::make_unique<MockJNIEnv>();
        mock_context = std::make_unique<MockAndroidContext>();
        ON_CALL(*mock_context, GetSDKVersion()).WillByDefault(Return(33));
    }

    void CreateManager() {
        manager = std::make_unique<WiFiDirectPermissionManager>();
        ASSERT_EQ(manager->Initialize(mock_context.get(), nullptr), ErrorCode::Success);
    }

    std::unique_ptr<MockJNIEnv> mock_env;
    std::unique_ptr<MockAndroidContext> mock_context;
    std::unique_ptr<WiFiDirectPermissionManager> manager;
};

class WiFiDirectPermissionAndroid13Test : public WiFiDirectPermissionTest {
protected:
    void SetUp() override {
        WiFiDirectPermissionTest::SetUp();
        ON_CALL(*mock_context, GetSDKVersion()).WillByDefault(Return(33));
    }
};

class WiFiDirectPermissionAndroid12Test : public WiFiDirectPermissionTest {
protected:
    void SetUp() override {
        WiFiDirectPermissionTest::SetUp();
        ON_CALL(*mock_context, GetSDKVersion()).WillByDefault(Return(31));
    }
};

TEST_F(WiFiDirectPermissionAndroid13Test, RequiresNearbyWifiDevicesPermission) {
    CreateManager();
    auto perms = manager->GetRequiredPermissions();
    EXPECT_THAT(perms, Contains(PermissionConstants::NEARBY_WIFI_DEVICES));
    EXPECT_THAT(perms, Not(Contains(PermissionConstants::ACCESS_FINE_LOCATION)));
}

TEST_F(WiFiDirectPermissionAndroid13Test, ChecksPermissionStatus) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(false));
    EXPECT_CALL(*mock_context, CheckSelfPermission(PermissionConstants::NEARBY_WIFI_DEVICES))
        .WillOnce(Return(true));
    CreateManager();
    EXPECT_TRUE(manager->IsPermissionGranted(PermissionConstants::NEARBY_WIFI_DEVICES));
}

TEST_F(WiFiDirectPermissionAndroid13Test, HasAllPermissionsTrueWhenGranted) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(true));
    CreateManager();
    EXPECT_TRUE(manager->AreAllPermissionsGranted());
}

TEST_F(WiFiDirectPermissionAndroid13Test, HasAllPermissionsFalseWhenMissing) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(true));
    EXPECT_CALL(*mock_context, CheckSelfPermission(PermissionConstants::NEARBY_WIFI_DEVICES))
        .WillOnce(Return(false));
    CreateManager();
    EXPECT_FALSE(manager->AreAllPermissionsGranted());
}

TEST_F(WiFiDirectPermissionAndroid13Test, RequestPermissionsInvokesCallback) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(true));
    CreateManager();
    std::vector<std::string> perms{PermissionConstants::NEARBY_WIFI_DEVICES};
    std::vector<PermissionResult> results;
    manager->RequestPermissions(perms, [&](const std::vector<PermissionResult>& r) {
        results = r;
    });
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].permission, PermissionConstants::NEARBY_WIFI_DEVICES);
}

TEST_F(WiFiDirectPermissionAndroid12Test, RequiresLocationPermissions) {
    CreateManager();
    auto perms = manager->GetRequiredPermissions();
    EXPECT_THAT(perms, Contains(PermissionConstants::ACCESS_FINE_LOCATION));
    EXPECT_THAT(perms, Not(Contains(PermissionConstants::NEARBY_WIFI_DEVICES)));
}

TEST_F(WiFiDirectPermissionAndroid12Test, ChecksLocationPermissionStatus) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(false));
    CreateManager();
    EXPECT_FALSE(manager->IsPermissionGranted(PermissionConstants::ACCESS_FINE_LOCATION));
}

TEST_F(WiFiDirectPermissionAndroid12Test, MissingFineLocationFailsPermissionCheck) {
    ON_CALL(*mock_context, CheckSelfPermission(_)).WillByDefault(Return(true));
    EXPECT_CALL(*mock_context, CheckSelfPermission(PermissionConstants::ACCESS_FINE_LOCATION))
        .WillOnce(Return(false));
    CreateManager();
    EXPECT_FALSE(manager->AreAllPermissionsGranted());
}

TEST(PermissionConstantsTest, DefinesCorrectPermissionStrings) {
    EXPECT_STREQ(PermissionConstants::NEARBY_WIFI_DEVICES, "android.permission.NEARBY_WIFI_DEVICES");
    EXPECT_STREQ(PermissionConstants::ACCESS_FINE_LOCATION, "android.permission.ACCESS_FINE_LOCATION");
    EXPECT_STREQ(PermissionConstants::ACCESS_WIFI_STATE, "android.permission.ACCESS_WIFI_STATE");
    EXPECT_STREQ(PermissionConstants::CHANGE_WIFI_STATE, "android.permission.CHANGE_WIFI_STATE");
    EXPECT_STREQ(PermissionConstants::INTERNET, "android.permission.INTERNET");
}

} // namespace

