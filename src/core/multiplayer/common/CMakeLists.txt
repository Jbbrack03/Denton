# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
    # Add source files as they are created
)

set(HEADERS
    error_codes.h
)

add_library(sudachi_multiplayer_common STATIC ${SOURCES} ${HEADERS})

target_include_directories(sudachi_multiplayer_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(sudachi_multiplayer_common
    PUBLIC
        sudachi_common
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_compile_features(sudachi_multiplayer_common PUBLIC cxx_std_17)