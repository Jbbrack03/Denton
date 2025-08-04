// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <chrono>
#include <random>

// Include the packet protocol header from the implementation
// Note: Path assumes test is built from project root
#include "core/multiplayer/common/packet_protocol.h"

using namespace Sudachi::Multiplayer;
using namespace testing;

/**
 * Test fixture for LDN Packet Protocol tests
 */
class PacketProtocolTest : public Test {
protected:
    void SetUp() override {
        // Initialize any test-specific setup
    }

    void TearDown() override {
        // Clean up after tests
    }

    // Helper to create test data
    std::vector<uint8_t> CreateTestData(size_t size) {
        std::vector<uint8_t> data(size);
        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<> dist(0, 255);
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dist(gen));
        }
        return data;
    }

    // Helper to verify packet header
    bool VerifyPacketHeader(const LdnPacketHeader& header) {
        return header.magic == 0x4C44 && // "LD"
               header.version == 1 &&
               header.payload_size <= MAX_PACKET_SIZE;
    }
};

/**
 * Test packet serialization and deserialization
 */
TEST_F(PacketProtocolTest, SerializeDeserializePacket) {
    // Create a test packet
    LdnPacket packet;
    packet.header.magic = 0x4C44;
    packet.header.version = 1;
    packet.header.session_id = 0x12345678;
    packet.header.packet_type = static_cast<uint16_t>(PacketType::Data);
    packet.header.source_node_id = 1;
    packet.header.dest_node_id = 2;
    packet.header.sequence_number = 100;
    packet.header.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    
    // Add test payload
    packet.payload = CreateTestData(1024);
    packet.header.payload_size = packet.payload.size();

    // Serialize the packet
    std::vector<uint8_t> serialized;
    ASSERT_TRUE(SerializePacket(packet, serialized));

    // Deserialize the packet
    LdnPacket deserialized;
    ASSERT_TRUE(DeserializePacket(serialized, deserialized));

    // Verify the deserialized packet matches original
    EXPECT_EQ(packet.header.magic, deserialized.header.magic);
    EXPECT_EQ(packet.header.version, deserialized.header.version);
    EXPECT_EQ(packet.header.session_id, deserialized.header.session_id);
    EXPECT_EQ(packet.header.packet_type, deserialized.header.packet_type);
    EXPECT_EQ(packet.header.source_node_id, deserialized.header.source_node_id);
    EXPECT_EQ(packet.header.dest_node_id, deserialized.header.dest_node_id);
    EXPECT_EQ(packet.header.sequence_number, deserialized.header.sequence_number);
    EXPECT_EQ(packet.header.payload_size, deserialized.header.payload_size);
    EXPECT_EQ(packet.payload, deserialized.payload);
}

/**
 * Test CRC32 checksum calculation
 */
TEST_F(PacketProtocolTest, CRC32Calculation) {
    // Test with known CRC32 values
    std::vector<uint8_t> test_data = {'H', 'e', 'l', 'l', 'o'};
    uint32_t expected_crc = 0xF7D18982; // CRC32 of "Hello"
    
    uint32_t calculated_crc = CalculateCRC32(test_data.data(), test_data.size());
    EXPECT_EQ(expected_crc, calculated_crc);

    // Test with empty data
    uint32_t empty_crc = CalculateCRC32(nullptr, 0);
    EXPECT_EQ(0x00000000, empty_crc);

    // Test with larger data
    auto large_data = CreateTestData(32768); // 32KB
    uint32_t large_crc = CalculateCRC32(large_data.data(), large_data.size());
    EXPECT_NE(0x00000000, large_crc); // Should not be zero for random data
}

/**
 * Test packet validation
 */
TEST_F(PacketProtocolTest, PacketValidation) {
    LdnPacket packet;
    
    // Test invalid magic number
    packet.header.magic = 0xFFFF;
    packet.header.version = 1;
    packet.header.payload_size = 100;
    EXPECT_FALSE(ValidatePacket(packet));

    // Test invalid version
    packet.header.magic = 0x4C44;
    packet.header.version = 99;
    EXPECT_FALSE(ValidatePacket(packet));

    // Test payload size exceeding maximum
    packet.header.version = 1;
    packet.header.payload_size = MAX_PACKET_SIZE + 1;
    EXPECT_FALSE(ValidatePacket(packet));

    // Test valid packet
    packet.header.payload_size = 1024;
    packet.payload = CreateTestData(1024);
    packet.header.source_node_id = 1;
    packet.header.dest_node_id = 2;
    packet.header.sequence_number = 1;
    packet.header.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    packet.header.crc32 = CalculateCRC32(packet.payload.data(), packet.payload.size());
    EXPECT_TRUE(ValidatePacket(packet));
}

/**
 * Test packet fragmentation for large payloads
 */
