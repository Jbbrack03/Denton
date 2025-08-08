# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
    error_handling.cpp
    network_security.cpp
    secure_network_handler.cpp
    # Phase 8.1: Error Recovery and Resilience Framework
    connection_recovery_manager.cpp
    circuit_breaker.cpp
    graceful_degradation_manager.cpp
)

set(HEADERS
    error_codes.h
    error_handling.h
    network_security.h
    secure_network_handler.h
    # Phase 8.1: Error Recovery and Resilience Framework
    connection_recovery_manager.h
    circuit_breaker.h
    graceful_degradation_manager.h
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

# Add tests subdirectory
if(ENABLE_TESTING)
    add_subdirectory(tests)
endif()
