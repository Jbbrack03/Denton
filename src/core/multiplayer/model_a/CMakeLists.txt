# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# cpp-libp2p Integration via FetchContent
include(FetchContent)

# Set C++20 requirement for cpp-libp2p
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# cpp-libp2p uses Hunter package manager internally
# Configure Hunter cache to speed up builds
set(HUNTER_CACHE_SERVERS "https://github.com/cpp-pm/hunter-cache" CACHE STRING "Hunter cache servers")

# Fetch cpp-libp2p
FetchContent_Declare(
    cpp-libp2p
    GIT_REPOSITORY https://github.com/libp2p/cpp-libp2p
    GIT_TAG v0.1.0  # Use specific release tag for stability
    CMAKE_ARGS 
        -DTESTING=OFF 
        -DEXAMPLES=OFF
        -DCLANG_TIDY=OFF  # Disable for faster builds
)

# Set Hunter configuration before making cpp-libp2p available
set(HUNTER_ROOT "${CMAKE_BINARY_DIR}/_hunter" CACHE PATH "Hunter root directory")

FetchContent_MakeAvailable(cpp-libp2p)

set(SOURCES
    room_client.cpp
    p2p_network.cpp
    libp2p_p2p_network.cpp
    p2p_network_factory.cpp
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
    libp2p_p2p_network.h
    p2p_network_factory.h
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
        # cpp-libp2p libraries (corrected linking)
        p2p::p2p_basic_host
        p2p::p2p_tcp_transport  
        p2p::p2p_websocket_transport
        p2p::p2p_mplex
        p2p::p2p_noise
        p2p::p2p_autonat
        p2p::p2p_relay
)

target_compile_features(sudachi_multiplayer_model_a PUBLIC cxx_std_20) # cpp-libp2p requires C++20

# Platform-specific system dependencies for cpp-libp2p
if(UNIX AND NOT APPLE)
    # Linux dependencies
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSL REQUIRED openssl)
    target_link_libraries(sudachi_multiplayer_model_a PUBLIC ${LIBSSL_LIBRARIES})
elseif(WIN32)
    # Windows dependencies handled by vcpkg
    find_package(Boost REQUIRED COMPONENTS system filesystem thread)
    target_link_libraries(sudachi_multiplayer_model_a PUBLIC 
        Boost::system 
        Boost::filesystem 
        Boost::thread
        ws2_32
        mswsock
    )
elseif(APPLE)
    # macOS dependencies
    find_package(Boost REQUIRED COMPONENTS system filesystem thread)
    target_link_libraries(sudachi_multiplayer_model_a PUBLIC 
        Boost::system 
        Boost::filesystem 
        Boost::thread
    )
endif()

# Add tests subdirectory if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()