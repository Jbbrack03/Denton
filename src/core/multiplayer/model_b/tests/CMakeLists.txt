# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Find required packages
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Test source files
set(TEST_SOURCES
    test_mdns_discovery.cpp
    test_mdns_txt_records.cpp
)

# Platform-specific test sources
if(ANDROID)
    list(APPEND TEST_SOURCES
        test_wifi_direct_wrapper.cpp
        test_wifi_direct_permissions.cpp
    )
elseif(WIN32)
    list(APPEND TEST_SOURCES
        test_mobile_hotspot_manager.cpp
        test_mobile_hotspot_capabilities.cpp
    )
endif()

# Red phase verification
set(VERIFICATION_SOURCES
    verify_red_phase.cpp
)

# Mock headers
set(MOCK_HEADERS
    mocks/mock_mdns_socket.h
)

# Platform-specific mock headers
if(ANDROID)
    list(APPEND MOCK_HEADERS
        mocks/mock_jni_env.h
    )
elseif(WIN32)
    list(APPEND MOCK_HEADERS
        mocks/mock_winrt_apis.h
    )
endif()

# Create main test executable
add_executable(test_multiplayer_model_b
    ${TEST_SOURCES}
    ${MOCK_HEADERS}
)

# Create red phase verification executable
add_executable(verify_model_b_red_phase_exe
    ${VERIFICATION_SOURCES}
    ${MOCK_HEADERS}
)

# Include directories
target_include_directories(test_multiplayer_model_b
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../externals/mdns
)

# Include directories for verification executable
target_include_directories(verify_model_b_red_phase_exe
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../externals/mdns
)

# Link libraries
target_link_libraries(test_multiplayer_model_b
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GMock::gmock
        GMock::gmock_main
        sudachi_multiplayer_common
        sudachi_multiplayer_model_b
)

# Link libraries for verification executable
target_link_libraries(verify_model_b_red_phase_exe
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GMock::gmock
        GMock::gmock_main
        sudachi_multiplayer_common
)

# Compiler features
target_compile_features(test_multiplayer_model_b PRIVATE cxx_std_17)
target_compile_features(verify_model_b_red_phase_exe PRIVATE cxx_std_17)

# Test discovery
include(GoogleTest)
gtest_discover_tests(test_multiplayer_model_b
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES
        TIMEOUT 30
)

# Custom test target for TDD red phase verification
add_custom_target(verify_model_b_red_phase
    COMMAND test_multiplayer_model_b --gtest_list_tests
    DEPENDS test_multiplayer_model_b
    COMMENT "Verifying Model B TDD red phase - all tests should fail"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Test categories for selective running
set_tests_properties(
    test_multiplayer_model_b
    PROPERTIES
        LABELS "model_b;mdns;tdd_red_phase"
)

# Add performance test configuration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(test_multiplayer_model_b
        PRIVATE
            ENABLE_PERFORMANCE_TESTS=1
    )
endif()

# Platform-specific test configuration
if(WIN32)
    target_compile_definitions(test_multiplayer_model_b
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            ENABLE_MOBILE_HOTSPOT_TESTS=1
    )
elseif(ANDROID)
    target_compile_definitions(test_multiplayer_model_b
        PRIVATE
            ANDROID_PLATFORM=1
            ENABLE_WIFI_DIRECT_TESTS=1
    )
endif()

# Coverage configuration for test analysis
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
    if(ENABLE_COVERAGE)
        target_compile_options(test_multiplayer_model_b
            PRIVATE
                --coverage
                -fprofile-arcs
                -ftest-coverage
        )
        target_link_options(test_multiplayer_model_b
            PRIVATE
                --coverage
        )
    endif()
endif()
