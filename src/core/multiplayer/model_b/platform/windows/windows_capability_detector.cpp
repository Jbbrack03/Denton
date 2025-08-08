// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windows_capability_detector.h"

#ifdef _WIN32

#include <windows.h>
#include <versionhelpers.h>
#include <shlobj.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <sstream>
#include <vector>

// WinRT headers - conditionally included based on SDK availability
#if defined(WINRT_BASE_H)
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.Networking.NetworkOperators.h>
#endif

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace Core::Multiplayer::ModelB::Windows {

WindowsCapabilityDetector::WindowsVersionInfo WindowsCapabilityDetector::GetWindowsVersion() {
    WindowsVersionInfo info{};
    
    // Use RtlGetVersion for accurate version info (GetVersionEx is deprecated)
    typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        RtlGetVersionPtr rtlGetVersion = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOW versionInfo = {};
            versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
            if (rtlGetVersion(&versionInfo) == 0) { // STATUS_SUCCESS
                info.major_version = versionInfo.dwMajorVersion;
                info.minor_version = versionInfo.dwMinorVersion;
                info.build_number = versionInfo.dwBuildNumber;
                
                // Format version string
                std::stringstream ss;
                ss << info.major_version << "." << info.minor_version << "." << info.build_number;
                info.version_string = ss.str();
                
                // Determine Windows edition
                if (info.major_version == 10) {
                    if (info.build_number >= 22000) {
                        info.edition = "Windows 11";
                    } else {
                        info.edition = "Windows 10";
                    }
                } else if (info.major_version == 6 && info.minor_version == 3) {
                    info.edition = "Windows 8.1";
                } else if (info.major_version == 6 && info.minor_version == 2) {
                    info.edition = "Windows 8";
                } else {
                    info.edition = "Unknown";
                }
            }
        }
    }
    
    return info;
}

bool WindowsCapabilityDetector::IsWindows10OrLater() {
    return IsWindows10OrGreater();
}

bool WindowsCapabilityDetector::IsWindows10Version1607OrLater() {
    auto version = GetWindowsVersion();
    return version.major_version >= 10 && version.build_number >= 14393;
}

WindowsCapabilityDetector::WinRTCapabilities WindowsCapabilityDetector::DetectWinRTCapabilities() {
    WinRTCapabilities caps{};
    
    // Check if WinRT can be initialized
    caps.winrt_available = CanInitializeWinRT();
    
    // Check Windows version for API availability
    if (IsWindows10Version1607OrLater()) {
        caps.mobile_hotspot_api_available = true;
    }
    
    // WiFi Direct is available on Windows 8+
    if (IsWindows8OrGreater()) {
        caps.wifi_direct_available = true;
    }
    
    // Check for loopback adapter
    caps.loopback_adapter_installed = IsLoopbackAdapterInstalled();
    
    // Check elevation
    caps.elevated_privileges = IsRunningAsAdministrator();
    
    return caps;
}

bool WindowsCapabilityDetector::IsWinRTAvailable() {
#if defined(WINRT_BASE_H)
    return true;
#else
    return false;
#endif
}

bool WindowsCapabilityDetector::CanInitializeWinRT() {
#if defined(WINRT_BASE_H)
    try {
        winrt::init_apartment();
        return true;
    } catch (...) {
        return false;
    }
#else
    // Try dynamic loading as fallback
    HMODULE combase = LoadLibraryW(L"combase.dll");
    if (combase) {
        auto roInitialize = GetProcAddress(combase, "RoInitialize");
        FreeLibrary(combase);
        return roInitialize != nullptr;
    }
    return false;
#endif
}

bool WindowsCapabilityDetector::IsRunningAsAdministrator() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin != FALSE;
}

bool WindowsCapabilityDetector::RequestAdminPrivileges() {
    if (IsRunningAsAdministrator()) {
        return true;
    }
    
    // Get current executable path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    
    // Re-launch with elevation
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.nShow = SW_NORMAL;
    
    if (ShellExecuteExW(&sei)) {
        // Original process should exit after launching elevated version
        return false; // Caller should exit
    }
    
    return false;
}

bool WindowsCapabilityDetector::HasWiFiAdapter() {
    ULONG bufferSize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter; adapter = adapter->Next) {
            if (adapter->IfType == IF_TYPE_IEEE80211 && adapter->OperStatus == IfOperStatusUp) {
                return true;
            }
        }
    }
    
    return false;
}

