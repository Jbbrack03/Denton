// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "core/multiplayer/common/network_security.h"

using namespace Core::Multiplayer::Security;
using Core::Multiplayer::ErrorCode;
using json = nlohmann::json;

/**
 * Tests for NetworkInputValidator JSON structure validation
 */

// Helper to generate nested JSON object exceeding depth
static std::string CreateDeepJson(size_t depth) {
    json obj = json::object();
    for (size_t i = 0; i < depth; ++i) {
        obj = json{{"level", obj}};
    }
    return obj.dump();
}

TEST(NetworkSecurityTest, RejectsExcessiveJsonDepth) {
    // Create JSON deeper than allowed
    std::string deep_json = CreateDeepJson(MAX_JSON_DEPTH + 1);

    auto result = NetworkInputValidator::ValidateJsonMessage(deep_json);

    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(ErrorCode::InvalidMessage, result.error_code);
}

TEST(NetworkSecurityTest, RejectsLargeJsonArray) {
    json arr = json::array();
    for (size_t i = 0; i < MAX_JSON_ARRAY_SIZE + 1; ++i) {
        arr.push_back(i);
    }

    auto result = NetworkInputValidator::ValidateJsonMessage(arr.dump());

    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(ErrorCode::InvalidMessage, result.error_code);
}

TEST(NetworkSecurityTest, RejectsLargeJsonObject) {
    json obj = json::object();
    for (size_t i = 0; i < MAX_JSON_OBJECT_SIZE + 1; ++i) {
        obj["key" + std::to_string(i)] = i;
    }

    auto result = NetworkInputValidator::ValidateJsonMessage(obj.dump());

    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(ErrorCode::InvalidMessage, result.error_code);
}

