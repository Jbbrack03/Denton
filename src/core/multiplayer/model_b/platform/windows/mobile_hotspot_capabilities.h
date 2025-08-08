// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/multiplayer/common/error_codes.h"
#include "windows_types.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace Core::Multiplayer::ModelB::Windows {

/**
 * Windows Mobile Hotspot Capabilities Detector
 * Analyzes system capabilities for mobile hotspot functionality.
 * 
 * This class will be implemented following TDD methodology.
 * All tests should currently FAIL as this is the red phase.
 */
class MobileHotspotCapabilities {
public:
    MobileHotspotCapabilities();
    ~MobileHotspotCapabilities();
    
    // Windows version detection
    WindowsVersionInfo GetWindowsVersionInfo() const;
    bool IsHotspotSupportedByVersion() const;
    bool IsWiFiDirectSupportedByVersion() const;
    
    // Hardware detection
    std::vector<WiFiAdapterInfo> GetWiFiAdapters() const;
    bool CanCreateHotspot() const;
    bool CanUseWiFiDirect() const;
    std::vector<WiFiBand> GetSupportedWiFiBands() const;
    
    // Policy and configuration
    bool IsHotspotAllowedByPolicy() const;
    bool IsICSServiceAvailable() const;
    bool DoesEditionSupportHotspot() const;
    bool IsHotspotAllowedByMDM() const;
    
    // Network analysis
    InternetConnectivity AnalyzeInternetConnectivity() const;
    bool CanUseLoopbackWorkaround() const;
    OptimalHotspotConfig GetOptimalHotspotConfiguration() const;
    ChannelAnalysis AnalyzeChannelAvailability() const;
    
    // Fallback capabilities
    FallbackMode GetBestFallbackMode() const;
    std::map<FallbackMode, struct FallbackCapability> EvaluateFallbackCapabilities() const;
    bool IsBluetoothTetheringAvailable() const;
    
    // Comprehensive analysis
    CapabilityReport GenerateCapabilityReport() const;
    OperationModeRecommendation RecommendOperationMode() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer::ModelB::Windows
