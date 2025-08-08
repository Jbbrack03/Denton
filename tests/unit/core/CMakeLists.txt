# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT BUILD_TESTING)
    return()
endif()

find_package(GTest REQUIRED)

# Core multiplayer integration tests
add_executable(test_multiplayer_integration
    test_multiplayer_integration.cpp
)

target_link_libraries(test_multiplayer_integration PRIVATE
    core
    GTest::gtest
    GTest::gtest_main
)

target_compile_definitions(test_multiplayer_integration PRIVATE
    $<$<BOOL:${ENABLE_MULTIPLAYER}>:ENABLE_MULTIPLAYER>
)

set_target_properties(test_multiplayer_integration PROPERTIES
    FOLDER "Tests/Unit/Core"
)

# Add to CTest
include(GoogleTest)
gtest_discover_tests(test_multiplayer_integration)
