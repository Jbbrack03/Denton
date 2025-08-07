# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

if(ENABLE_TESTING)
    # Error handling framework integration tests
    add_executable(test_error_handling_integration
        test_error_code_integration.cpp
        test_ui_error_integration.cpp
        test_error_context_enhancement.cpp
        test_cross_component_error_propagation.cpp
    )

    target_link_libraries(test_error_handling_integration
        PRIVATE
            sudachi_multiplayer_common
            gtest_main
            gmock_main
    )

    target_include_directories(test_error_handling_integration
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    # Register the test with CTest
    add_test(NAME ErrorHandlingIntegrationTests COMMAND test_error_handling_integration)
    
    # Set test properties
    set_tests_properties(ErrorHandlingIntegrationTests PROPERTIES
        TIMEOUT 60
        LABELS "integration;error_handling"
    )

    add_executable(test_network_security_utf8
        test_utf8_validation.cpp
    )

    target_link_libraries(test_network_security_utf8
        PRIVATE
            sudachi_multiplayer_common
            gtest_main
    )

    target_include_directories(test_network_security_utf8
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    add_test(NAME NetworkSecurityUtf8Tests COMMAND test_network_security_utf8)
endif()