bool WindowsCapabilityDetector::HasEthernetAdapter() {
    ULONG bufferSize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter; adapter = adapter->Next) {
            if (adapter->IfType == IF_TYPE_ETHERNET_CSMACD && adapter->OperStatus == IfOperStatusUp) {
                return true;
            }
        }
    }
    
    return false;
}

bool WindowsCapabilityDetector::HasInternetConnection() {
#if defined(WINRT_BASE_H)
    try {
        using namespace winrt::Windows::Networking::Connectivity;
        auto profile = NetworkInformation::GetInternetConnectionProfile();
        return profile != nullptr;
    } catch (...) {
        // Fallback to WinAPI method
    }
#endif
    
    // Fallback: Try to resolve a well-known host
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        struct hostent* host = gethostbyname("dns.google");
        WSACleanup();
        return host != nullptr;
    }
    
    return false;
}

bool WindowsCapabilityDetector::IsLoopbackAdapterInstalled() {
    ULONG bufferSize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter; adapter = adapter->Next) {
            // Check for Microsoft Loopback Adapter
            std::wstring description(adapter->Description);
            if (description.find(L"Microsoft") != std::wstring::npos && 
                description.find(L"Loopback") != std::wstring::npos) {
                return true;
            }
        }
    }
    
    return false;
}

std::optional<std::string> WindowsCapabilityDetector::GetLoopbackAdapterGuid() {
    ULONG bufferSize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter; adapter = adapter->Next) {
            std::wstring description(adapter->Description);
            if (description.find(L"Microsoft") != std::wstring::npos && 
                description.find(L"Loopback") != std::wstring::npos) {
                // Convert adapter name (GUID) to string
                std::wstring guidStr(adapter->AdapterName);
                std::string result(guidStr.begin(), guidStr.end());
                return result;
            }
        }
    }
    
    return std::nullopt;
}

bool WindowsCapabilityDetector::InstallLoopbackAdapter() {
    if (!IsRunningAsAdministrator()) {
        return false;
    }
    
    // Use devcon or pnputil to install Microsoft Loopback Adapter
    // This requires the hdwwiz.exe (Add Hardware Wizard)
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = L"hdwwiz.exe";
    sei.lpParameters = L"/a /m";
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    
    if (ShellExecuteExW(&sei)) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        return exitCode == 0;
    }
    
    return false;
}

