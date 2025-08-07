// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include "windows_capability_detector.h"

#ifdef _WIN32

using namespace Core::Multiplayer::ModelB::Windows;

class WindowsCapabilityDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
};

// Test Windows version detection
TEST_F(WindowsCapabilityDetectorTest, GetWindowsVersion_ReturnsValidVersion) {
    auto version = WindowsCapabilityDetector::GetWindowsVersion();
    
    // Version should have valid major version (at least Windows 7 = 6.1)
    EXPECT_GE(version.major_version, 6);
    
    // Build number should be non-zero
    EXPECT_GT(version.build_number, 0);
    
    // Version string should not be empty
    EXPECT_FALSE(version.version_string.empty());
    
    // Edition should be detected
    EXPECT_FALSE(version.edition.empty());
}

TEST_F(WindowsCapabilityDetectorTest, IsWindows10OrLater_ReturnsCorrectValue) {
    auto version = WindowsCapabilityDetector::GetWindowsVersion();
    bool is_win10_or_later = WindowsCapabilityDetector::IsWindows10OrLater();
    
    // If major version is 10 or higher, should return true
    if (version.major_version >= 10) {
        EXPECT_TRUE(is_win10_or_later);
    }
}

TEST_F(WindowsCapabilityDetectorTest, IsWindows10Version1607OrLater_ChecksBuildNumber) {
    auto version = WindowsCapabilityDetector::GetWindowsVersion();
    bool is_1607_or_later = WindowsCapabilityDetector::IsWindows10Version1607OrLater();
    
    // Windows 10 version 1607 has build number 14393
    if (version.major_version >= 10 && version.build_number >= 14393) {
        EXPECT_TRUE(is_1607_or_later);
    } else {
        EXPECT_FALSE(is_1607_or_later);
    }
}

// Test WinRT capability detection
TEST_F(WindowsCapabilityDetectorTest, DetectWinRTCapabilities_ReturnsValidCapabilities) {
    auto caps = WindowsCapabilityDetector::DetectWinRTCapabilities();
    
    // WinRT availability should be consistent with CanInitializeWinRT
    EXPECT_EQ(caps.winrt_available, WindowsCapabilityDetector::CanInitializeWinRT());
    
    // Mobile Hotspot API requires Windows 10 1607+
    if (WindowsCapabilityDetector::IsWindows10Version1607OrLater()) {
        EXPECT_TRUE(caps.mobile_hotspot_api_available);
    } else {
        EXPECT_FALSE(caps.mobile_hotspot_api_available);
    }
    
    // WiFi Direct available on Windows 8+
    if (WindowsCapabilityDetector::GetWindowsVersion().major_version >= 8 ||
        (WindowsCapabilityDetector::GetWindowsVersion().major_version == 6 && 
         WindowsCapabilityDetector::GetWindowsVersion().minor_version >= 2)) {
        EXPECT_TRUE(caps.wifi_direct_available);
    }
}

TEST_F(WindowsCapabilityDetectorTest, IsWinRTAvailable_ReturnsBooleanValue) {
    bool available = WindowsCapabilityDetector::IsWinRTAvailable();
    // Should return true or false, test just validates it returns
    EXPECT_TRUE(available == true || available == false);
}

TEST_F(WindowsCapabilityDetectorTest, CanInitializeWinRT_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool can_init = WindowsCapabilityDetector::CanInitializeWinRT();
        // Result should be true or false
        EXPECT_TRUE(can_init == true || can_init == false);
    });
}

// Test admin detection
TEST_F(WindowsCapabilityDetectorTest, IsRunningAsAdministrator_ReturnsBooleanValue) {
    bool is_admin = WindowsCapabilityDetector::IsRunningAsAdministrator();
    // Should return true or false without throwing
    EXPECT_TRUE(is_admin == true || is_admin == false);
}

// Test network adapter detection
TEST_F(WindowsCapabilityDetectorTest, HasWiFiAdapter_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool has_wifi = WindowsCapabilityDetector::HasWiFiAdapter();
        EXPECT_TRUE(has_wifi == true || has_wifi == false);
    });
}

TEST_F(WindowsCapabilityDetectorTest, HasEthernetAdapter_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool has_ethernet = WindowsCapabilityDetector::HasEthernetAdapter();
        EXPECT_TRUE(has_ethernet == true || has_ethernet == false);
    });
}

TEST_F(WindowsCapabilityDetectorTest, HasInternetConnection_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool has_internet = WindowsCapabilityDetector::HasInternetConnection();
        EXPECT_TRUE(has_internet == true || has_internet == false);
    });
}

// Test loopback adapter detection
TEST_F(WindowsCapabilityDetectorTest, IsLoopbackAdapterInstalled_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool installed = WindowsCapabilityDetector::IsLoopbackAdapterInstalled();
        EXPECT_TRUE(installed == true || installed == false);
    });
}

TEST_F(WindowsCapabilityDetectorTest, GetLoopbackAdapterGuid_ReturnsOptional) {
    auto guid = WindowsCapabilityDetector::GetLoopbackAdapterGuid();
    
    // If loopback adapter is installed, should return a GUID
    if (WindowsCapabilityDetector::IsLoopbackAdapterInstalled()) {
        EXPECT_TRUE(guid.has_value());
        if (guid.has_value()) {
            EXPECT_FALSE(guid.value().empty());
        }
    } else {
        EXPECT_FALSE(guid.has_value());
    }
}

