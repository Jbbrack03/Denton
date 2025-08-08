// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Test program to verify WinRT integration
#ifdef _WIN32

#include <iostream>
#include <string>
#include "windows_capability_detector.h"

int main() {
    using namespace Core::Multiplayer::ModelB::Windows;
    
    std::cout << "=== Testing Windows WinRT Integration ===\n\n";
    
    // Get diagnostic report
    std::string report = WindowsCapabilityDetector::GetDiagnosticReport();
    std::cout << report << std::endl;
    
    // Test specific capabilities
    std::cout << "\n=== Additional Tests ===\n";
    
    // Test WinRT initialization
    std::cout << "Testing WinRT initialization... ";
    if (WindowsCapabilityDetector::CanInitializeWinRT()) {
        std::cout << "SUCCESS\n";
    } else {
        std::cout << "FAILED\n";
    }
    
    // Test loopback adapter
    if (!WindowsCapabilityDetector::IsLoopbackAdapterInstalled()) {
        std::cout << "\nLoopback adapter not installed.\n";
        std::cout << "To install: Run as administrator and use Device Manager to add 'Microsoft KM-TEST Loopback Adapter'\n";
    } else {
        auto guid = WindowsCapabilityDetector::GetLoopbackAdapterGuid();
        if (guid.has_value()) {
            std::cout << "\nLoopback adapter GUID: " << guid.value() << "\n";
        }
    }
    
    // Recommended action
    auto mode = WindowsCapabilityDetector::GetRecommendedMode();
    std::cout << "\n=== Recommended Action ===\n";
    std::cout << WindowsCapabilityDetector::GetRecommendedModeDescription(mode) << "\n";
    
    if (!WindowsCapabilityDetector::IsRunningAsAdministrator() && 
        mode == WindowsCapabilityDetector::RecommendedMode::MobileHotspot) {
        std::cout << "\nNote: Mobile Hotspot management requires administrator privileges.\n";
        std::cout << "Please run this program as administrator for full functionality.\n";
    }
    
    return 0;
}

#else
int main() {
    std::cout << "This test is Windows-specific.\n";
    return 1;
}
#endif
