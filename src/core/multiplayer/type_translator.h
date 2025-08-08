// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>

// Forward declarations for LDN types
namespace Service::LDN {
class NetworkInfo;
class NodeInfo;
class SessionId;
class MacAddress;
class Ipv4Address;
class CreateNetworkConfig;
class SecurityParameter;
enum class State : uint32_t;
}

namespace Core::Multiplayer::HLE {

/**
 * Internal Multiplayer Types - Minimal definitions for type conversion
 */
struct InternalNetworkInfo {
    std::string network_name;
    std::vector<uint8_t> network_id;
    std::vector<uint8_t> session_id;
    uint64_t local_communication_id;
    uint16_t channel;
    uint8_t node_count;
    uint8_t node_count_max;
    int8_t link_level;  // Signal strength
    uint8_t network_mode;  // 0=None, 1=General, 2=LDN, 3=All
    std::vector<uint8_t> advertise_data;
    bool has_password;
    std::vector<uint8_t> security_parameter;
};

struct InternalNodeInfo {
    uint8_t node_id;
    std::string user_name;
    std::array<uint8_t, 6> mac_address{};
    std::array<uint8_t, 4> ipv4_address{};
    bool is_connected;
    uint16_t local_communication_version;
};

struct InternalSessionInfo {
    std::vector<uint8_t> session_id;
    uint64_t local_communication_id;
    uint16_t scene_id;
    std::string passphrase;
    uint8_t security_mode;  // 0=All, 1=Retail, 2=Debug
};

struct InternalScanResult {
    InternalNetworkInfo network;
    int8_t rssi;  // Signal strength
    uint64_t timestamp;
    std::string platform_info;  // Model A vs Model B specific info
};

/**
 * Type Translator Interface - Converts between LDN and internal types
 */
class TypeTranslator {
public:
    virtual ~TypeTranslator() = default;
    
    // Network info translations
    virtual Service::LDN::NetworkInfo ToLdnNetworkInfo(const InternalNetworkInfo& internal) = 0;
    virtual InternalNetworkInfo FromLdnNetworkInfo(const Service::LDN::NetworkInfo& ldn) = 0;
    
    // Node info translations
    virtual Service::LDN::NodeInfo ToLdnNodeInfo(const InternalNodeInfo& internal) = 0;
    virtual InternalNodeInfo FromLdnNodeInfo(const Service::LDN::NodeInfo& ldn) = 0;
    
    // Session info translations
    virtual Service::LDN::SessionId ToLdnSessionId(const std::vector<uint8_t>& internal_id) = 0;
    virtual std::vector<uint8_t> FromLdnSessionId(const Service::LDN::SessionId& ldn_id) = 0;
    
    // Scan result translations
    virtual std::vector<Service::LDN::NetworkInfo> ToLdnScanResults(
        const std::vector<InternalScanResult>& internal_results) = 0;
    virtual std::vector<InternalScanResult> FromLdnScanResults(
        const std::vector<Service::LDN::NetworkInfo>& ldn_results) = 0;
    
    // Address translations
    virtual Service::LDN::MacAddress ToLdnMacAddress(const std::array<uint8_t, 6>& internal_mac) = 0;
    virtual std::array<uint8_t, 6> FromLdnMacAddress(const Service::LDN::MacAddress& ldn_mac) = 0;

    virtual Service::LDN::Ipv4Address ToLdnIpv4Address(const std::array<uint8_t, 4>& internal_ip) = 0;
    virtual std::array<uint8_t, 4> FromLdnIpv4Address(const Service::LDN::Ipv4Address& ldn_ip) = 0;
    
    // Configuration translations
    virtual Service::LDN::CreateNetworkConfig ToLdnCreateConfig(const InternalSessionInfo& internal) = 0;
    virtual InternalSessionInfo FromLdnCreateConfig(const Service::LDN::CreateNetworkConfig& ldn) = 0;
    
    // Security parameter translations
    virtual Service::LDN::SecurityParameter ToLdnSecurityParameter(const std::vector<uint8_t>& internal) = 0;
    virtual std::vector<uint8_t> FromLdnSecurityParameter(const Service::LDN::SecurityParameter& ldn) = 0;
    
    // State translations
    virtual Service::LDN::State ToLdnState(const std::string& internal_state) = 0;
    virtual std::string FromLdnState(Service::LDN::State ldn_state) = 0;
    
    // Validation methods
    virtual bool ValidateLdnNetworkInfo(const Service::LDN::NetworkInfo& info) = 0;
    virtual bool ValidateInternalNetworkInfo(const InternalNetworkInfo& info) = 0;
};

} // namespace Core::Multiplayer::HLE
