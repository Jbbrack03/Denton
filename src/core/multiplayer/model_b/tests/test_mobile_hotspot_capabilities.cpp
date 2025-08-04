// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>
#include <map>

// Mock Windows APIs for testing
#include "mocks/mock_winrt_apis.h"

// Header for the Mobile Hotspot Capabilities detector
#include "core/multiplayer/model_b/platform/windows/mobile_hotspot_capabilities.h"

// Common multiplayer components
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelB::Testing;

namespace {

// Using types from the header file
using namespace Core::Multiplayer::ModelB::Windows;

/**
 * Test data for Windows version compatibility
 */
struct WindowsVersionTestCase {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t build_number;
    std::string version_name;
    bool supports_hotspot;
    bool supports_wifi_direct;
    TetheringSupport expected_support;
};

/**
 * Test fixture for Mobile Hotspot Capabilities testing
 */
class MobileHotspotCapabilitiesTest : public Test {
protected:
    void SetUp() override {
        mock_winrt_ = std::make_unique<MockWinRTAPIs>();
        mock_registry_ = std::make_unique<MockWindowsRegistry>();
        mock_wmi_ = std::make_unique<MockWMIProvider>();
        
        // Set up default mock behavior
        SetupDefaultMocks();
    }
    
    void TearDown() override {
        // Clean up resources
    }
    
    void SetupDefaultMocks() {
        // Default to Windows 10 1607 (supported)
        mock_winrt_->SetWindowsVersion(10, 0, 14393);
        
        // Default registry values for hotspot support
        EXPECT_CALL(*mock_registry_, ReadDWORD(_, "SharingEnabled", _))
            .WillRepeatedly(DoAll(SetArgReferee<2>(1), Return(true)));
        
        // Default WMI queries for hardware detection
        EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
            .WillRepeatedly(Return(std::vector<WiFiAdapterInfo>{{
                "Intel Wireless-AC 9560",
                "Intel",
                "23.20.0.3",
                true, // supports_hosted_network
                true, // supports_wifi_direct
                {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
            }}));
    }
    
    void CreateCapabilities() {
        capabilities_ = std::make_unique<MobileHotspotCapabilities>();
    }
    
    std::vector<WindowsVersionTestCase> GetWindowsVersionTestCases() {
        return {
            {10, 0, 14393, "Windows 10 1607", true, true, TetheringSupport::Full},
            {10, 0, 15063, "Windows 10 1703", true, true, TetheringSupport::Full},
            {10, 0, 16299, "Windows 10 1709", true, true, TetheringSupport::Full},
            {10, 0, 17134, "Windows 10 1803", true, true, TetheringSupport::Full},
            {10, 0, 17763, "Windows 10 1809", true, true, TetheringSupport::Full},
            {10, 0, 18362, "Windows 10 1903", true, true, TetheringSupport::Full},
            {10, 0, 19041, "Windows 10 2004", true, true, TetheringSupport::Full},
            {11, 0, 22000, "Windows 11", true, true, TetheringSupport::Full},
            {10, 0, 10240, "Windows 10 RTM", false, false, TetheringSupport::None},
            {10, 0, 10586, "Windows 10 1511", false, false, TetheringSupport::None},
            {6, 3, 9600, "Windows 8.1", false, false, TetheringSupport::None},
            {6, 1, 7601, "Windows 7 SP1", false, false, TetheringSupport::None}
        };
    }
    
    std::unique_ptr<MockWinRTAPIs> mock_winrt_;
    std::unique_ptr<MockWindowsRegistry> mock_registry_;
    std::unique_ptr<MockWMIProvider> mock_wmi_;
    std::unique_ptr<MobileHotspotCapabilities> capabilities_;
};

/**
 * Test class for Windows version detection
 */
class MobileHotspotCapabilitiesVersionTest : public MobileHotspotCapabilitiesTest {};

/**
 * Test class for hardware capability detection
 */
class MobileHotspotCapabilitiesHardwareTest : public MobileHotspotCapabilitiesTest {};

/**
 * Test class for policy and configuration detection
 */
class MobileHotspotCapabilitiesPolicyTest : public MobileHotspotCapabilitiesTest {};

/**
 * Test class for network adapter analysis
 */
class MobileHotspotCapabilitiesNetworkTest : public MobileHotspotCapabilitiesTest {};

/**
 * Test class for fallback capability detection
 */
class MobileHotspotCapabilitiesFallbackTest : public MobileHotspotCapabilitiesTest {};

// ============================================================================
// Windows Version Detection Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesVersionTest, DetectsWindowsVersionCorrectly) {
    CreateCapabilities();
    
