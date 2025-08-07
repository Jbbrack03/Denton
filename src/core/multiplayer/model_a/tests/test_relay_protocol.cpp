// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <vector>

#include "../relay_protocol.h"
#include "../../common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

class RelayProtocolTest : public Test {
protected:
    void SetUp() override { protocol_handler = std::make_unique<RelayProtocol>(); }
    void TearDown() override { protocol_handler.reset(); }

    std::unique_ptr<RelayProtocol> protocol_handler;
    static constexpr size_t RELAY_HEADER_SIZE = 12;
    static constexpr uint16_t MAX_PAYLOAD_SIZE = 65535;
    static constexpr uint32_t TEST_SESSION_TOKEN = 0x12345678;
    static constexpr uint32_t TEST_SEQUENCE_NUM = 0x87654321;
};

// Ensure RelayHeader layout matches specification
TEST_F(RelayProtocolTest, RelayHeaderStructureTest) {
    EXPECT_EQ(sizeof(RelayHeader), RELAY_HEADER_SIZE);

    RelayHeader header{};
    const uint8_t* header_ptr = reinterpret_cast<const uint8_t*>(&header);
    EXPECT_EQ(reinterpret_cast<const uint8_t*>(&header.session_token) - header_ptr, 0);
    EXPECT_EQ(reinterpret_cast<const uint8_t*>(&header.payload_size) - header_ptr, 4);
    EXPECT_EQ(reinterpret_cast<const uint8_t*>(&header.flags) - header_ptr, 6);
    EXPECT_EQ(reinterpret_cast<const uint8_t*>(&header.reserved) - header_ptr, 7);
    EXPECT_EQ(reinterpret_cast<const uint8_t*>(&header.sequence_num) - header_ptr, 8);
}

// Serialize header with typical values
TEST_F(RelayProtocolTest, HeaderSerializationValidData) {
    std::vector<uint8_t> serialized =
        protocol_handler->SerializeHeader(TEST_SESSION_TOKEN, 1024, 0x01, TEST_SEQUENCE_NUM);

    ASSERT_EQ(serialized.size(), RELAY_HEADER_SIZE);
    EXPECT_EQ(serialized[0], 0x78);
    EXPECT_EQ(serialized[1], 0x56);
    EXPECT_EQ(serialized[2], 0x34);
    EXPECT_EQ(serialized[3], 0x12);
    EXPECT_EQ(serialized[4], 0x00);
    EXPECT_EQ(serialized[5], 0x04);
    EXPECT_EQ(serialized[6], 0x01);
    EXPECT_EQ(serialized[7], 0x00);
    EXPECT_EQ(serialized[8], 0x21);
    EXPECT_EQ(serialized[9], 0x43);
    EXPECT_EQ(serialized[10], 0x65);
    EXPECT_EQ(serialized[11], 0x87);
}

// Serialize header using maximum payload size
TEST_F(RelayProtocolTest, HeaderSerializationMaxPayloadSize) {
    std::vector<uint8_t> serialized =
        protocol_handler->SerializeHeader(0xFFFFFFFF, MAX_PAYLOAD_SIZE, 0xFF, 0xFFFFFFFF);

    ASSERT_EQ(serialized.size(), RELAY_HEADER_SIZE);
    EXPECT_EQ(serialized[4], 0xFF);
    EXPECT_EQ(serialized[5], 0xFF);
    EXPECT_EQ(serialized[6], 0xFF);
}

// Deserialize a valid header
TEST_F(RelayProtocolTest, HeaderDeserializationValidData) {
    std::vector<uint8_t> header_data = {
        0x78, 0x56, 0x34, 0x12,
        0x00, 0x04,
        0x01,
        0x00,
        0x21, 0x43, 0x65, 0x87};

    uint32_t session_token{};
    uint32_t sequence_num{};
    uint16_t payload_size{};
    uint8_t flags{};
    bool result = protocol_handler->DeserializeHeader(header_data, session_token,
                                                      payload_size, flags, sequence_num);
    EXPECT_TRUE(result);
    EXPECT_EQ(session_token, TEST_SESSION_TOKEN);
    EXPECT_EQ(payload_size, 1024);
    EXPECT_EQ(flags, 0x01);
    EXPECT_EQ(sequence_num, TEST_SEQUENCE_NUM);
}

// Deserialize should fail with insufficient bytes
TEST_F(RelayProtocolTest, HeaderDeserializationInsufficientData) {
    std::vector<uint8_t> short_data = {0x78, 0x56, 0x34};
    uint32_t session_token{};
    uint32_t sequence_num{};
    uint16_t payload_size{};
    uint8_t flags{};
    bool result = protocol_handler->DeserializeHeader(short_data, session_token,
                                                      payload_size, flags, sequence_num);
    EXPECT_FALSE(result);
}

