# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Test Infrastructure for Sudachi Multiplayer
# Following Implementation Plan Phase 4, Task 4.1

cmake_minimum_required(VERSION 3.22)

# Enable testing
enable_testing()

# Find Google Test package
find_package(GTest CONFIG REQUIRED)

# Common test utilities and mocks
set(TEST_COMMON_SOURCES
    mocks/mock_websocket_server.h
    mocks/mock_p2p_network.h
    mocks/mock_platform_apis.h
    mocks/mock_mdns_discovery.h
    mocks/mock_room_client.h
    mocks/mock_relay_client.h
    mocks/mock_mobile_hotspot_manager.h
    mocks/mock_wifi_direct_wrapper.h
)

# Unit tests
set(UNIT_TEST_SOURCES
    unit/test_hle_interface.cpp
    unit/test_p2p_network.cpp
    unit/test_mdns_discovery.cpp
    unit/test_packet_protocol.cpp
    unit/test_room_client.cpp
    unit/test_relay_client.cpp
    unit/test_configuration.cpp
    unit/test_error_handling.cpp
    unit/test_connection_recovery_manager.cpp
)

# Integration tests
set(INTEGRATION_TEST_SOURCES
    integration/test_model_a_flow.cpp
    integration/test_model_b_flow.cpp
    integration/test_game_integration.cpp
    integration/test_network_fallback.cpp
    integration/test_cross_platform.cpp
)

# Create unit test executable
add_executable(sudachi_multiplayer_unit_tests
    ${UNIT_TEST_SOURCES}
    ${TEST_COMMON_SOURCES}
)

# Create integration test executable
add_executable(sudachi_multiplayer_integration_tests
    ${INTEGRATION_TEST_SOURCES}
    ${TEST_COMMON_SOURCES}
)

# Common test configuration
function(configure_test_target target_name)
    target_link_libraries(${target_name}
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        sudachi_multiplayer_common
        sudachi_multiplayer_model_a
        sudachi_multiplayer_model_b
    )

    target_include_directories(${target_name}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/core/multiplayer
    )

    target_compile_features(${target_name} PUBLIC cxx_std_17)

    # Platform-specific test configuration
    if(WIN32)
        target_compile_definitions(${target_name} PRIVATE
            _WIN32_WINNT=0x0A00  # Windows 10
            NOMINMAX
        )
    elseif(ANDROID)
        target_compile_definitions(${target_name} PRIVATE
            ANDROID_PLATFORM
        )
    endif()

    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(${target_name}
        PROPERTIES
        TIMEOUT 120
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

# Configure test targets
configure_test_target(sudachi_multiplayer_unit_tests)
configure_test_target(sudachi_multiplayer_integration_tests)

# Performance benchmark tests (using Google Benchmark)
find_package(benchmark CONFIG QUIET)
if(benchmark_FOUND)
    add_executable(sudachi_multiplayer_benchmarks
        benchmarks/benchmark_main.cpp
        benchmarks/benchmark_connection_establishment.cpp
        benchmarks/benchmark_latency_measurement.cpp
        benchmarks/benchmark_packet_processing.cpp
        benchmarks/benchmark_scalability_memory.cpp
        benchmarks/benchmark_component_performance.cpp
    )

    target_link_libraries(sudachi_multiplayer_benchmarks
        PRIVATE
        benchmark::benchmark
        sudachi_multiplayer_common
        sudachi_multiplayer_model_a
        sudachi_multiplayer_model_b
    )
    
    target_include_directories(sudachi_multiplayer_benchmarks
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks
        ${CMAKE_SOURCE_DIR}/src/core/multiplayer
    )

    target_compile_features(sudachi_multiplayer_benchmarks PUBLIC cxx_std_17)
    
    # Platform-specific configuration
    if(WIN32)
        target_compile_definitions(sudachi_multiplayer_benchmarks PRIVATE
            _WIN32_WINNT=0x0A00  # Windows 10
            NOMINMAX
        )
    elseif(ANDROID)
        target_compile_definitions(sudachi_multiplayer_benchmarks PRIVATE
            ANDROID_PLATFORM
        )
    endif()
    
    # Add custom benchmark target
    add_custom_target(run_benchmarks
        COMMAND $<TARGET_FILE:sudachi_multiplayer_benchmarks>
        DEPENDS sudachi_multiplayer_benchmarks
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running performance benchmarks..."
    )
    
    # Add benchmark with JSON output
    add_custom_target(run_benchmarks_json
        COMMAND $<TARGET_FILE:sudachi_multiplayer_benchmarks> --benchmark_format=json --benchmark_out=benchmark_results.json
        DEPENDS sudachi_multiplayer_benchmarks
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running performance benchmarks with JSON output..."
    )
    
else()
    message(STATUS "Google Benchmark not found, skipping performance tests")
endif()

# Docker test environment configuration
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/docker/docker-compose.test.yml
    ${CMAKE_BINARY_DIR}/docker-compose.test.yml
    COPYONLY
)

# Custom test targets
add_custom_target(run_unit_tests
    COMMAND $<TARGET_FILE:sudachi_multiplayer_unit_tests>
    DEPENDS sudachi_multiplayer_unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
)

add_custom_target(run_integration_tests
    COMMAND $<TARGET_FILE:sudachi_multiplayer_integration_tests>
    DEPENDS sudachi_multiplayer_integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests..."
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS sudachi_multiplayer_unit_tests sudachi_multiplayer_integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# Coverage reporting (if enabled)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(GCOV_EXECUTABLE gcov)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)

    if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage.info
            COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/externals/*' '*/tests/*' --output-file coverage.filtered.info
            COMMAND ${GENHTML_EXECUTABLE} coverage.filtered.info --output-directory coverage_report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report..."
        )
    endif()
endif()

# Test utilities
add_library(sudachi_test_utilities INTERFACE)
target_sources(sudachi_test_utilities INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_utilities.h
    ${CMAKE_CURRENT_SOURCE_DIR}/network_test_helpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/game_test_fixtures.h
)

# Add UI tests subdirectory
add_subdirectory(ui)

# Add core tests subdirectory
add_subdirectory(unit/core)

# Export test configuration for use by other modules
set(SUDACHI_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE PATH "Path to test data files")
set(SUDACHI_ENABLE_INTEGRATION_TESTS ON CACHE BOOL "Enable integration tests")
set(SUDACHI_ENABLE_PERFORMANCE_TESTS ${benchmark_FOUND} CACHE BOOL "Enable performance benchmarks")
set(SUDACHI_ENABLE_UI_TESTS ON CACHE BOOL "Enable UI tests")