    auto test_cases = GetWindowsVersionTestCases();
    
    for (const auto& test_case : test_cases) {
        mock_winrt_->SetWindowsVersion(test_case.major_version, test_case.minor_version, test_case.build_number);
        
        auto version_info = capabilities_->GetWindowsVersionInfo();
        EXPECT_EQ(version_info.major_version, test_case.major_version) << test_case.version_name;
        EXPECT_EQ(version_info.minor_version, test_case.minor_version) << test_case.version_name;
        EXPECT_EQ(version_info.build_number, test_case.build_number) << test_case.version_name;
        EXPECT_EQ(version_info.version_name, test_case.version_name) << test_case.version_name;
    }
    
    FAIL() << "MobileHotspotCapabilities::GetWindowsVersionInfo not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesVersionTest, DeterminesHotspotSupportByVersion) {
    CreateCapabilities();
    
    auto test_cases = GetWindowsVersionTestCases();
    
    for (const auto& test_case : test_cases) {
        mock_winrt_->SetWindowsVersion(test_case.major_version, test_case.minor_version, test_case.build_number);
        
        bool supports_hotspot = capabilities_->IsHotspotSupportedByVersion();
        EXPECT_EQ(supports_hotspot, test_case.supports_hotspot) << test_case.version_name;
    }
    
    FAIL() << "MobileHotspotCapabilities::IsHotspotSupportedByVersion not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesVersionTest, DeterminesWiFiDirectSupportByVersion) {
    CreateCapabilities();
    
    auto test_cases = GetWindowsVersionTestCases();
    
    for (const auto& test_case : test_cases) {
        mock_winrt_->SetWindowsVersion(test_case.major_version, test_case.minor_version, test_case.build_number);
        
        bool supports_wifi_direct = capabilities_->IsWiFiDirectSupportedByVersion();
        EXPECT_EQ(supports_wifi_direct, test_case.supports_wifi_direct) << test_case.version_name;
    }
    
    FAIL() << "MobileHotspotCapabilities::IsWiFiDirectSupportedByVersion not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesVersionTest, HandlesUnknownWindowsVersions) {
    CreateCapabilities();
    
    // Test with future Windows version
    mock_winrt_->SetWindowsVersion(12, 0, 30000);
    
    auto version_info = capabilities_->GetWindowsVersionInfo();
    EXPECT_EQ(version_info.major_version, 12);
    EXPECT_EQ(version_info.version_name, "Unknown Windows Version");
    
    // Should assume support for newer versions
    bool supports_hotspot = capabilities_->IsHotspotSupportedByVersion();
    EXPECT_TRUE(supports_hotspot);
    
    FAIL() << "MobileHotspotCapabilities unknown version handling not implemented - TDD red phase";
}

// ============================================================================
// Hardware Capability Detection Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesHardwareTest, DetectsWiFiAdaptersCorrectly) {
    CreateCapabilities();
    
