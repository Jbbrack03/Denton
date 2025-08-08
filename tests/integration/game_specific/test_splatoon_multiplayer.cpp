// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "../../../src/core/multiplayer/type_translator.cpp"

using namespace Core::Multiplayer::HLE;

// Splatoon 3 requires correct security mode and local communication version
TEST(SplatoonMultiplayerTest, MapsSecurityAndVersionFields) {
    ConcreteTypeTranslator translator;

    InternalNetworkInfo internal{};
    internal.network_name = "Splatoon3";
    internal.local_communication_id = 0x12345678ULL;
    internal.channel = 1;
    internal.node_count = 1;
    internal.node_count_max = 8;
    internal.link_level = 0;
    internal.network_mode = static_cast<uint8_t>(Service::LDN::PackedNetworkType::Ldn);
    internal.session_id = std::vector<uint8_t>(sizeof(Service::LDN::SessionId), 0x42);
    internal.security_mode = static_cast<uint8_t>(Service::LDN::SecurityMode::Retail);
    internal.local_communication_version = 0x0102;
    internal.nodes.push_back({0, "Host", {0,0,0,0,0,0}, {0,0,0,0}, true, internal.local_communication_version});

    auto ldn = translator.ToLdnNetworkInfo(internal);
    EXPECT_EQ(ldn.ldn.security_mode, Service::LDN::SecurityMode::Retail);
    EXPECT_EQ(ldn.common.network_type, Service::LDN::PackedNetworkType::Ldn);
    EXPECT_EQ(ldn.ldn.nodes[0].local_communication_version,
              static_cast<int16_t>(internal.local_communication_version));

    auto roundtrip = translator.FromLdnNetworkInfo(ldn);
    EXPECT_EQ(roundtrip.security_mode, internal.security_mode);
    EXPECT_EQ(roundtrip.network_mode, internal.network_mode);
    EXPECT_EQ(roundtrip.local_communication_version, internal.local_communication_version);
}

