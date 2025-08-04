// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "common/error_codes.h"

// Forward declarations for LDN types
namespace Service::LDN {
class CreateNetworkConfig;
class ConnectNetworkData;
class NetworkInfo;
class ScanFilter;
enum class State : uint32_t;
class SecurityParameter;
enum class DisconnectReason : uint32_t;
}

namespace Core::Multiplayer::HLE {

/**
 * MultiplayerBackend Interface - The unified interface both Model A and B must implement
 * This is the critical missing piece that allows LDN service to use our new backends
 */
class MultiplayerBackend {
public:
    virtual ~MultiplayerBackend() = default;
    
    // Core lifecycle methods
    virtual ErrorCode Initialize() = 0;
    virtual ErrorCode Finalize() = 0;
    virtual bool IsInitialized() const = 0;
    
    // Network management
    virtual ErrorCode CreateNetwork(const Service::LDN::CreateNetworkConfig& config) = 0;
    virtual ErrorCode DestroyNetwork() = 0;
    virtual ErrorCode Connect(const Service::LDN::ConnectNetworkData& connect_data,
                             const Service::LDN::NetworkInfo& network_info) = 0;
    virtual ErrorCode Disconnect() = 0;
    
    // Discovery and information
    virtual ErrorCode Scan(std::vector<Service::LDN::NetworkInfo>& out_networks,
                          const Service::LDN::ScanFilter& filter) = 0;
    virtual ErrorCode GetNetworkInfo(Service::LDN::NetworkInfo& out_info) = 0;
    virtual ErrorCode GetCurrentState(Service::LDN::State& out_state) = 0;
    
    // Access point management
    virtual ErrorCode OpenAccessPoint() = 0;
    virtual ErrorCode CloseAccessPoint() = 0;
    virtual ErrorCode OpenStation() = 0;
    virtual ErrorCode CloseStation() = 0;
    
    // Data transmission
    virtual ErrorCode SendPacket(const std::vector<uint8_t>& data, uint8_t node_id) = 0;
    virtual ErrorCode ReceivePacket(std::vector<uint8_t>& out_data, uint8_t& out_node_id) = 0;
    
    // Configuration and status
    virtual ErrorCode SetAdvertiseData(const std::vector<uint8_t>& data) = 0;
    virtual ErrorCode GetSecurityParameter(Service::LDN::SecurityParameter& out_param) = 0;
    virtual ErrorCode GetDisconnectReason(Service::LDN::DisconnectReason& out_reason) = 0;
};

} // namespace Core::Multiplayer::HLE