    // Mock multiple WiFi adapters
    std::vector<WiFiAdapterInfo> mock_adapters = {
        {
            "Intel Wireless-AC 9560",
            "Intel",
            "23.20.0.3",
            true,  // supports_hosted_network
            true,  // supports_wifi_direct
            {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
        },
        {
            "Qualcomm Atheros QCA9377",
            "Qualcomm",
            "10.0.0.374",
            false, // supports_hosted_network
            true,  // supports_wifi_direct
            {WiFiBand::TwoPointFourGHz}
        },
        {
            "Realtek 8822BE",
            "Realtek",
            "2024.0.10.106",
            true,  // supports_hosted_network
            false, // supports_wifi_direct
            {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
        }
    };
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(mock_adapters));
    
    auto detected_adapters = capabilities_->GetWiFiAdapters();
    EXPECT_EQ(detected_adapters.size(), 3);
    
    // Check first adapter details
    EXPECT_EQ(detected_adapters[0].name, "Intel Wireless-AC 9560");
    EXPECT_TRUE(detected_adapters[0].supports_hosted_network);
    EXPECT_TRUE(detected_adapters[0].supports_wifi_direct);
    EXPECT_EQ(detected_adapters[0].supported_bands.size(), 2);
    
    FAIL() << "MobileHotspotCapabilities::GetWiFiAdapters not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesHardwareTest, DeterminesHotspotCapabilityFromHardware) {
    CreateCapabilities();
    
    // Test with hotspot-capable adapter
    std::vector<WiFiAdapterInfo> capable_adapters = {{
        "Intel Wireless-AC 9560", "Intel", "23.20.0.3",
        true, true, {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(capable_adapters));
    
    bool can_hotspot = capabilities_->CanCreateHotspot();
    EXPECT_TRUE(can_hotspot);
    
    // Test with incapable adapter
    std::vector<WiFiAdapterInfo> incapable_adapters = {{
        "Old WiFi Adapter", "Generic", "1.0.0.0",
        false, false, {WiFiBand::TwoPointFourGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(incapable_adapters));
    
    can_hotspot = capabilities_->CanCreateHotspot();
    EXPECT_FALSE(can_hotspot);
    
    FAIL() << "MobileHotspotCapabilities::CanCreateHotspot not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesHardwareTest, DeterminesWiFiDirectCapabilityFromHardware) {
    CreateCapabilities();
    
    // Test with WiFi Direct capable adapter
    std::vector<WiFiAdapterInfo> wifi_direct_adapters = {{
        "Modern WiFi Adapter", "Intel", "20.0.0.0",
        false, true, {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(wifi_direct_adapters));
    
    bool can_wifi_direct = capabilities_->CanUseWiFiDirect();
    EXPECT_TRUE(can_wifi_direct);
    
    FAIL() << "MobileHotspotCapabilities::CanUseWiFiDirect not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesHardwareTest, AnalyzesSupportedWiFiBands) {
    CreateCapabilities();
    
    std::vector<WiFiAdapterInfo> dual_band_adapters = {{
        "Dual Band Adapter", "Intel", "20.0.0.0",
        true, true, {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(dual_band_adapters));
    
    auto supported_bands = capabilities_->GetSupportedWiFiBands();
    EXPECT_THAT(supported_bands, Contains(WiFiBand::TwoPointFourGHz));
    EXPECT_THAT(supported_bands, Contains(WiFiBand::FiveGHz));
    
    // Test single band adapter
    std::vector<WiFiAdapterInfo> single_band_adapters = {{
        "Single Band Adapter", "Generic", "1.0.0.0",
        true, false, {WiFiBand::TwoPointFourGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(single_band_adapters));
    
    supported_bands = capabilities_->GetSupportedWiFiBands();
    EXPECT_THAT(supported_bands, Contains(WiFiBand::TwoPointFourGHz));
    EXPECT_THAT(supported_bands, Not(Contains(WiFiBand::FiveGHz)));
    
    FAIL() << "MobileHotspotCapabilities::GetSupportedWiFiBands not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesHardwareTest, HandlesNoWiFiAdapters) {
    CreateCapabilities();
    
    // Mock no WiFi adapters found
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(std::vector<WiFiAdapterInfo>{}));
    
    auto adapters = capabilities_->GetWiFiAdapters();
    EXPECT_TRUE(adapters.empty());
    
    bool can_hotspot = capabilities_->CanCreateHotspot();
    EXPECT_FALSE(can_hotspot);
    
    bool can_wifi_direct = capabilities_->CanUseWiFiDirect();
    EXPECT_FALSE(can_wifi_direct);
    
    FAIL() << "MobileHotspotCapabilities no adapter handling not implemented - TDD red phase";
}

// ============================================================================
// Policy and Configuration Detection Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesPolicyTest, ChecksGroupPolicyRestrictions) {
    CreateCapabilities();
    
    // Test with hotspot disabled by group policy
    EXPECT_CALL(*mock_registry_, ReadDWORD(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\Network Connections",
        "NC_ShowSharedAccessUI", _))
        .WillOnce(DoAll(SetArgReferee<2>(0), Return(true)));
    
    bool policy_allows = capabilities_->IsHotspotAllowedByPolicy();
    EXPECT_FALSE(policy_allows);
    
    // Test with policy allowing hotspot
    EXPECT_CALL(*mock_registry_, ReadDWORD(_, "NC_ShowSharedAccessUI", _))
        .WillOnce(DoAll(SetArgReferee<2>(1), Return(true)));
    
    policy_allows = capabilities_->IsHotspotAllowedByPolicy();
    EXPECT_TRUE(policy_allows);
    
    FAIL() << "MobileHotspotCapabilities::IsHotspotAllowedByPolicy not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesPolicyTest, ChecksICSServiceStatus) {
    CreateCapabilities();
    
    // Test with ICS service disabled
    EXPECT_CALL(*mock_winrt_, GetServiceStatus("SharedAccess"))
        .WillOnce(Return(ServiceStatus::Disabled));
    
    bool ics_available = capabilities_->IsICSServiceAvailable();
    EXPECT_FALSE(ics_available);
    
    // Test with ICS service running
    EXPECT_CALL(*mock_winrt_, GetServiceStatus("SharedAccess"))
        .WillOnce(Return(ServiceStatus::Running));
    
    ics_available = capabilities_->IsICSServiceAvailable();
    EXPECT_TRUE(ics_available);
    
    FAIL() << "MobileHotspotCapabilities::IsICSServiceAvailable not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesPolicyTest, ChecksWindowsEditionSupport) {
    CreateCapabilities();
    
    struct EditionTestCase {
        std::string edition;
        bool supports_hotspot;
        std::string description;
    };
    
    std::vector<EditionTestCase> test_cases = {
        {"Professional", true, "Windows 10/11 Pro"},
        {"Enterprise", true, "Windows 10/11 Enterprise"},
        {"Education", true, "Windows 10/11 Education"},
        {"Home", true, "Windows 10/11 Home"},
        {"Home Single Language", true, "Windows 10/11 Home Single Language"},
        {"Starter", false, "Windows Starter Edition"},
        {"Home Basic", false, "Windows Home Basic"}
    };
    
    for (const auto& test_case : test_cases) {
        EXPECT_CALL(*mock_registry_, ReadString(_, "EditionID", _))
            .WillOnce(DoAll(SetArgReferee<2>(test_case.edition), Return(true)));
        
        bool edition_supports = capabilities_->DoesEditionSupportHotspot();
        EXPECT_EQ(edition_supports, test_case.supports_hotspot) << test_case.description;
    }
    
    FAIL() << "MobileHotspotCapabilities::DoesEditionSupportHotspot not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesPolicyTest, ChecksMDMRestrictions) {
    CreateCapabilities();
    
    // Test with MDM policy restricting hotspot
    EXPECT_CALL(*mock_registry_, ReadDWORD(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\WiFi",
        "AllowInternetSharing", _))
        .WillOnce(DoAll(SetArgReferee<2>(0), Return(true)));
    
    bool mdm_allows = capabilities_->IsHotspotAllowedByMDM();
    EXPECT_FALSE(mdm_allows);
    
    // Test without MDM restrictions
    EXPECT_CALL(*mock_registry_, ReadDWORD(_, "AllowInternetSharing", _))
        .WillOnce(Return(false)); // Key doesn't exist = no restriction
    
    mdm_allows = capabilities_->IsHotspotAllowedByMDM();
    EXPECT_TRUE(mdm_allows);
    
    FAIL() << "MobileHotspotCapabilities::IsHotspotAllowedByMDM not implemented - TDD red phase";
}

// ============================================================================
// Network Adapter Analysis Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesNetworkTest, AnalyzesInternetConnectivity) {
    CreateCapabilities();
    
    // Mock network adapters with different states
    std::vector<NetworkAdapterInfo> adapters = {
        {"Ethernet", NetworkInterfaceType::Ethernet, true, OperationalStatus::Up},
        {"Wi-Fi", NetworkInterfaceType::Wireless80211, false, OperationalStatus::Down},
        {"Bluetooth", NetworkInterfaceType::Bluetooth, true, OperationalStatus::Up}
    };
    
    EXPECT_CALL(*mock_winrt_, GetNetworkAdapters())
        .WillOnce(Return(adapters));
    
    auto connectivity = capabilities_->AnalyzeInternetConnectivity();
    EXPECT_TRUE(connectivity.has_internet);
    EXPECT_EQ(connectivity.primary_adapter_type, NetworkInterfaceType::Ethernet);
    EXPECT_EQ(connectivity.connected_adapters.size(), 2);
    
    FAIL() << "MobileHotspotCapabilities::AnalyzeInternetConnectivity not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesNetworkTest, DetectsLoopbackWorkaroundCapability) {
    CreateCapabilities();
    
    // Test with only loopback available (no internet)
    std::vector<NetworkAdapterInfo> loopback_only = {
        {"Loopback", NetworkInterfaceType::Loopback, true, OperationalStatus::Up}
    };
    
    EXPECT_CALL(*mock_winrt_, GetNetworkAdapters())
        .WillOnce(Return(loopback_only));
    
    bool can_use_workaround = capabilities_->CanUseLoopbackWorkaround();
    EXPECT_TRUE(can_use_workaround);
    
    auto connectivity = capabilities_->AnalyzeInternetConnectivity();
    EXPECT_FALSE(connectivity.has_internet);
    EXPECT_TRUE(connectivity.can_use_loopback_workaround);
    
    FAIL() << "MobileHotspotCapabilities::CanUseLoopbackWorkaround not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesNetworkTest, CalculatesOptimalHotspotConfiguration) {
    CreateCapabilities();
    
    // Mock dual-band WiFi adapter
    std::vector<WiFiAdapterInfo> adapters = {{
        "Intel AC 9560", "Intel", "23.20.0.3",
        true, true, {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(adapters));
    
    auto optimal_config = capabilities_->GetOptimalHotspotConfiguration();
    EXPECT_EQ(optimal_config.preferred_band, WiFiBand::FiveGHz); // Less congested
    EXPECT_GT(optimal_config.max_clients, 0);
    EXPECT_LE(optimal_config.max_clients, 8);
    EXPECT_FALSE(optimal_config.suggested_ssid.empty());
    
    FAIL() << "MobileHotspotCapabilities::GetOptimalHotspotConfiguration not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesNetworkTest, AnalyzesChannelAvailability) {
    CreateCapabilities();
    
    // Mock WiFi scan results for channel analysis
    std::map<int, int> channel_usage = {
        {1, 3},   // Channel 1: 3 networks
        {6, 1},   // Channel 6: 1 network
        {11, 5},  // Channel 11: 5 networks
        {36, 0},  // Channel 36 (5GHz): No networks
        {149, 1}  // Channel 149 (5GHz): 1 network
    };
    
    EXPECT_CALL(*mock_winrt_, ScanNearbyNetworks())
        .WillOnce(Return(channel_usage));
    
    auto channel_analysis = capabilities_->AnalyzeChannelAvailability();
    
    // Should recommend least congested channels
    EXPECT_EQ(channel_analysis.best_2_4ghz_channel, 6);
    EXPECT_EQ(channel_analysis.best_5ghz_channel, 36);
    EXPECT_TRUE(channel_analysis.is_5ghz_clear);
    
    FAIL() << "MobileHotspotCapabilities::AnalyzeChannelAvailability not implemented - TDD red phase";
}

// ============================================================================
// Fallback Capability Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesFallbackTest, DeterminesBestFallbackMode) {
    CreateCapabilities();
    
    // Scenario 1: WiFi Direct available
    std::vector<WiFiAdapterInfo> wifi_direct_adapters = {{
        "WiFi Direct Adapter", "Intel", "20.0.0.0",
        false, true, {WiFiBand::TwoPointFourGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(wifi_direct_adapters));
    
    auto fallback_mode = capabilities_->GetBestFallbackMode();
    EXPECT_EQ(fallback_mode, FallbackMode::WiFiDirect);
    
    // Scenario 2: No WiFi Direct, but internet available
    std::vector<WiFiAdapterInfo> no_wifi_direct = {{
        "Basic WiFi Adapter", "Generic", "1.0.0.0",
        false, false, {WiFiBand::TwoPointFourGHz}
    }};
    
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(no_wifi_direct));
    
    std::vector<NetworkAdapterInfo> internet_adapters = {
        {"Ethernet", NetworkInterfaceType::Ethernet, true, OperationalStatus::Up}
    };
    EXPECT_CALL(*mock_winrt_, GetNetworkAdapters())
        .WillOnce(Return(internet_adapters));
    
    fallback_mode = capabilities_->GetBestFallbackMode();
    EXPECT_EQ(fallback_mode, FallbackMode::InternetMode);
    
    FAIL() << "MobileHotspotCapabilities::GetBestFallbackMode not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesFallbackTest, EvaluatesFallbackCapabilities) {
    CreateCapabilities();
    
    auto fallback_capabilities = capabilities_->EvaluateFallbackCapabilities();
    
    // Should evaluate all fallback options
    EXPECT_TRUE(fallback_capabilities.find(FallbackMode::WiFiDirect) != fallback_capabilities.end());
    EXPECT_TRUE(fallback_capabilities.find(FallbackMode::InternetMode) != fallback_capabilities.end());
    EXPECT_TRUE(fallback_capabilities.find(FallbackMode::BluetoothTethering) != fallback_capabilities.end());
    
    // Each capability should have a reason and availability
    for (const auto& [mode, capability] : fallback_capabilities) {
        EXPECT_FALSE(capability.reason.empty());
        // availability can be true or false
    }
    
    FAIL() << "MobileHotspotCapabilities::EvaluateFallbackCapabilities not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesFallbackTest, ChecksBluetoothTetheringSupport) {
    CreateCapabilities();
    
    // Test with Bluetooth available
    EXPECT_CALL(*mock_winrt_, IsBluetoothAvailable())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_winrt_, GetBluetoothVersion())
        .WillOnce(Return("5.0"));
    
    bool bt_tethering_available = capabilities_->IsBluetoothTetheringAvailable();
    EXPECT_TRUE(bt_tethering_available);
    
    // Test without Bluetooth
    EXPECT_CALL(*mock_winrt_, IsBluetoothAvailable())
        .WillOnce(Return(false));
    
    bt_tethering_available = capabilities_->IsBluetoothTetheringAvailable();
    EXPECT_FALSE(bt_tethering_available);
    
    FAIL() << "MobileHotspotCapabilities::IsBluetoothTetheringAvailable not implemented - TDD red phase";
}

// ============================================================================
// Comprehensive Capability Assessment Tests
// ============================================================================

TEST_F(MobileHotspotCapabilitiesTest, GeneratesComprehensiveCapabilityReport) {
    CreateCapabilities();
    
    auto report = capabilities_->GenerateCapabilityReport();
    
    // Report should contain all major sections
    EXPECT_FALSE(report.windows_version.version_name.empty());
    EXPECT_FALSE(report.wifi_adapters.empty() && report.has_wifi_adapters);
    EXPECT_GE(report.tethering_support, TetheringSupport::None);
    EXPECT_LE(report.tethering_support, TetheringSupport::Full);
    
    // Should have fallback analysis
    EXPECT_FALSE(report.fallback_modes.empty());
    
    // Should include policy information
    EXPECT_TRUE(report.policy_restrictions.checked);
    
    FAIL() << "MobileHotspotCapabilities::GenerateCapabilityReport not implemented - TDD red phase";
}

TEST_F(MobileHotspotCapabilitiesTest, RecommendsBestOperationMode) {
    CreateCapabilities();
    
    // Test optimal scenario (everything supported)
    mock_winrt_->SetWindowsVersion(10, 0, 19041); // Windows 10 2004
    std::vector<WiFiAdapterInfo> optimal_adapters = {{
        "Intel AX200", "Intel", "23.20.0.3",
        true, true, {WiFiBand::TwoPointFourGHz, WiFiBand::FiveGHz, WiFiBand::SixGHz}
    }};
    EXPECT_CALL(*mock_wmi_, QueryWiFiAdapters())
        .WillOnce(Return(optimal_adapters));
    
    auto recommendation = capabilities_->RecommendOperationMode();
    EXPECT_EQ(recommendation.primary_mode, OperationMode::MobileHotspot);
    EXPECT_FALSE(recommendation.fallback_modes.empty());
    EXPECT_FALSE(recommendation.reason.empty());
    
    FAIL() << "MobileHotspotCapabilities::RecommendOperationMode not implemented - TDD red phase";
}

} // anonymous namespace