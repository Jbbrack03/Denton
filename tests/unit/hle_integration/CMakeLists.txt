# SPDX-FileCopyrightText: 2025 sudachi Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# HLE Integration Tests - Critical bridge between LDN HLE and multiplayer backends
# These tests MUST FAIL initially (red phase) to guide implementation

set(HLE_INTEGRATION_TEST_SOURCES
    test_multiplayer_backend_interface.cpp
    test_backend_factory.cpp
    test_ldn_service_bridge.cpp
    test_error_code_mapper.cpp
    test_type_translator.cpp
)

# Create HLE integration test executable
add_executable(hle_integration_tests ${HLE_INTEGRATION_TEST_SOURCES})

# Link against Google Test and Mock
target_link_libraries(hle_integration_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    sudachi_multiplayer_hle_integration
)

# Include directories for the test files
target_include_directories(hle_integration_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/sudachi/src
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/tests/unit/hle_integration
)

# Compiler flags for proper C++17 support
target_compile_features(hle_integration_tests PRIVATE cxx_std_17)

# Add test discovery for CTest
include(GoogleTest)
gtest_discover_tests(hle_integration_tests
    PROPERTIES
    LABELS "hle_integration;unit;critical"
    TIMEOUT 30
)

# Custom test targets for different categories
add_custom_target(test_backend_interface
    COMMAND $<TARGET_FILE:hle_integration_tests> --gtest_filter="MultiplayerBackendInterfaceTest.*"
    DEPENDS hle_integration_tests
    COMMENT "Running Multiplayer Backend Interface tests"
)

add_custom_target(test_backend_factory
    COMMAND $<TARGET_FILE:hle_integration_tests> --gtest_filter="BackendFactoryTest.*:ConfigurationManagerTest.*:PlatformSpecificBackendTest.*"
    DEPENDS hle_integration_tests
    COMMENT "Running Backend Factory tests"
)

add_custom_target(test_ldn_bridge
    COMMAND $<TARGET_FILE:hle_integration_tests> --gtest_filter="LdnServiceBridgeTest.*:LdnServiceBridgeIntegrationTest.*"
    DEPENDS hle_integration_tests
    COMMENT "Running LDN Service Bridge tests"
)

add_custom_target(test_error_mapping
    COMMAND $<TARGET_FILE:hle_integration_tests> --gtest_filter="ErrorCodeMapperTest.*:ErrorCodeMapperIntegrationTest.*"
    DEPENDS hle_integration_tests
    COMMENT "Running Error Code Mapper tests"
)

add_custom_target(test_type_translation
    COMMAND $<TARGET_FILE:hle_integration_tests> --gtest_filter="TypeTranslatorTest.*:TypeTranslatorIntegrationTest.*"
    DEPENDS hle_integration_tests
    COMMENT "Running Type Translator tests"
)

# Target to run all HLE integration tests
add_custom_target(test_hle_integration
    COMMAND $<TARGET_FILE:hle_integration_tests>
    DEPENDS hle_integration_tests
    COMMENT "Running all HLE integration tests (these should FAIL until implementation exists)"
)

# Add to parent test suite
add_dependencies(test_all hle_integration_tests)

# Install test executable if requested
if(INSTALL_TESTS)
    install(TARGETS hle_integration_tests
        RUNTIME DESTINATION bin/tests
        COMPONENT tests
    )
endif()

# Test configuration
set_target_properties(hle_integration_tests PROPERTIES
    FOLDER "tests/unit/hle_integration"
    OUTPUT_NAME "hle_integration_tests"
)

# Enable additional warnings for test code
if(MSVC)
    target_compile_options(hle_integration_tests PRIVATE /W4)
else()
    target_compile_options(hle_integration_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Custom target to demonstrate expected test failures
add_custom_target(demonstrate_test_failures
    COMMAND echo "Running HLE integration tests - these SHOULD FAIL until implementation exists:"
    COMMAND $<TARGET_FILE:hle_integration_tests> || echo "EXPECTED: Tests failed because implementations are missing"
    DEPENDS hle_integration_tests
    COMMENT "Demonstrating expected test failures in TDD red phase"
)

# Add metadata for test documentation
set_property(TARGET hle_integration_tests PROPERTY
    DESCRIPTION "Critical HLE integration tests that bridge LDN HLE service to multiplayer backends"
)

set_property(TARGET hle_integration_tests PROPERTY
    TEST_LABELS "critical;hle;integration;bridge;tdd;red_phase"
)

# Test timeout settings
set_property(TARGET hle_integration_tests PROPERTY
    TIMEOUT 60
)

message(STATUS "HLE Integration Tests configured - these tests will FAIL until Phase 6 implementation is complete")
message(STATUS "Test categories: Backend Interface, Backend Factory, LDN Bridge, Error Mapping, Type Translation")