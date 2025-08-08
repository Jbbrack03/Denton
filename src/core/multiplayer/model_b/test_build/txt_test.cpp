// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Simple test to verify TXT record implementation works

#include <iostream>
#include <cassert>
#include "../mdns_txt_records.h"
#include "../mdns_discovery.h"  // For GameSessionInfo

using namespace Core::Multiplayer::ModelB;
using namespace Core::Multiplayer;

int main() {
    std::cout << "Running TXT Records implementation test..." << std::endl;
    
    try {
        // Test TxtRecordBuilder
        TxtRecordBuilder builder;
        
        // Add records
        auto result = builder.AddRecord("game_id", "0100000000010000");
        assert(result == ErrorCode::Success);
        
        result = builder.AddRecord("version", "1.0");
        assert(result == ErrorCode::Success);
        
        result = builder.AddRecord("players", "2");
        assert(result == ErrorCode::Success);
        
        result = builder.AddRecord("max_players", "4");
        assert(result == ErrorCode::Success);
        
        result = builder.AddRecord("has_password", "false");
        assert(result == ErrorCode::Success);
        
        // Check record exists
        assert(builder.HasRecord("game_id"));
        assert(builder.GetRecord("game_id") == "0100000000010000");
        assert(builder.GetRecordCount() == 5);
        
        // Update a record
        result = builder.UpdateRecord("players", "3");
        assert(result == ErrorCode::Success);
        assert(builder.GetRecord("players") == "3");
        
        // Get binary representation
        auto binary = builder.ToBinary();
        assert(!binary.empty());
        std::cout << "Binary size: " << binary.size() << " bytes" << std::endl;
        
        // Test TxtRecordParser
        TxtRecordParser parser(binary);
        
        // Verify parsed data
        assert(parser.HasRecord("game_id"));
        assert(parser.GetRecord("game_id") == "0100000000010000");
        assert(parser.HasRecord("version"));
        assert(parser.GetRecord("version") == "1.0");
        assert(parser.HasRecord("players"));
        assert(parser.GetRecord("players") == "3");
        assert(parser.GetRecordCount() == 5);
        
        // Test validation with GameSessionInfo
        GameSessionInfo session_info;
        session_info.game_id = "0100000000010000";
        session_info.version = "1.0";
        session_info.current_players = 2;
        session_info.max_players = 4;
        session_info.has_password = false;
        session_info.host_name = "TestHost";
        session_info.session_id = "test-session-123";
        
        auto valid_records = TxtRecordBuilder::CreateGameSessionTxtRecords(session_info);
        std::cout << "Valid records count: " << valid_records.GetRecordCount() << std::endl;
        assert(valid_records.GetRecordCount() >= 5);  // At least 5 records
        
        // Test edge cases
        TxtRecordBuilder edge_builder;
        
        // Empty key
        result = edge_builder.AddRecord("", "value");
        assert(result == ErrorCode::InvalidParameter);
        
        // Empty value is allowed
        result = edge_builder.AddRecord("empty", "");
        assert(result == ErrorCode::Success);
        
        // Long key (over 63 chars)
        std::string long_key(64, 'a');
        result = edge_builder.AddRecord(long_key, "value");
        assert(result == ErrorCode::InvalidParameter);
        
        // Long value (over 255 chars)
        std::string long_value(256, 'b');
        result = edge_builder.AddRecord("key", long_value);
        assert(result == ErrorCode::InvalidParameter);
        
        std::cout << "\nAll TXT Records tests PASSED!" << std::endl;
        std::cout << "✓ Builder functionality works correctly" << std::endl;
        std::cout << "✓ Parser functionality works correctly" << std::endl;
        std::cout << "✓ Binary serialization/deserialization works" << std::endl;
        std::cout << "✓ Validation and edge cases handled properly" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
