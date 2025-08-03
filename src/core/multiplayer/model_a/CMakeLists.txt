# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
    room_client.cpp
    p2p_network.cpp
    relay_protocol.cpp
    relay_client.cpp
)

set(HEADERS
    i_websocket_connection.h
    room_types.h
    room_messages.h
    room_client.h
    p2p_types.h
    i_p2p_network.h
    p2p_network.h
    relay_types.h
    i_relay_client.h
    relay_protocol.h
    relay_client.h
)

add_library(sudachi_multiplayer_model_a STATIC ${SOURCES} ${HEADERS})

target_include_directories(sudachi_multiplayer_model_a
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(sudachi_multiplayer_model_a
    PUBLIC
        sudachi_multiplayer_common
        websocketpp::websocketpp
        OpenSSL::SSL
        OpenSSL::Crypto
        # cpp-libp2p will be added via FetchContent
)

target_compile_features(sudachi_multiplayer_model_a PUBLIC cxx_std_20) # cpp-libp2p requires C++20

# Add tests subdirectory if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()