// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/multiplayer/common/error_codes.h"
#include "windows_types.h"
#include <memory>
#include <string>
#include <vector>

namespace Core::Multiplayer::ModelB::Windows {

/**
 * Windows Mobile Hotspot Manager
 * Manages Windows Mobile Hotspot functionality with multi-tier fallback strategy.
 * 
 * This class will be implemented following TDD methodology.
 * All tests should currently FAIL as this is the red phase.
 */
class MobileHotspotManager {
public:
    MobileHotspotManager();
    ~MobileHotspotManager();
    
    // Core lifecycle methods
    Core::Multiplayer::ErrorCode Initialize();
    void Shutdown();
    HotspotState GetState() const;
    
    // Capability detection
    bool IsWindowsVersionSupported() const;
    bool IsHotspotCapable() const;
    bool HasInternetConnection() const;
    bool CanUseLoopbackWorkaround() const;
    
    // Configuration
    Core::Multiplayer::ErrorCode ConfigureHotspot(const HotspotConfiguration& config);
    HotspotConfiguration GetCurrentConfiguration() const;
    
    // Operations
    Core::Multiplayer::ErrorCode StartHotspot();
    Core::Multiplayer::ErrorCode StopHotspot();
    HotspotStatus GetHotspotStatus() const;
    
    // Client management
    void OnClientConnected(const ClientInfo& client);
    void OnClientDisconnected(const std::string& mac_address);
    
    // Fallback management
    FallbackMode GetRecommendedFallbackMode() const;
    Core::Multiplayer::ErrorCode InitializeFallbackMode(FallbackMode mode);
    OperationMode GetCurrentMode() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Core::Multiplayer::ModelB::Windows
