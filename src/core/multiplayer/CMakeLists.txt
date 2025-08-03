# SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(common)
add_subdirectory(model_a)
add_subdirectory(model_b)

# Create the main multiplayer library that combines all components
add_library(sudachi_multiplayer INTERFACE)
target_link_libraries(sudachi_multiplayer INTERFACE
    sudachi_multiplayer_common
    sudachi_multiplayer_model_a
    sudachi_multiplayer_model_b
)