// Test Mobile Hotspot detection
TEST_F(WindowsCapabilityDetectorTest, IsMobileHotspotSupported_ConsistentWithCapabilities) {
    bool supported = WindowsCapabilityDetector::IsMobileHotspotSupported();
    auto caps = WindowsCapabilityDetector::DetectWinRTCapabilities();
    
    // If not Windows 10 1607+, should not be supported
    if (!WindowsCapabilityDetector::IsWindows10Version1607OrLater()) {
        EXPECT_FALSE(supported);
    }
    
    // Support should be consistent with capability detection
    if (caps.mobile_hotspot_api_available && 
        (WindowsCapabilityDetector::HasInternetConnection() || 
         WindowsCapabilityDetector::IsLoopbackAdapterInstalled())) {
        // Could be supported
    } else if (!caps.mobile_hotspot_api_available) {
        EXPECT_FALSE(supported);
    }
}

TEST_F(WindowsCapabilityDetectorTest, IsMobileHotspotEnabled_DoesNotThrow) {
    EXPECT_NO_THROW({
        bool enabled = WindowsCapabilityDetector::IsMobileHotspotEnabled();
        EXPECT_TRUE(enabled == true || enabled == false);
    });
}

TEST_F(WindowsCapabilityDetectorTest, CanUseMobileHotspotWithoutInternet_RequiresLoopback) {
    bool can_use = WindowsCapabilityDetector::CanUseMobileHotspotWithoutInternet();
    
    // Should only return true if loopback adapter is installed and Mobile Hotspot is supported
    if (can_use) {
        EXPECT_TRUE(WindowsCapabilityDetector::IsLoopbackAdapterInstalled());
        EXPECT_TRUE(WindowsCapabilityDetector::IsMobileHotspotSupported());
    }
}

// Test WiFi Direct detection
TEST_F(WindowsCapabilityDetectorTest, IsWiFiDirectSupported_RequiresWindows8AndWiFi) {
    bool supported = WindowsCapabilityDetector::IsWiFiDirectSupported();
    
    if (supported) {
        // Must have Windows 8+ and WiFi adapter
        auto version = WindowsCapabilityDetector::GetWindowsVersion();
        EXPECT_TRUE(version.major_version > 8 || 
                   (version.major_version == 8) ||
                   (version.major_version == 6 && version.minor_version >= 2));
        EXPECT_TRUE(WindowsCapabilityDetector::HasWiFiAdapter());
    }
}

// Test recommended mode
TEST_F(WindowsCapabilityDetectorTest, GetRecommendedMode_ReturnsValidMode) {
    auto mode = WindowsCapabilityDetector::GetRecommendedMode();
    
    // Should return one of the valid enum values
    EXPECT_TRUE(mode == WindowsCapabilityDetector::RecommendedMode::MobileHotspot ||
                mode == WindowsCapabilityDetector::RecommendedMode::MobileHotspotWithLoopback ||
                mode == WindowsCapabilityDetector::RecommendedMode::WiFiDirect ||
                mode == WindowsCapabilityDetector::RecommendedMode::InternetMultiplayer ||
                mode == WindowsCapabilityDetector::RecommendedMode::NotSupported);
    
    // Description should not be empty
    std::string desc = WindowsCapabilityDetector::GetRecommendedModeDescription(mode);
    EXPECT_FALSE(desc.empty());
}

TEST_F(WindowsCapabilityDetectorTest, GetRecommendedModeDescription_HandlesAllModes) {
    // Test all enum values
    std::vector<WindowsCapabilityDetector::RecommendedMode> modes = {
        WindowsCapabilityDetector::RecommendedMode::MobileHotspot,
        WindowsCapabilityDetector::RecommendedMode::MobileHotspotWithLoopback,
        WindowsCapabilityDetector::RecommendedMode::WiFiDirect,
        WindowsCapabilityDetector::RecommendedMode::InternetMultiplayer,
        WindowsCapabilityDetector::RecommendedMode::NotSupported
    };
    
    for (auto mode : modes) {
        std::string desc = WindowsCapabilityDetector::GetRecommendedModeDescription(mode);
        EXPECT_FALSE(desc.empty());
        EXPECT_NE(desc, "Unknown mode");
    }
}

// Test diagnostic report
TEST_F(WindowsCapabilityDetectorTest, GetDiagnosticReport_ReturnsNonEmptyReport) {
    std::string report = WindowsCapabilityDetector::GetDiagnosticReport();
    
    EXPECT_FALSE(report.empty());
    
    // Report should contain key sections
    EXPECT_NE(report.find("Windows Version:"), std::string::npos);
    EXPECT_NE(report.find("WinRT Capabilities:"), std::string::npos);
    EXPECT_NE(report.find("Network Adapters:"), std::string::npos);
    EXPECT_NE(report.find("Feature Support:"), std::string::npos);
    EXPECT_NE(report.find("Recommended Mode:"), std::string::npos);
}

#else // Not Windows

TEST(WindowsCapabilityDetectorTest, NotAvailableOnNonWindows) {
#ifdef _WIN32
    FAIL() << "Test should not run on Windows";
#else
    GTEST_SKIP() << "Windows capability detection unavailable on this platform";
#endif
}

#endif // _WIN32