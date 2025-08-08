// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "relay_protocol.h"
#include <cstring>
#include <algorithm>

namespace Core::Multiplayer::ModelA {

// Protocol constants
static constexpr size_t RELAY_HEADER_SIZE = 12;
static constexpr uint16_t MAX_PAYLOAD_SIZE = 65535;

// Flag constants for easy access
using namespace RelayProtocolFlags;

std::vector<uint8_t> RelayProtocol::SerializeHeader(uint32_t session_token, uint16_t payload_size,
                                                    uint8_t flags, uint32_t sequence_num) {
    std::vector<uint8_t> header(RELAY_HEADER_SIZE);
    
    // Little-endian serialization
    // Session token (4 bytes)
    header[0] = static_cast<uint8_t>(session_token & 0xFF);
    header[1] = static_cast<uint8_t>((session_token >> 8) & 0xFF);
    header[2] = static_cast<uint8_t>((session_token >> 16) & 0xFF);
    header[3] = static_cast<uint8_t>((session_token >> 24) & 0xFF);
    
    // Payload size (2 bytes)
    header[4] = static_cast<uint8_t>(payload_size & 0xFF);
    header[5] = static_cast<uint8_t>((payload_size >> 8) & 0xFF);
    
    // Flags (1 byte)
    header[6] = flags;
    
    // Reserved (1 byte) - set to 0
    header[7] = 0x00;
    
    // Sequence number (4 bytes)
    header[8] = static_cast<uint8_t>(sequence_num & 0xFF);
    header[9] = static_cast<uint8_t>((sequence_num >> 8) & 0xFF);
    header[10] = static_cast<uint8_t>((sequence_num >> 16) & 0xFF);
    header[11] = static_cast<uint8_t>((sequence_num >> 24) & 0xFF);
    
    return header;
}

bool RelayProtocol::DeserializeHeader(const std::vector<uint8_t>& data, uint32_t& session_token,
                                     uint16_t& payload_size, uint8_t& flags, uint32_t& sequence_num) {
    if (data.size() < RELAY_HEADER_SIZE) {
        return false;
    }
    
    // Little-endian deserialization
    // Session token (4 bytes)
    session_token = static_cast<uint32_t>(data[0]) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) |
                   (static_cast<uint32_t>(data[3]) << 24);
    
    // Payload size (2 bytes)
    payload_size = static_cast<uint16_t>(data[4]) |
                  (static_cast<uint16_t>(data[5]) << 8);
    
    // Flags (1 byte)
    flags = data[6];
    
    // Skip reserved byte (data[7])
    
    // Sequence number (4 bytes)
    sequence_num = static_cast<uint32_t>(data[8]) |
                  (static_cast<uint32_t>(data[9]) << 8) |
                  (static_cast<uint32_t>(data[10]) << 16) |
                  (static_cast<uint32_t>(data[11]) << 24);
    
    return true;
}

std::vector<uint8_t> RelayProtocol::CreateDataMessage(uint32_t session_token,
                                                     const std::vector<uint8_t>& payload,
                                                     uint32_t sequence_num) {
    uint16_t payload_size = static_cast<uint16_t>(std::min(payload.size(), static_cast<size_t>(MAX_PAYLOAD_SIZE)));
    
    std::vector<uint8_t> header = SerializeHeader(session_token, payload_size, FLAG_DATA, sequence_num);
    
    // Combine header and payload
    std::vector<uint8_t> message;
    message.reserve(header.size() + payload_size);
    message.insert(message.end(), header.begin(), header.end());
    message.insert(message.end(), payload.begin(), payload.begin() + payload_size);
    
    return message;
}

std::vector<uint8_t> RelayProtocol::CreateControlMessage(uint32_t session_token, uint8_t control_type,
                                                        uint32_t sequence_num) {
    // Control messages have no payload
    return SerializeHeader(session_token, 0, control_type, sequence_num);
}

bool RelayProtocol::ValidateHeader(const std::vector<uint8_t>& header_data) {
    if (header_data.size() != RELAY_HEADER_SIZE) {
        return false;
    }
    
    // Extract payload size for basic validation
    uint16_t payload_size = static_cast<uint16_t>(header_data[4]) |
                           (static_cast<uint16_t>(header_data[5]) << 8);
    
    // Check if payload size is within reasonable limits
    if (payload_size > MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    return true;
}

bool RelayProtocol::ValidateMessage(const std::vector<uint8_t>& message_data) {
    if (message_data.size() < RELAY_HEADER_SIZE) {
        return false;
    }
    
    // Extract header portion
    std::vector<uint8_t> header(message_data.begin(), message_data.begin() + RELAY_HEADER_SIZE);
    
    if (!ValidateHeader(header)) {
        return false;
    }
    
    // Extract payload size from header
    uint16_t payload_size = static_cast<uint16_t>(header[4]) |
                           (static_cast<uint16_t>(header[5]) << 8);
    
    // Check if actual message size matches expected size
    size_t expected_size = RELAY_HEADER_SIZE + payload_size;
    return message_data.size() == expected_size;
}

size_t RelayProtocol::GetHeaderSize() const {
    return RELAY_HEADER_SIZE;
}

size_t RelayProtocol::GetMaxPayloadSize() const {
    return MAX_PAYLOAD_SIZE;
}

} // namespace Core::Multiplayer::ModelA
