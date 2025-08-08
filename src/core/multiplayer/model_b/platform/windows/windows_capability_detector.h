// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

namespace Core::Multiplayer::ModelB::Windows {

/**
 * Windows Capability Detector
 * Detects Windows version, SDK availability, and WinRT support at runtime.
 */
class WindowsCapabilityDetector {
public:
  struct WindowsVersionInfo {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t build_number;
    std::string version_string;
    std::string edition;
  };

  struct WinRTCapabilities {
    bool winrt_available;
    bool mobile_hotspot_api_available;
    bool wifi_direct_available;
    bool loopback_adapter_installed;
    bool elevated_privileges;
  };

  // Version detection
  static WindowsVersionInfo GetWindowsVersion();
  static bool IsWindows10OrLater();
  static bool
  IsWindows10Version1607OrLater(); // Required for
                                   // NetworkOperatorTetheringManager

  // WinRT capability detection
  static WinRTCapabilities DetectWinRTCapabilities();
  static bool IsWinRTAvailable();
  static bool CanInitializeWinRT();

  // Admin/elevation detection
  static bool IsRunningAsAdministrator();
  static bool RequestAdminPrivileges();

  // Network adapter detection
  static bool HasWiFiAdapter();
  static bool HasEthernetAdapter();
  static bool HasInternetConnection();
  static void InvalidateNetworkAdapterCache();

  // Loopback adapter detection and installation
  static bool IsLoopbackAdapterInstalled();
  static std::optional<std::string> GetLoopbackAdapterGuid();
  static bool InstallLoopbackAdapter();

  // Mobile Hotspot specific checks
  static bool IsMobileHotspotSupported();
  static bool IsMobileHotspotEnabled();
  static bool CanUseMobileHotspotWithoutInternet(); // Loopback workaround

  // WiFi Direct specific checks
  static bool IsWiFiDirectSupported();
  static bool IsWiFiDirectEnabled();

  // Get recommended fallback mode based on capabilities
  enum class RecommendedMode {
    MobileHotspot,
    MobileHotspotWithLoopback,
    WiFiDirect,
    InternetMultiplayer,
    NotSupported
  };
  static RecommendedMode GetRecommendedMode();
  static std::string GetRecommendedModeDescription(RecommendedMode mode);

  // Diagnostic information
  static std::string GetDiagnosticReport();
};

} // namespace Core::Multiplayer::ModelB::Windows
