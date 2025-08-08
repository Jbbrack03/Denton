// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

// Include all our implementation headers to verify they compile
#include "../room_client.h"
#include "../room_messages.h"
#include "../room_types.h"
#include "../p2p_network.h"
#include "../p2p_types.h"
#include "../relay_client.h"
#include "../relay_protocol.h"
#include "../relay_types.h"

// This test verifies that all our implementations compile correctly
TEST(CompilationCheck, AllHeadersCompile) {
    // If this test compiles the headers are valid; no runtime behavior
    GTEST_SKIP() << "Compilation check only";
}

// Verify basic type instantiation
TEST(CompilationCheck, BasicTypeInstantiation) {
    // Room types
    ConnectionState state = ConnectionState::Disconnected;
    EXPECT_EQ(state, ConnectionState::Disconnected);
    
    // P2P types
    MultiplayerResult result = MultiplayerResult::Success;
    EXPECT_EQ(result, MultiplayerResult::Success);
    
    // Relay types
    ConnectionType type = ConnectionType::Direct;
    EXPECT_EQ(type, ConnectionType::Direct);
}

// Verify protocol structures
TEST(CompilationCheck, ProtocolStructures) {
    // Relay header size verification
    RelayHeader header{};
    EXPECT_EQ(sizeof(header), 12u) << "RelayHeader must be exactly 12 bytes";
    
    // Verify header is packed correctly
    EXPECT_EQ(offsetof(RelayHeader, session_token), 0u);
    EXPECT_EQ(offsetof(RelayHeader, payload_size), 4u);
    EXPECT_EQ(offsetof(RelayHeader, flags), 6u);
    EXPECT_EQ(offsetof(RelayHeader, reserved), 7u);
    EXPECT_EQ(offsetof(RelayHeader, sequence_num), 8u);
}

// Simple functionality test
TEST(CompilationCheck, RelayProtocolSerialization) {
    RelayHeader header{};
    header.session_token = 0x12345678;
    header.payload_size = 1024;
    header.flags = 0x01;
    header.sequence_num = 42;
    
    std::vector<uint8_t> buffer(12);
    RelayProtocol::SerializeHeader(header, buffer.data());
    
    RelayHeader deserialized{};
    RelayProtocol::DeserializeHeader(buffer.data(), deserialized);
    
    EXPECT_EQ(header.session_token, deserialized.session_token);
    EXPECT_EQ(header.payload_size, deserialized.payload_size);
    EXPECT_EQ(header.flags, deserialized.flags);
    EXPECT_EQ(header.sequence_num, deserialized.sequence_num);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