bool WindowsCapabilityDetector::IsMobileHotspotSupported() {
    if (!IsWindows10Version1607OrLater()) {
        return false;
    }
    
#if defined(WINRT_BASE_H)
    try {
        using namespace winrt::Windows::Networking::NetworkOperators;
        using namespace winrt::Windows::Networking::Connectivity;
        
        // Check if we can get an internet connection profile
        auto profile = NetworkInformation::GetInternetConnectionProfile();
        if (!profile) {
            // No internet connection - might still work with loopback
            return IsLoopbackAdapterInstalled();
        }
        
        // Try to create tethering manager
        auto manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
        return manager != nullptr;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

bool WindowsCapabilityDetector::IsMobileHotspotEnabled() {
#if defined(WINRT_BASE_H)
    try {
        using namespace winrt::Windows::Networking::NetworkOperators;
        using namespace winrt::Windows::Networking::Connectivity;
        
        auto profile = NetworkInformation::GetInternetConnectionProfile();
        if (!profile) {
            return false;
        }
        
        auto manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
        if (manager) {
            return manager.TetheringOperationalState() == TetheringOperationalState::On;
        }
    } catch (...) {
        // Ignore exceptions
    }
#endif
    return false;
}

bool WindowsCapabilityDetector::CanUseMobileHotspotWithoutInternet() {
    return IsLoopbackAdapterInstalled() && IsMobileHotspotSupported();
}

bool WindowsCapabilityDetector::IsWiFiDirectSupported() {
    // WiFi Direct requires Windows 8+ and a compatible WiFi adapter
    return IsWindows8OrGreater() && HasWiFiAdapter();
}

bool WindowsCapabilityDetector::IsWiFiDirectEnabled() {
    // This would require checking WlanHostedNetwork status or WiFi Direct API
    // For now, just check if the service is running
    SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scManager) {
        SC_HANDLE service = OpenServiceW(scManager, L"WlanSvc", SERVICE_QUERY_STATUS);
        if (service) {
            SERVICE_STATUS status;
            if (QueryServiceStatus(service, &status)) {
                CloseServiceHandle(service);
                CloseServiceHandle(scManager);
                return status.dwCurrentState == SERVICE_RUNNING;
            }
            CloseServiceHandle(service);
        }
        CloseServiceHandle(scManager);
    }
    return false;
}

WindowsCapabilityDetector::RecommendedMode WindowsCapabilityDetector::GetRecommendedMode() {
    auto caps = DetectWinRTCapabilities();
    
    // First choice: Mobile Hotspot with internet
    if (caps.mobile_hotspot_api_available && HasInternetConnection()) {
        return RecommendedMode::MobileHotspot;
    }
    
    // Second choice: Mobile Hotspot with loopback adapter
    if (caps.mobile_hotspot_api_available && caps.loopback_adapter_installed) {
        return RecommendedMode::MobileHotspotWithLoopback;
    }
    
    // Third choice: WiFi Direct
    if (caps.wifi_direct_available && HasWiFiAdapter()) {
        return RecommendedMode::WiFiDirect;
    }
    
    // Fourth choice: Fall back to Internet multiplayer
    if (HasInternetConnection()) {
        return RecommendedMode::InternetMultiplayer;
    }
    
    // No suitable mode available
    return RecommendedMode::NotSupported;
}

std::string WindowsCapabilityDetector::GetRecommendedModeDescription(RecommendedMode mode) {
    switch (mode) {
    case RecommendedMode::MobileHotspot:
        return "Mobile Hotspot (Recommended) - Create a local wireless network for nearby devices";
    case RecommendedMode::MobileHotspotWithLoopback:
        return "Mobile Hotspot with Loopback Adapter - Create a local network without internet";
    case RecommendedMode::WiFiDirect:
        return "WiFi Direct - Direct device-to-device connection";
    case RecommendedMode::InternetMultiplayer:
        return "Internet Multiplayer - Play with others over the internet";
    case RecommendedMode::NotSupported:
        return "No supported multiplayer mode available on this system";
    default:
        return "Unknown mode";
    }
}

std::string WindowsCapabilityDetector::GetDiagnosticReport() {
    std::stringstream report;
    
    report << "=== Windows Capability Diagnostic Report ===\n\n";
    
    // Windows version
    auto version = GetWindowsVersion();
    report << "Windows Version: " << version.edition << " (" << version.version_string << ")\n";
    report << "Build Number: " << version.build_number << "\n\n";
    
    // WinRT capabilities
    auto caps = DetectWinRTCapabilities();
    report << "WinRT Capabilities:\n";
    report << "  WinRT Available: " << (caps.winrt_available ? "Yes" : "No") << "\n";
    report << "  Mobile Hotspot API: " << (caps.mobile_hotspot_api_available ? "Yes" : "No") << "\n";
    report << "  WiFi Direct API: " << (caps.wifi_direct_available ? "Yes" : "No") << "\n";
    report << "  Loopback Adapter: " << (caps.loopback_adapter_installed ? "Installed" : "Not Installed") << "\n";
    report << "  Admin Privileges: " << (caps.elevated_privileges ? "Yes" : "No") << "\n\n";
    
    // Network adapters
    report << "Network Adapters:\n";
    report << "  WiFi Adapter: " << (HasWiFiAdapter() ? "Present" : "Not Found") << "\n";
    report << "  Ethernet Adapter: " << (HasEthernetAdapter() ? "Present" : "Not Found") << "\n";
    report << "  Internet Connection: " << (HasInternetConnection() ? "Available" : "Not Available") << "\n\n";
    
    // Feature support
    report << "Feature Support:\n";
    report << "  Mobile Hotspot: " << (IsMobileHotspotSupported() ? "Supported" : "Not Supported") << "\n";
    report << "  Mobile Hotspot Active: " << (IsMobileHotspotEnabled() ? "Yes" : "No") << "\n";
    report << "  WiFi Direct: " << (IsWiFiDirectSupported() ? "Supported" : "Not Supported") << "\n\n";
    
    // Recommendation
    auto mode = GetRecommendedMode();
    report << "Recommended Mode: " << GetRecommendedModeDescription(mode) << "\n";
    
    return report.str();
}

} // namespace Core::Multiplayer::ModelB::Windows

#endif // _WIN32
