// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>

#include "backend_factory.h"
#include "multiplayer_backend.h"

// Forward declarations for LDN types
namespace Service::LDN {
class Result;
enum class State : uint32_t;
class CreateNetworkConfig; 
class ConnectNetworkData;
class NetworkInfo;
class ScanFilter;
enum class WifiChannel : uint16_t;
class NodeLatestUpdate;
class Ipv4Address;
class NetworkConfig;
class SecurityParameter;
enum class DisconnectReason : uint32_t;
enum class AcceptPolicy : uint8_t;
class MacAddress;
}

namespace Service::LDN {

/**
 * LDN Service Bridge - The critical component that connects LDN HLE to multiplayer backends
 * This MUST be implemented to replace the legacy LANDiscovery usage in user_local_communication_service.cpp
 */
class LdnServiceBridge {
public:
    LdnServiceBridge(std::unique_ptr<Core::Multiplayer::HLE::BackendFactory> factory)
        : backend_factory_(std::move(factory)) {}
    
    virtual ~LdnServiceBridge() = default;
    
    // Core lifecycle methods - these bridge LDN IPC to backend calls
    virtual Result Initialize() = 0;
    virtual Result Finalize() = 0;
    virtual Result GetState(State& out_state) = 0;
    
    // Network management - these replace LANDiscovery methods
    virtual Result CreateNetwork(const CreateNetworkConfig& config) = 0;
    virtual Result DestroyNetwork() = 0;
    virtual Result OpenAccessPoint() = 0;
    virtual Result CloseAccessPoint() = 0;
    
    // Station management
    virtual Result OpenStation() = 0;
    virtual Result CloseStation() = 0;
    virtual Result Connect(const ConnectNetworkData& connect_data, 
                          const NetworkInfo& network_info) = 0;
    virtual Result Disconnect() = 0;
    
    // Discovery and information
    virtual Result Scan(std::vector<NetworkInfo>& out_networks, 
                       WifiChannel channel, const ScanFilter& filter) = 0;
    virtual Result GetNetworkInfo(NetworkInfo& out_info) = 0;
    virtual Result GetNetworkInfoLatestUpdate(NetworkInfo& out_info,
                                            std::vector<NodeLatestUpdate>& out_updates) = 0;
    
    // Network configuration
    virtual Result GetIpv4Address(Ipv4Address& out_address, Ipv4Address& out_subnet) = 0;
    virtual Result GetNetworkConfig(NetworkConfig& out_config) = 0;
    virtual Result GetSecurityParameter(SecurityParameter& out_param) = 0;
    virtual Result GetDisconnectReason(DisconnectReason& out_reason) = 0;
    
    // Data transmission
    virtual Result SetAdvertiseData(const std::vector<uint8_t>& data) = 0;
    virtual Result SetStationAcceptPolicy(AcceptPolicy policy) = 0;
    virtual Result AddAcceptFilterEntry(const MacAddress& mac_address) = 0;
    
    // Backend management
    virtual Result SwitchBackend(Core::Multiplayer::HLE::BackendFactory::BackendType type) = 0;
    virtual Core::Multiplayer::HLE::BackendFactory::BackendType GetCurrentBackendType() = 0;

protected:
    std::unique_ptr<Core::Multiplayer::HLE::BackendFactory> backend_factory_;
    std::unique_ptr<Core::Multiplayer::HLE::MultiplayerBackend> current_backend_;
    Core::Multiplayer::HLE::BackendFactory::BackendType current_backend_type_;
    State current_state_;
};

} // namespace Service::LDN