// SPDX-FileCopyrightText: 2025 sudachi Emulator Project  
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <vector>
#include "relay_types.h"

namespace Core::Multiplayer::ModelA {

/**
 * Relay Protocol Header structure as specified in PRD Section 3.4
 * 12-byte header for session-based routing
 */
struct RelayHeader {
    uint32_t session_token;  // 4 bytes - unique session ID
    uint16_t payload_size;   // 2 bytes - size of following data
    uint8_t  flags;          // 1 byte  - control flags
    uint8_t  reserved;       // 1 byte  - future use
    uint32_t sequence_num;   // 4 bytes - packet sequencing
} __attribute__((packed));

static_assert(sizeof(RelayHeader) == 12, "RelayHeader must be exactly 12 bytes");

/**
 * RelayProtocol class for handling relay message serialization/deserialization
 */
class RelayProtocol {
public:
    // Protocol flag constants
    static constexpr uint8_t FLAG_DATA = 0x00;
    static constexpr uint8_t FLAG_CONTROL = 0x80;
    static constexpr uint8_t FLAG_KEEPALIVE = 0x40;
    static constexpr uint8_t FLAG_SESSION_CREATE = 0x20;
    static constexpr uint8_t FLAG_SESSION_JOIN = 0x10;
    static constexpr uint8_t FLAG_SESSION_LEAVE = 0x08;

    // Header serialization
    std::vector<uint8_t> SerializeHeader(uint32_t session_token, uint16_t payload_size, 
                                        uint8_t flags, uint32_t sequence_num);
    bool DeserializeHeader(const std::vector<uint8_t>& data, uint32_t& session_token, 
                          uint16_t& payload_size, uint8_t& flags, uint32_t& sequence_num);
    
    // Message handling
    std::vector<uint8_t> CreateDataMessage(uint32_t session_token, 
                                          const std::vector<uint8_t>& payload, 
                                          uint32_t sequence_num);
    std::vector<uint8_t> CreateControlMessage(uint32_t session_token, uint8_t control_type, 
                                             uint32_t sequence_num);
    
    // Validation
    bool ValidateHeader(const std::vector<uint8_t>& header_data);
    bool ValidateMessage(const std::vector<uint8_t>& message_data);
    size_t GetHeaderSize() const;
    size_t GetMaxPayloadSize() const;
};

} // namespace Core::Multiplayer::ModelA