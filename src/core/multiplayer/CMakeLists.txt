# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(common)
add_subdirectory(model_a)
add_subdirectory(model_b)

# HLE Integration library - connects multiplayer backends to LDN service
add_library(sudachi_multiplayer_hle_integration
    multiplayer_backend.h
    backend_factory.h
    backend_factory.cpp
    error_code_mapper.h
    error_code_mapper.cpp
    type_translator.h
    type_translator.cpp
    ldn_service_bridge.h
    ldn_service_bridge.cpp
)

target_link_libraries(sudachi_multiplayer_hle_integration PUBLIC
    sudachi_multiplayer_common
    sudachi_multiplayer_model_a
    sudachi_multiplayer_model_b
)

# Create the main multiplayer library that combines all components
add_library(sudachi_multiplayer INTERFACE)
target_link_libraries(sudachi_multiplayer INTERFACE
    sudachi_multiplayer_common
    sudachi_multiplayer_model_a
    sudachi_multiplayer_model_b
    sudachi_multiplayer_hle_integration
)
