// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#include "core/multiplayer/model_b/platform/windows/mobile_hotspot_capabilities.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelB::Windows;

namespace {

TEST(MobileHotspotCapabilitiesTest, DetectsWindowsVersionCorrectly) {
    MobileHotspotCapabilities caps;
    auto version = caps.GetWindowsVersionInfo();
    EXPECT_EQ(version.major_version, 10u);
    EXPECT_EQ(version.minor_version, 0u);
    EXPECT_EQ(version.build_number, 14393u);
    EXPECT_EQ(version.version_name, "Windows 10 1607");
}

TEST(MobileHotspotCapabilitiesTest, ReportsSupportedByVersion) {
    MobileHotspotCapabilities caps;
    EXPECT_TRUE(caps.IsHotspotSupportedByVersion());
    EXPECT_TRUE(caps.IsWiFiDirectSupportedByVersion());
}

TEST(MobileHotspotCapabilitiesTest, DeterminesHardwareCapabilities) {
    MobileHotspotCapabilities caps;
    EXPECT_TRUE(caps.CanCreateHotspot());
    EXPECT_TRUE(caps.CanUseWiFiDirect());
}

TEST(MobileHotspotCapabilitiesTest, AggregatesSupportedBands) {
    MobileHotspotCapabilities caps;
    auto bands = caps.GetSupportedWiFiBands();
    EXPECT_THAT(bands, Contains(WiFiBand::TwoPointFourGHz));
    EXPECT_THAT(bands, Contains(WiFiBand::FiveGHz));
}

TEST(MobileHotspotCapabilitiesTest, EvaluatesFallbackMode) {
    MobileHotspotCapabilities caps;
    EXPECT_EQ(caps.GetBestFallbackMode(), FallbackMode::WiFiDirect);
}

TEST(MobileHotspotCapabilitiesTest, GeneratesCapabilityReport) {
    MobileHotspotCapabilities caps;
    auto report = caps.GenerateCapabilityReport();
    EXPECT_FALSE(report.windows_version.version_name.empty());
    EXPECT_TRUE(report.has_wifi_adapters);
    EXPECT_FALSE(report.fallback_modes.empty());
}

} // namespace

