// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mdns_txt_records.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace Core::Multiplayer::ModelB {

// TxtRecordBuilder implementation
struct TxtRecordBuilder::Impl {
    std::unordered_map<std::string, std::string> records;
    size_t total_size = 0;
};

TxtRecordBuilder::TxtRecordBuilder() : impl_(std::make_unique<Impl>()) {}

TxtRecordBuilder::TxtRecordBuilder(TxtRecordBuilder&& other) noexcept 
    : impl_(std::move(other.impl_)) {}

TxtRecordBuilder& TxtRecordBuilder::operator=(TxtRecordBuilder&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

TxtRecordBuilder::~TxtRecordBuilder() = default;

ErrorCode TxtRecordBuilder::AddRecord(const std::string& key, const std::string& value) {
    // Validate key
    if (!TxtRecordValidator::IsValidKey(key)) {
        return ErrorCode::InvalidParameter;
    }
    
    // Validate value
    if (!TxtRecordValidator::IsValidValue(key, value)) {
        return ErrorCode::InvalidParameter;
    }
    
    // Check if record would exceed size limits
    size_t record_size = key.length() + value.length() + 2; // +1 for '=', +1 for length byte
    if (record_size > TxtRecordConstants::kMaxRecordLength) {
        return ErrorCode::InvalidParameter;
    }
    
    if (impl_->total_size + record_size > TxtRecordConstants::kMaxTotalSize) {
        return ErrorCode::InvalidParameter;
    }
    
    // Add record
    if (impl_->records.find(key) == impl_->records.end()) {
        impl_->total_size += record_size;
    } else {
        // Update existing record - adjust size
        size_t old_size = key.length() + impl_->records[key].length() + 2;
        impl_->total_size = impl_->total_size - old_size + record_size;
    }
    
    impl_->records[key] = value;
    return ErrorCode::Success;
}

ErrorCode TxtRecordBuilder::UpdateRecord(const std::string& key, const std::string& value) {
    if (impl_->records.find(key) == impl_->records.end()) {
        return ErrorCode::InvalidParameter; // Use available error code
    }
    return AddRecord(key, value); // AddRecord handles updates
}

ErrorCode TxtRecordBuilder::RemoveRecord(const std::string& key) {
    auto it = impl_->records.find(key);
    if (it == impl_->records.end()) {
        return ErrorCode::InvalidParameter; // Use available error code
    }
    
    size_t record_size = key.length() + it->second.length() + 2;
    impl_->total_size -= record_size;
    impl_->records.erase(it);
    return ErrorCode::Success;
}

void TxtRecordBuilder::Clear() {
    impl_->records.clear();
    impl_->total_size = 0;
}

bool TxtRecordBuilder::HasRecord(const std::string& key) const {
    return impl_->records.find(key) != impl_->records.end();
}

std::string TxtRecordBuilder::GetRecord(const std::string& key) const {
    auto it = impl_->records.find(key);
    return it != impl_->records.end() ? it->second : "";
}

std::unordered_map<std::string, std::string> TxtRecordBuilder::GetAllRecords() const {
    return impl_->records;
}

bool TxtRecordBuilder::IsEmpty() const {
    return impl_->records.empty();
}

size_t TxtRecordBuilder::GetRecordCount() const {
    return impl_->records.size();
}

size_t TxtRecordBuilder::GetTotalSize() const {
    return impl_->total_size;
}

std::vector<uint8_t> TxtRecordBuilder::ToBinary() const {
    std::vector<uint8_t> data;
    data.reserve(impl_->total_size);

    for (const auto& record : impl_->records) {
        std::string record_str = record.first + "=" + record.second;
        if (record_str.length() > 255) {
            continue; // Skip oversized records
        }
        
        data.push_back(static_cast<uint8_t>(record_str.length()));
        data.insert(data.end(), record_str.begin(), record_str.end());
    }
    
    return data;
}

TxtRecordBuilder TxtRecordBuilder::CreateGameSessionTxtRecords(const GameSessionInfo& session_info) {
    TxtRecordBuilder builder;
    
    builder.AddRecord(TxtRecordConstants::kGameId, session_info.game_id);
    builder.AddRecord(TxtRecordConstants::kVersion, session_info.version);
    builder.AddRecord(TxtRecordConstants::kPlayers, std::to_string(session_info.current_players));
    builder.AddRecord(TxtRecordConstants::kMaxPlayers, std::to_string(session_info.max_players));
    builder.AddRecord(TxtRecordConstants::kHasPassword, 
                     session_info.has_password ? TxtRecordConstants::kBooleanTrue : TxtRecordConstants::kBooleanFalse);
    
    if (!session_info.host_name.empty()) {
        builder.AddRecord(TxtRecordConstants::kHostName, session_info.host_name);
    }
    if (!session_info.session_id.empty()) {
        builder.AddRecord(TxtRecordConstants::kSessionId, session_info.session_id);
    }
    
    return std::move(builder);
}

std::pair<ErrorCode, TxtRecordBuilder> TxtRecordBuilder::FromMap(
    const std::unordered_map<std::string, std::string>& records) {
    TxtRecordBuilder builder;
    builder.impl_->records.reserve(records.size());
    for (const auto& record : records) {
        ErrorCode ec = builder.AddRecord(record.first, record.second);
        if (ec != ErrorCode::Success) {
            return {ec, TxtRecordBuilder{}};
        }
    }
    return {ErrorCode::Success, std::move(builder)};
}

// TxtRecordParser implementation
struct TxtRecordParser::Impl {
    std::unordered_map<std::string, std::string> records;
    bool is_valid = false;
};

TxtRecordParser::TxtRecordParser(const std::vector<uint8_t>& binary_data) : impl_(std::make_unique<Impl>()) {
    impl_->is_valid = ParseBinaryData(binary_data);
}

TxtRecordParser::TxtRecordParser(const std::unordered_map<std::string, std::string>& records) : impl_(std::make_unique<Impl>()) {
    impl_->records = records;
    impl_->is_valid = true;
}

TxtRecordParser::TxtRecordParser(TxtRecordParser&& other) noexcept 
    : impl_(std::move(other.impl_)) {}

TxtRecordParser& TxtRecordParser::operator=(TxtRecordParser&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

TxtRecordParser::~TxtRecordParser() = default;

bool TxtRecordParser::ParseBinaryData(const std::vector<uint8_t>& binary_data) {
    if (binary_data.empty()) {
        return false;
    }

    impl_->records.clear();
    impl_->records.reserve(binary_data.size() / 2);

    size_t offset = 0;
    size_t total_size = 0;
    while (offset < binary_data.size()) {
        if (offset >= binary_data.size()) {
            return false;
        }

        uint8_t length = binary_data[offset];
        if (length == 0) {
            offset++;
            continue; // Skip zero-length records
        }

        if (offset + 1 + length > binary_data.size()) {
            return false; // Truncated record
        }

        total_size += length + 1;
        if (total_size > TxtRecordConstants::kMaxTotalSize) {
            return false;
        }

        std::string record_data(binary_data.begin() + offset + 1,
                               binary_data.begin() + offset + 1 + length);

        // Find '=' separator
        size_t equals_pos = record_data.find('=');
        if (equals_pos == std::string::npos) {
            return false; // Missing separator
        }

        std::string key = record_data.substr(0, equals_pos);
        std::string value = record_data.substr(equals_pos + 1);

        if (!TxtRecordValidator::IsValidKey(key) ||
            !TxtRecordValidator::IsValidValue(key, value)) {
            return false;
        }

        auto [it, inserted] = impl_->records.emplace(std::move(key), std::move(value));
        if (!inserted) {
            return false; // Duplicate key
        }

        offset += length + 1;
    }

    return true;
}

bool TxtRecordParser::IsValid() const {
    return impl_->is_valid;
}

bool TxtRecordParser::HasRecord(const std::string& key) const {
    return impl_->records.find(key) != impl_->records.end();
}

std::string TxtRecordParser::GetRecord(const std::string& key) const {
    auto it = impl_->records.find(key);
    return it != impl_->records.end() ? it->second : "";
}

std::unordered_map<std::string, std::string> TxtRecordParser::GetAllRecords() const {
    return impl_->records;
}

size_t TxtRecordParser::GetRecordCount() const {
    return impl_->records.size();
}

TxtRecordParser TxtRecordParser::ParseTxtRecords(const std::vector<uint8_t>& binary_data) {
    return TxtRecordParser(binary_data);
}

std::pair<ErrorCode, TxtRecordParser> TxtRecordParser::FromMap(
    const std::unordered_map<std::string, std::string>& records) {
    TxtRecordParser parser(std::unordered_map<std::string, std::string>{});
    parser.impl_->records.reserve(records.size());

    size_t total_size = 0;
    for (const auto& record : records) {
        if (!TxtRecordValidator::IsValidKey(record.first) ||
            !TxtRecordValidator::IsValidValue(record.first, record.second)) {
            return {ErrorCode::InvalidParameter, TxtRecordParser(std::unordered_map<std::string, std::string>{})};
        }

        size_t record_size = record.first.length() + record.second.length() + 2;
        if (record_size > TxtRecordConstants::kMaxRecordLength ||
            total_size + record_size > TxtRecordConstants::kMaxTotalSize) {
            return {ErrorCode::InvalidParameter, TxtRecordParser(std::unordered_map<std::string, std::string>{})};
        }

        parser.impl_->records.emplace(record.first, record.second);
        total_size += record_size;
    }

    parser.impl_->is_valid = true;
    return {ErrorCode::Success, std::move(parser)};
}

// TxtRecordValidator implementation
ErrorCode TxtRecordValidator::ValidateGameSessionTxtRecords(const TxtRecordParser& parser) {
    if (!parser.IsValid()) {
        return ErrorCode::InvalidParameter;
    }
    
    // Check required fields
    std::vector<std::string> required_fields = {
        TxtRecordConstants::kGameId,
        TxtRecordConstants::kVersion,
        TxtRecordConstants::kPlayers,
        TxtRecordConstants::kMaxPlayers,
        TxtRecordConstants::kHasPassword
    };
    
    for (const auto& field : required_fields) {
        if (!parser.HasRecord(field)) {
            return ErrorCode::InvalidParameter;
        }
        if (parser.GetRecord(field).empty()) {
            return ErrorCode::InvalidParameter;
        }
    }
    
    return ErrorCode::Success;
}

ErrorCode TxtRecordValidator::ValidateRecord(const std::string& key, const std::string& value) {
    if (!IsValidKey(key)) {
        return ErrorCode::InvalidParameter;
    }
    if (!IsValidValue(key, value)) {
        return ErrorCode::InvalidParameter;  
    }
    return ErrorCode::Success;
}

bool TxtRecordValidator::IsValidKey(const std::string& key) {
    if (key.empty() || key.length() > GetMaxKeyLength()) {
        return false;
    }
    
    // Keys should not contain control characters or spaces
    for (char c : key) {
        if (std::iscntrl(c) || c == ' ') {
            return false;
        }
    }
    
    return true;
}

bool TxtRecordValidator::IsValidValue(const std::string& key, const std::string& value) {
    if (value.length() > GetMaxValueLength()) {
        return false;
    }
    
    // Required fields cannot be empty
    if (IsRequiredField(key) && value.empty()) {
        return false;
    }
    
    // Type specific validation
    if (key == TxtRecordConstants::kPlayers || key == TxtRecordConstants::kMaxPlayers) {
        try {
            int num = std::stoi(value);
            return num >= 0;
        } catch (...) {
            return false;
        }
    }
    
    if (key == TxtRecordConstants::kHasPassword) {
        return value == TxtRecordConstants::kBooleanTrue || value == TxtRecordConstants::kBooleanFalse;
    }
    
    return true;
}

bool TxtRecordValidator::IsRequiredField(const std::string& key) {
    return key == TxtRecordConstants::kGameId ||
           key == TxtRecordConstants::kVersion ||
           key == TxtRecordConstants::kPlayers ||
           key == TxtRecordConstants::kMaxPlayers ||
           key == TxtRecordConstants::kHasPassword;
}

size_t TxtRecordValidator::GetMaxKeyLength() {
    return TxtRecordConstants::kMaxKeyLength;
}

size_t TxtRecordValidator::GetMaxValueLength() {
    return TxtRecordConstants::kMaxValueLength;
}

size_t TxtRecordValidator::GetMaxTotalSize() {
    return TxtRecordConstants::kMaxTotalSize;
}

} // namespace Core::Multiplayer::ModelB
