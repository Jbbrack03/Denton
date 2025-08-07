// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

#include "../common/error_codes.h"
#include "mdns_discovery.h" // For GameSessionInfo

namespace Core::Multiplayer::ModelB {

/**
 * TXT Record Builder
 * 
 * Creates and manages mDNS TXT records according to RFC 6763 format.
 * TXT records are used to store game session metadata in mDNS service advertisements.
 * 
 * Required TXT record fields for Sudachi LDN sessions (PRD Section 4.2.2):
 * - game_id: Unique identifier for the game
 * - version: Game version string
 * - players: Current number of players (integer)
 * - max_players: Maximum number of players (integer)
 * - has_password: Whether session requires password (boolean: "true"/"false")
 * 
 * Optional fields:
 * - host_name: Name of the host player
 * - session_id: Unique session identifier
 * - region: Geographic region code
 * - language: Language code
 * 
 * This is a stub implementation - all methods will fail until implemented
 * following TDD red phase methodology.
 */
class TxtRecordBuilder {
public:
    /**
     * Constructor - creates empty TXT record builder
     */
    TxtRecordBuilder();
    
    /**
     * Move constructor
     */
    TxtRecordBuilder(TxtRecordBuilder&& other) noexcept;
    
    /**
     * Move assignment operator
     */
    TxtRecordBuilder& operator=(TxtRecordBuilder&& other) noexcept;
    
    /**
     * Destructor
     */
    ~TxtRecordBuilder();
    
    // Delete copy constructor and copy assignment operator
    TxtRecordBuilder(const TxtRecordBuilder&) = delete;
    TxtRecordBuilder& operator=(const TxtRecordBuilder&) = delete;
    
    // Record management
    ErrorCode AddRecord(const std::string& key, const std::string& value);
    ErrorCode UpdateRecord(const std::string& key, const std::string& value);
    ErrorCode RemoveRecord(const std::string& key);
    void Clear();
    
    // Record queries
    bool HasRecord(const std::string& key) const;
    std::string GetRecord(const std::string& key) const;
    std::unordered_map<std::string, std::string> GetAllRecords() const;
    
    // State queries
    bool IsEmpty() const;
    size_t GetRecordCount() const;
    size_t GetTotalSize() const;
    
    // Serialization
    std::vector<uint8_t> ToBinary() const;
    
    // Static factory methods
    static TxtRecordBuilder CreateGameSessionTxtRecords(const GameSessionInfo& session_info);
    static std::pair<ErrorCode, TxtRecordBuilder> FromMap(const std::unordered_map<std::string, std::string>& records);

private:
    // This is a stub - implementation will be added during TDD green phase
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * TXT Record Parser
 * 
 * Parses binary TXT record data from mDNS responses back into key-value pairs.
 * Handles RFC 6763 format parsing and validation.
 * 
 * This is a stub implementation - all methods will fail until implemented
 * following TDD red phase methodology.
 */
class TxtRecordParser {
public:
    /**
     * Constructor for parsing binary data
     */
    explicit TxtRecordParser(const std::vector<uint8_t>& binary_data);
    
    /**
     * Constructor from key-value map (for testing)
     */
    explicit TxtRecordParser(const std::unordered_map<std::string, std::string>& records);
    
    /**
     * Move constructor
     */
    TxtRecordParser(TxtRecordParser&& other) noexcept;
    
    /**
     * Move assignment operator
     */
    TxtRecordParser& operator=(TxtRecordParser&& other) noexcept;
    
    /**
     * Destructor
     */
    ~TxtRecordParser();
    
    // Delete copy constructor and copy assignment operator
    TxtRecordParser(const TxtRecordParser&) = delete;
    TxtRecordParser& operator=(const TxtRecordParser&) = delete;
    
    // Parsing state
    bool IsValid() const;
    
    // Record queries
    bool HasRecord(const std::string& key) const;
    std::string GetRecord(const std::string& key) const;
    std::unordered_map<std::string, std::string> GetAllRecords() const;
    size_t GetRecordCount() const;
    
    // Static factory methods
    static TxtRecordParser ParseTxtRecords(const std::vector<uint8_t>& binary_data);
    static std::pair<ErrorCode, TxtRecordParser> FromMap(const std::unordered_map<std::string, std::string>& records);

private:
    // This is a stub - implementation will be added during TDD green phase
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal parsing helper
    bool ParseBinaryData(const std::vector<uint8_t>& binary_data);
};

/**
 * TXT Record Validator
 * 
 * Validates TXT records for compliance with Sudachi LDN requirements
 * and general mDNS standards.
 * 
 * This is a stub implementation - all methods will fail until implemented
 * following TDD red phase methodology.
 */
class TxtRecordValidator {
public:
    /**
     * Validate game session TXT records
     * Ensures all required fields are present and have valid values
     */
    static ErrorCode ValidateGameSessionTxtRecords(const TxtRecordParser& parser);
    
    /**
     * Validate individual record key-value pair
     */
    static ErrorCode ValidateRecord(const std::string& key, const std::string& value);
    
    /**
     * Validate record key format
     */
    static bool IsValidKey(const std::string& key);
    
    /**
     * Validate record value for specific key
     */
    static bool IsValidValue(const std::string& key, const std::string& value);
    
    /**
     * Check if key is a required field for game sessions
     */
    static bool IsRequiredField(const std::string& key);
    
    /**
     * Get maximum allowed length for key or value
     */
    static size_t GetMaxKeyLength();
    static size_t GetMaxValueLength();
    static size_t GetMaxTotalSize();

private:
    TxtRecordValidator() = delete; // Static class only
};

/**
 * TXT Record Constants
 */
namespace TxtRecordConstants {
    // Required field names
    constexpr const char* kGameId = "game_id";
    constexpr const char* kVersion = "version";
    constexpr const char* kPlayers = "players";
    constexpr const char* kMaxPlayers = "max_players";
    constexpr const char* kHasPassword = "has_password";
    
    // Optional field names
    constexpr const char* kHostName = "host_name";
    constexpr const char* kSessionId = "session_id";
    constexpr const char* kRegion = "region";
    constexpr const char* kLanguage = "language";
    
    // Size limits (RFC 6763)
    constexpr size_t kMaxKeyLength = 63;        // Maximum key length
    constexpr size_t kMaxValueLength = 255;     // Maximum value length  
    constexpr size_t kMaxRecordLength = 255;    // Maximum single record length
    constexpr size_t kMaxTotalSize = 65535;     // Maximum total TXT record size
    
    // Boolean values
    constexpr const char* kBooleanTrue = "true";
    constexpr const char* kBooleanFalse = "false";
}

} // namespace Core::Multiplayer::ModelB