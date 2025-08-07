// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/multiplayer/common/network_security.h"
#include <gtest/gtest.h>

using namespace Core::Multiplayer::Security;

TEST(NetworkInputValidatorUtf8Test, RejectsOverlongEncoding) {
    std::string data("\xC0\xAF", 2);
    auto result = NetworkInputValidator::ValidateJsonMessage(data);
    EXPECT_FALSE(result.is_valid);
}

TEST(NetworkInputValidatorUtf8Test, RejectsInvalidContinuation) {
    std::string data("\xE2\x28\xA1", 3);
    auto result = NetworkInputValidator::ValidateJsonMessage(data);
    EXPECT_FALSE(result.is_valid);
}

TEST(NetworkInputValidatorUtf8Test, RejectsSurrogateRange) {
    std::string data("\xED\xA0\x80", 3);
    auto result = NetworkInputValidator::ValidateJsonMessage(data);
    EXPECT_FALSE(result.is_valid);
}

TEST(NetworkInputValidatorUtf8Test, AcceptsValidUtf8) {
    std::string data = "{\"msg\":\"hello \xF0\x9F\x98\x81\"}";
    auto result = NetworkInputValidator::ValidateJsonMessage(data);
    EXPECT_TRUE(result.is_valid);
}

