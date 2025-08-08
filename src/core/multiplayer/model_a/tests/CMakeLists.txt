# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    set(TEST_SOURCES
        # Existing room client tests
        test_room_client_connection.cpp
        test_room_client_messages.cpp
        test_room_client_reconnection.cpp
        test_room_client_thread_safety.cpp
        
        # Core P2P network tests (essential functionality)
        test_p2p_network.cpp
        
        # Additional P2P integration tests
        test_p2p_network_config.cpp
        test_p2p_connection_lifecycle.cpp
        test_nat_traversal_relay.cpp
        test_security_encryption.cpp
        test_p2p_room_server_integration.cpp
        test_p2p_performance_scalability.cpp
        
        # Relay client tests
        test_relay_protocol.cpp
        test_relay_client.cpp
    )

    add_executable(model_a_tests ${TEST_SOURCES})

    target_include_directories(model_a_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    )

    target_link_libraries(model_a_tests
        PRIVATE
            sudachi_multiplayer_model_a
            sudachi_multiplayer_common
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    # Add the tests to CTest
    include(GoogleTest)
    gtest_discover_tests(model_a_tests)
endif()
