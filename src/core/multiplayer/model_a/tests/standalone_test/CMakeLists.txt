# SPDX-FileCopyrightText: 2025 sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

project(multiplayer_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Add our minimal test
add_executable(test_multiplayer_compilation
    ../test_compilation_check.cpp
    ../../relay_protocol.cpp
)

target_include_directories(test_multiplayer_compilation
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

target_compile_definitions(test_multiplayer_compilation
    PRIVATE
        BUILDING_TESTS=1
)

target_link_libraries(test_multiplayer_compilation
    PRIVATE
        gtest
        gtest_main
        gmock
)

enable_testing()
add_test(NAME compilation_test COMMAND test_multiplayer_compilation)
