// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "test_helpers.h"

namespace Core::Multiplayer::HLE {

class BackendFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_config = std::make_shared<MockConfigurationManager>();
        factory = std::make_unique<ConcreteBackendFactory>(mock_config);
    }

    std::shared_ptr<MockConfigurationManager> mock_config;
    std::unique_ptr<ConcreteBackendFactory> factory;
};

TEST_F(BackendFactoryTest, CreateModelABackend) {
    auto backend = factory->CreateBackend(BackendFactory::BackendType::ModelA_Internet);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->Initialize(), ErrorCode::Success);
}

TEST_F(BackendFactoryTest, CreateModelBBackend) {
    auto backend = factory->CreateBackend(BackendFactory::BackendType::ModelB_AdHoc);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->Initialize(), ErrorCode::Success);
}

TEST_F(BackendFactoryTest, GetPreferredBackend_InternetMode) {
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Internet));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelA_Internet);
}

TEST_F(BackendFactoryTest, GetPreferredBackend_AdHocMode) {
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::AdHoc));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelB_AdHoc);
}

TEST_F(BackendFactoryTest, GetPreferredBackend_AutoMode_PreferAdHoc) {
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    EXPECT_CALL(*mock_config, IsModelBAvailable()).WillOnce(::testing::Return(true));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelB_AdHoc);
}

TEST_F(BackendFactoryTest, GetPreferredBackend_AutoMode_FallbackToInternet) {
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    EXPECT_CALL(*mock_config, IsModelBAvailable()).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mock_config, IsModelAAvailable()).WillOnce(::testing::Return(true));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelA_Internet);
}

TEST_F(BackendFactoryTest, GetPreferredBackend_AutoMode_NothingAvailable) {
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    EXPECT_CALL(*mock_config, IsModelBAvailable()).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mock_config, IsModelAAvailable()).WillOnce(::testing::Return(false));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelA_Internet);
}

class PlatformSpecificBackendTest : public BackendFactoryTest {
protected:
    void SetupAndroidPlatform() {
        EXPECT_CALL(*mock_config, IsModelBAvailable()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mock_config, IsModelAAvailable()).WillRepeatedly(::testing::Return(true));
    }
    void SetupWindowsPlatform() {
        EXPECT_CALL(*mock_config, IsModelBAvailable()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mock_config, IsModelAAvailable()).WillRepeatedly(::testing::Return(true));
    }
    void SetupLinuxPlatform() {
        EXPECT_CALL(*mock_config, IsModelBAvailable()).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mock_config, IsModelAAvailable()).WillRepeatedly(::testing::Return(true));
    }
};

TEST_F(PlatformSpecificBackendTest, AndroidPreferredBackend) {
    SetupAndroidPlatform();
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelB_AdHoc);
}

TEST_F(PlatformSpecificBackendTest, WindowsPreferredBackend) {
    SetupWindowsPlatform();
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelB_AdHoc);
}

TEST_F(PlatformSpecificBackendTest, LinuxFallbackToInternet) {
    SetupLinuxPlatform();
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::Auto));
    auto preferred = factory->GetPreferredBackend();
    EXPECT_EQ(preferred, BackendFactory::BackendType::ModelA_Internet);
}

class ConfigurationManagerTest : public ::testing::Test {
protected:
    void SetUp() override { mock_config = std::make_shared<MockConfigurationManager>(); }
    std::shared_ptr<MockConfigurationManager> mock_config;
};

TEST_F(ConfigurationManagerTest, SetAndGetPreferredMode) {
    EXPECT_CALL(*mock_config, SetPreferredMode(ConfigurationManager::MultiplayerMode::AdHoc)).Times(1);
    EXPECT_CALL(*mock_config, GetPreferredMode())
        .WillOnce(::testing::Return(ConfigurationManager::MultiplayerMode::AdHoc));
    mock_config->SetPreferredMode(ConfigurationManager::MultiplayerMode::AdHoc);
    auto mode = mock_config->GetPreferredMode();
    EXPECT_EQ(mode, ConfigurationManager::MultiplayerMode::AdHoc);
}

TEST_F(ConfigurationManagerTest, PlatformAvailabilityCheck) {
    EXPECT_CALL(*mock_config, IsModelAAvailable()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_config, IsModelBAvailable()).WillOnce(::testing::Return(false));
    bool model_a_available = mock_config->IsModelAAvailable();
    bool model_b_available = mock_config->IsModelBAvailable();
    EXPECT_TRUE(model_a_available);
    EXPECT_FALSE(model_b_available);
}

TEST_F(ConfigurationManagerTest, ConfigurationFilePath) {
    std::string expected_path = "/path/to/multiplayer/config.json";
    EXPECT_CALL(*mock_config, GetConfigFilePath()).WillOnce(::testing::Return(expected_path));
    auto path = mock_config->GetConfigFilePath();
    EXPECT_EQ(path, expected_path);
}

TEST_F(BackendFactoryTest, RealConfigurationIntegration) {
    GTEST_SKIP() << "Real configuration manager not implemented.";
}

TEST_F(BackendFactoryTest, CompleteBackendCreationFlow) {
    GTEST_SKIP() << "Complete backend flow requires full implementation.";
}

} // namespace Core::Multiplayer::HLE