// Create data message combining header and payload
TEST_F(RelayProtocolTest, CreateDataMessage) {
    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC, 0xDD};
    std::vector<uint8_t> message = protocol_handler->CreateDataMessage(
        TEST_SESSION_TOKEN, payload, TEST_SEQUENCE_NUM);

    ASSERT_EQ(message.size(), RELAY_HEADER_SIZE + payload.size());
    EXPECT_EQ(message[0], 0x78);
    EXPECT_EQ(message[4], 0x04);
    EXPECT_EQ(message[5], 0x00);
    EXPECT_EQ(message[RELAY_HEADER_SIZE], 0xAA);
    EXPECT_EQ(message[RELAY_HEADER_SIZE + 1], 0xBB);
    EXPECT_EQ(message[RELAY_HEADER_SIZE + 2], 0xCC);
    EXPECT_EQ(message[RELAY_HEADER_SIZE + 3], 0xDD);
}

// Create a control message with no payload
TEST_F(RelayProtocolTest, CreateControlMessage) {
    std::vector<uint8_t> message = protocol_handler->CreateControlMessage(
        TEST_SESSION_TOKEN, RelayProtocol::FLAG_CONTROL, TEST_SEQUENCE_NUM);

    ASSERT_EQ(message.size(), RELAY_HEADER_SIZE);
    EXPECT_EQ(message[4], 0x00);
    EXPECT_EQ(message[5], 0x00);
    EXPECT_EQ(message[6], RelayProtocol::FLAG_CONTROL);
}

// Validate header with proper data
TEST_F(RelayProtocolTest, ValidateHeaderValidData) {
    std::vector<uint8_t> valid_header = {
        0x78, 0x56, 0x34, 0x12,
        0x00, 0x04,
        0x01,
        0x00,
        0x21, 0x43, 0x65, 0x87};

    EXPECT_TRUE(protocol_handler->ValidateHeader(valid_header));
}

// Detect malformed headers
TEST_F(RelayProtocolTest, ValidateHeaderInvalidData) {
    std::vector<uint8_t> wrong_size = {0x78, 0x56, 0x34};
    EXPECT_FALSE(protocol_handler->ValidateHeader(wrong_size));
}

// Validate complete message including payload size
TEST_F(RelayProtocolTest, ValidateCompleteMessage) {
    std::vector<uint8_t> good_message = {
        0x78, 0x56, 0x34, 0x12,
        0x04, 0x00,
        0x01,
        0x00,
        0x21, 0x43, 0x65, 0x87,
        0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_TRUE(protocol_handler->ValidateMessage(good_message));

    std::vector<uint8_t> bad_message = {
        0x78, 0x56, 0x34, 0x12,
        0x08, 0x00,
        0x01,
        0x00,
        0x21, 0x43, 0x65, 0x87,
        0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_FALSE(protocol_handler->ValidateMessage(bad_message));
}

// Confirm protocol constants and limits
TEST_F(RelayProtocolTest, ProtocolLimitsAndConstants) {
    EXPECT_EQ(protocol_handler->GetHeaderSize(), RELAY_HEADER_SIZE);
    EXPECT_EQ(protocol_handler->GetMaxPayloadSize(), MAX_PAYLOAD_SIZE);
}

// Ensure little-endian serialization
TEST_F(RelayProtocolTest, EndiannessHandling) {
    std::vector<uint8_t> serialized = protocol_handler->SerializeHeader(
        0x12345678, 0x1234, 0x00, 0x87654321);
    EXPECT_EQ(serialized[0], 0x78);
    EXPECT_EQ(serialized[1], 0x56);
    EXPECT_EQ(serialized[2], 0x34);
    EXPECT_EQ(serialized[3], 0x12);
    EXPECT_EQ(serialized[4], 0x34);
    EXPECT_EQ(serialized[5], 0x12);
}

// Verify protocol flag definitions
TEST_F(RelayProtocolTest, ProtocolFlags) {
    EXPECT_EQ(RelayProtocol::FLAG_DATA, 0x00);
    EXPECT_EQ(RelayProtocol::FLAG_CONTROL, 0x80);
    EXPECT_EQ(RelayProtocol::FLAG_KEEPALIVE, 0x40);
    EXPECT_EQ(RelayProtocol::FLAG_SESSION_CREATE, 0x20);
    EXPECT_EQ(RelayProtocol::FLAG_SESSION_JOIN, 0x10);
    EXPECT_EQ(RelayProtocol::FLAG_SESSION_LEAVE, 0x08);
}

} // namespace
