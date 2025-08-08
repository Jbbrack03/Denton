# Game-Specific Integration Tests for Sudachi Multiplayer
# Tests multiplayer functionality for specific Nintendo Switch games

cmake_minimum_required(VERSION 3.15)

# Find required packages
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Game-specific test sources
set(GAME_SPECIFIC_TEST_SOURCES
    test_animal_crossing_multiplayer.cpp
    test_smash_bros_multiplayer.cpp
    test_mario_kart_multiplayer.cpp
    test_splatoon_multiplayer.cpp
    game_test_framework.cpp
    mock_game_state.cpp
    network_condition_simulator.cpp
)

# Test utilities and mocks
set(GAME_TEST_UTILITIES_SOURCES
    utilities/game_test_base.cpp
    utilities/multiplayer_session_simulator.cpp
    utilities/packet_capture_analyzer.cpp
    mocks/mock_emulator_instance.cpp
    mocks/mock_game_save_state.cpp
    mocks/mock_ldn_service.cpp
)

# Create game-specific integration test executable
add_executable(sudachi_game_specific_tests
    ${GAME_SPECIFIC_TEST_SOURCES}
    ${GAME_TEST_UTILITIES_SOURCES}
)

# Link libraries
target_link_libraries(sudachi_game_specific_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GMock::gmock
    sudachi_multiplayer_common
    sudachi_multiplayer_model_a
    sudachi_multiplayer_model_b
)

# Include directories
target_include_directories(sudachi_game_specific_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_SOURCE_DIR}/src/core/multiplayer/common
)

# Test discovery
include(GoogleTest)
gtest_discover_tests(sudachi_game_specific_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_PREFIX "GameSpecific."
)

# Game data test files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Custom test targets
add_custom_target(run_game_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L "game_specific" --output-on-failure
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running game-specific multiplayer tests..."
)

# Individual game test targets
add_custom_target(test_animal_crossing
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*AnimalCrossing*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running Animal Crossing multiplayer tests..."
)

add_custom_target(test_smash_bros
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*SmashBros*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running Super Smash Bros Ultimate multiplayer tests..."
)

add_custom_target(test_mario_kart
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*MarioKart*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running Mario Kart 8 Deluxe multiplayer tests..."
)

add_custom_target(test_splatoon
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*Splatoon*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running Splatoon 3 multiplayer tests..."
)

# Performance test targets
add_custom_target(test_game_performance
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*Performance*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running game-specific performance tests..."
)

# Network condition tests
add_custom_target(test_network_conditions
    COMMAND $<TARGET_FILE:sudachi_game_specific_tests> --gtest_filter="*NetworkCondition*"
    DEPENDS sudachi_game_specific_tests
    COMMENT "Running network condition simulation tests..."
)

# Test configuration
set_target_properties(sudachi_game_specific_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add test labels for CTest filtering
set_tests_properties(GameSpecific.* PROPERTIES LABELS "game_specific;integration")
