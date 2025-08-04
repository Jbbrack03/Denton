# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Windows-specific CMake configuration for Mobile Hotspot functionality

# Ensure we're building for Windows
if(NOT WIN32)
    message(FATAL_ERROR "This CMakeLists.txt is Windows-specific")
endif()

# Require Windows 10 SDK version 1607 or later for NetworkOperatorTetheringManager
# Version 1607 = Build 14393 = Windows 10 Anniversary Update
if(NOT DEFINED CMAKE_SYSTEM_VERSION OR CMAKE_SYSTEM_VERSION VERSION_LESS "10.0.14393")
    set(CMAKE_SYSTEM_VERSION "10.0.14393.0" CACHE STRING "Windows SDK version" FORCE)
endif()

# Find Windows SDK
find_package(WindowsSDK REQUIRED)
if(WINDOWSSDK_FOUND)
    message(STATUS "Found Windows SDK: ${WINDOWSSDK_LATEST_DIR}")
    message(STATUS "Windows SDK Version: ${WINDOWSSDK_LATEST_VERSION}")
else()
    message(FATAL_ERROR "Windows SDK not found. Please install Windows SDK 10.0.14393 or later.")
endif()

# Check for C++/WinRT support
find_program(CPPWINRT_EXE cppwinrt 
    PATHS 
        "${WINDOWSSDK_LATEST_DIR}/bin/${WINDOWSSDK_LATEST_VERSION}/x64"
        "${WINDOWSSDK_LATEST_DIR}/bin/x64"
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_SYSTEM_VERSION}/x64"
)

if(NOT CPPWINRT_EXE)
    message(WARNING "C++/WinRT tool (cppwinrt.exe) not found. WinRT features may not work properly.")
else()
    message(STATUS "Found C++/WinRT: ${CPPWINRT_EXE}")
endif()

# Windows-specific source files
set(WINDOWS_SOURCES
    mobile_hotspot_manager.cpp
    mobile_hotspot_capabilities.cpp
    windows_capability_detector.cpp
)

set(WINDOWS_HEADERS
    mobile_hotspot_manager.h
    mobile_hotspot_capabilities.h
    windows_types.h
    windows_capability_detector.h
)

# Create Windows platform library
add_library(sudachi_multiplayer_windows_platform STATIC
    ${WINDOWS_SOURCES}
    ${WINDOWS_HEADERS}
)

target_include_directories(sudachi_multiplayer_windows_platform
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        "${WINDOWSSDK_LATEST_DIR}/Include/${WINDOWSSDK_LATEST_VERSION}/cppwinrt"
)

# Windows-specific compile definitions
target_compile_definitions(sudachi_multiplayer_windows_platform PRIVATE
    WINRT_LEAN_AND_MEAN
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WINDOWS
    # Enable C++/WinRT
    WINRT_NO_MAKE_DETECTION
)

# Link Windows Runtime libraries
target_link_libraries(sudachi_multiplayer_windows_platform
    PUBLIC
        sudachi_multiplayer_common
    PRIVATE
        windowsapp.lib    # Windows Runtime core
        ws2_32.lib       # Winsock for network operations
        iphlpapi.lib     # IP Helper API for network interface info
        # Additional libraries for tethering
        runtimeobject.lib # Windows Runtime Object support
)

# Set C++ standard to C++17 minimum (C++/WinRT requirement)
target_compile_features(sudachi_multiplayer_windows_platform PUBLIC cxx_std_17)

# Enable coroutines for C++/WinRT (if using C++20)
if(CMAKE_CXX_STANDARD VERSION_GREATER_EQUAL 20)
    target_compile_options(sudachi_multiplayer_windows_platform PRIVATE "/await")
endif()

# Platform capability detection helper
add_library(sudachi_windows_capability_detector INTERFACE)
target_compile_definitions(sudachi_windows_capability_detector INTERFACE
    SUDACHI_WINDOWS_SDK_VERSION="${WINDOWSSDK_LATEST_VERSION}"
    SUDACHI_HAS_WINRT=$<BOOL:${CPPWINRT_EXE}>
)

# Optional: Build test executable for WinRT integration
if(BUILD_TESTING)
    add_executable(test_winrt_integration test_winrt_integration.cpp)
    target_link_libraries(test_winrt_integration PRIVATE
        sudachi_multiplayer_windows_platform
    )
    set_target_properties(test_winrt_integration PROPERTIES
        FOLDER "Tests/Multiplayer/Windows"
    )
    
    # Add Google Test for Windows capability detector
    find_package(GTest REQUIRED)
    
    add_executable(test_windows_capability_detector
        test_windows_capability_detector.cpp
    )
    
    target_link_libraries(test_windows_capability_detector PRIVATE
        sudachi_multiplayer_windows_platform
        GTest::gtest
        GTest::gtest_main
    )
    
    set_target_properties(test_windows_capability_detector PROPERTIES
        FOLDER "Tests/Multiplayer/Windows"
    )
    
    # Add to CTest
    include(GoogleTest)
    gtest_discover_tests(test_windows_capability_detector)
endif()