TEST_F(PacketProtocolTest, PacketFragmentation) {
    // Create large data that needs fragmentation
    size_t large_size = MAX_PACKET_SIZE * 3 + 1024;
    auto large_data = CreateTestData(large_size);

    // Fragment the data
    std::vector<LdnPacket> fragments;
    ASSERT_TRUE(FragmentData(large_data, 0x12345678, 1, 2, fragments));

    // Verify fragment count
    size_t expected_fragments = (large_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
    EXPECT_EQ(expected_fragments, fragments.size());

    // Verify each fragment
    for (size_t i = 0; i < fragments.size(); ++i) {
        const auto& fragment = fragments[i];
        EXPECT_TRUE(VerifyPacketHeader(fragment.header));
        EXPECT_EQ(static_cast<uint16_t>(PacketType::Fragment), fragment.header.packet_type);
        EXPECT_EQ(i, fragment.header.fragment_id);
        EXPECT_EQ(fragments.size(), fragment.header.total_fragments);
        
        // Verify payload size
        if (i < fragments.size() - 1) {
            EXPECT_EQ(MAX_PACKET_SIZE, fragment.payload.size());
        } else {
            EXPECT_EQ(large_size % MAX_PACKET_SIZE, fragment.payload.size());
        }
    }

    // Reassemble fragments
    std::vector<uint8_t> reassembled;
    ASSERT_TRUE(ReassembleFragments(fragments, reassembled));
    
    // Verify reassembled data matches original
    EXPECT_EQ(large_data, reassembled);
}

/**
 * Test packet type handling
 */
TEST_F(PacketProtocolTest, PacketTypes) {
    // Test each packet type
    std::vector<PacketType> types = {
        PacketType::Handshake,
        PacketType::HandshakeAck,
        PacketType::Data,
        PacketType::Ack,
        PacketType::Heartbeat,
        PacketType::Disconnect,
        PacketType::Fragment,
        PacketType::Error
    };

    for (auto type : types) {
        LdnPacket packet;
        packet.header.magic = 0x4C44;
        packet.header.version = 1;
        packet.header.packet_type = static_cast<uint16_t>(type);
        packet.header.source_node_id = 1;
        packet.header.dest_node_id = 2;
        packet.header.payload_size = 0;

        // Each packet type should be valid
        EXPECT_TRUE(ValidatePacketType(packet.header.packet_type)) 
            << "Failed for packet type: " << static_cast<int>(type);
    }

    // Test invalid packet type
    EXPECT_FALSE(ValidatePacketType(999));
}

/**
 * Performance test for CRC32 calculation
 */
TEST_F(PacketProtocolTest, CRC32Performance) {
    // Test data sizes
    std::vector<size_t> test_sizes = {1024, 4096, 16384, 32768, 65536};
    
    for (size_t size : test_sizes) {
        auto data = CreateTestData(size);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            CalculateCRC32(data.data(), data.size());
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double throughput_mbps = (size * 1000 * 8.0) / duration.count();
        
        // Verify performance meets requirement (>100MB/s)
        EXPECT_GT(throughput_mbps, 800.0) // 100MB/s = 800Mbps
            << "CRC32 throughput for " << size << " bytes: " << throughput_mbps << " Mbps";
    }
}

/**
 * Test error packet creation
 */
TEST_F(PacketProtocolTest, ErrorPacketCreation) {
    // Create error packet
    auto error_packet = CreateErrorPacket(
        0x12345678,  // session_id
        1,           // source_node
        2,           // dest_node
        ErrorCode::InvalidPacket,
        "Test error message"
    );

    EXPECT_TRUE(VerifyPacketHeader(error_packet.header));
    EXPECT_EQ(static_cast<uint16_t>(PacketType::Error), error_packet.header.packet_type);
    
    // Verify error payload
    ASSERT_GE(error_packet.payload.size(), sizeof(uint32_t));
    uint32_t error_code = *reinterpret_cast<const uint32_t*>(error_packet.payload.data());
    EXPECT_EQ(static_cast<uint32_t>(ErrorCode::InvalidPacket), error_code);
    
    // Verify error message
    std::string error_msg(
        reinterpret_cast<const char*>(error_packet.payload.data() + sizeof(uint32_t)),
        error_packet.payload.size() - sizeof(uint32_t)
    );
    EXPECT_EQ("Test error message", error_msg);
}

/**
 * Test packet ordering and sequence numbers
 */
TEST_F(PacketProtocolTest, PacketSequencing) {
    PacketSequencer sequencer;
    
    // Generate sequence numbers
    std::vector<uint32_t> sequences;
    for (int i = 0; i < 10; ++i) {
        sequences.push_back(sequencer.GetNextSequence());
    }
    
    // Verify sequences are monotonically increasing
    for (size_t i = 1; i < sequences.size(); ++i) {
        EXPECT_EQ(sequences[i-1] + 1, sequences[i]);
    }
    
    // Test sequence number wraparound
    sequencer.SetSequence(0xFFFFFFFE);
    EXPECT_EQ(0xFFFFFFFF, sequencer.GetNextSequence());
    EXPECT_EQ(0x00000000, sequencer.GetNextSequence());
    EXPECT_EQ(0x00000001, sequencer.GetNextSequence());
}

/**
 * Test endianness handling
 */
TEST_F(PacketProtocolTest, EndiannessHandling) {
    // Create packet with specific values
    LdnPacket packet;
    packet.header.magic = 0x4C44;
    packet.header.session_id = 0x12345678;
    packet.header.sequence_number = 0xABCDEF00;
    
    // Serialize
    std::vector<uint8_t> serialized;
    ASSERT_TRUE(SerializePacket(packet, serialized));
    
    // Verify little-endian encoding
    ASSERT_GE(serialized.size(), sizeof(LdnPacketHeader));
    
    // Magic number should be 0x44, 0x4C in little-endian
    EXPECT_EQ(0x44, serialized[0]);
    EXPECT_EQ(0x4C, serialized[1]);
    
    // Session ID should be 0x78, 0x56, 0x34, 0x12 in little-endian
    size_t session_offset = offsetof(LdnPacketHeader, session_id);
    EXPECT_EQ(0x78, serialized[session_offset + 0]);
    EXPECT_EQ(0x56, serialized[session_offset + 1]);
    EXPECT_EQ(0x34, serialized[session_offset + 2]);
    EXPECT_EQ(0x12, serialized[session_offset + 3]);
}