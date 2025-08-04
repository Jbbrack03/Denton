# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
    # mDNS Discovery implementation
    mdns_discovery.cpp
    mdns_txt_records.cpp
)

set(HEADERS
    # mDNS Discovery headers (interfaces defined for TDD red phase)
    mdns_discovery.h
    mdns_txt_records.h
)

# Platform-specific sources
if(ANDROID)
    list(APPEND SOURCES
        platform/android/wifi_direct_wrapper.cpp
        platform/android/wifi_direct_permission_manager.cpp
    )
    list(APPEND HEADERS
        platform/android/wifi_direct_wrapper.h
        platform/android/wifi_direct_permission_manager.h
    )
elseif(WIN32)
    list(APPEND SOURCES
        platform/windows/mobile_hotspot_manager.cpp
        platform/windows/mobile_hotspot_capabilities.cpp
    )
    list(APPEND HEADERS
        platform/windows/mobile_hotspot_manager.h
        platform/windows/mobile_hotspot_capabilities.h
        platform/windows/windows_types.h
    )
endif()

add_library(sudachi_multiplayer_model_b STATIC ${SOURCES} ${HEADERS})

target_include_directories(sudachi_multiplayer_model_b
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../externals/mdns
)

target_link_libraries(sudachi_multiplayer_model_b
    PUBLIC
        sudachi_multiplayer_common
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(sudachi_multiplayer_model_b
        PRIVATE
            windowsapp  # For Windows Runtime APIs
    )
endif()

target_compile_features(sudachi_multiplayer_model_b PUBLIC cxx_std_17)

# Add test subdirectory for TDD development
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()