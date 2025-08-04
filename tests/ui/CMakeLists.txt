# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Simplified UI Test build for verification
# This verifies our test structure without requiring full Qt6 setup

cmake_minimum_required(VERSION 3.22)
project(SudachiMultiplayerUITests)

# Find Google Test package
find_package(GTest CONFIG REQUIRED)

# Simple verification test
add_executable(ui_test_verification
    test_build_verification.cpp
)

# Implementation verification test
add_executable(ui_implementation_verification
    test_implementation_verification.cpp
)

target_link_libraries(ui_test_verification
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_link_libraries(ui_implementation_verification
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_compile_features(ui_test_verification PUBLIC cxx_std_17)
target_compile_features(ui_implementation_verification PUBLIC cxx_std_17)

# Enable testing
enable_testing()

# Add tests
add_test(NAME ui_test_verification COMMAND ui_test_verification)
add_test(NAME ui_implementation_verification COMMAND ui_implementation_verification)