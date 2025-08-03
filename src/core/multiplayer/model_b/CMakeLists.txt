# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
    # Add source files as they are created
)

set(HEADERS
    # Add header files as they are created
)

# Platform-specific sources
if(ANDROID)
    list(APPEND SOURCES
        platform/android/wifi_direct_wrapper.cpp
    )
    list(APPEND HEADERS
        platform/android/wifi_direct_wrapper.h
    )
elseif(WIN32)
    list(APPEND SOURCES
        platform/windows/mobile_hotspot_manager.cpp
    )
    list(APPEND HEADERS
        platform/windows/mobile_hotspot_manager.h
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