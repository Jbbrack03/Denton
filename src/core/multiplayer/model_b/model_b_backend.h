// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "core/multiplayer/multiplayer_backend.h"
#include "mdns_discovery.h"

namespace Core::Multiplayer::ModelB {

class ModelBBackend : public HLE::MultiplayerBackend {
public:
    static bool IsSupported();

    ModelBBackend(std::shared_ptr<HLE::ConfigurationManager> config,
                  std::shared_ptr<MdnsDiscovery> discovery);

    ErrorCode Initialize() override;
    ErrorCode Finalize() override;
    bool IsInitialized() const override;

    ErrorCode CreateNetwork(const Service::LDN::CreateNetworkConfig& config) override;
    ErrorCode DestroyNetwork() override;
    ErrorCode Connect(const Service::LDN::ConnectNetworkData& connect_data,
                      const Service::LDN::NetworkInfo& network_info) override;
    ErrorCode Disconnect() override;

    ErrorCode Scan(std::vector<Service::LDN::NetworkInfo>& out_networks,
                   const Service::LDN::ScanFilter& filter) override;
    ErrorCode GetNetworkInfo(Service::LDN::NetworkInfo& out_info) override;
    ErrorCode GetCurrentState(Service::LDN::State& out_state) override;

    ErrorCode OpenAccessPoint() override;
    ErrorCode CloseAccessPoint() override;
    ErrorCode OpenStation() override;
    ErrorCode CloseStation() override;

    ErrorCode SendPacket(const std::vector<uint8_t>& data, uint8_t node_id) override;
    ErrorCode ReceivePacket(std::vector<uint8_t>& out_data, uint8_t& out_node_id) override;

    ErrorCode SetAdvertiseData(const std::vector<uint8_t>& data) override;
    ErrorCode SetStationAcceptPolicy(Service::LDN::AcceptPolicy policy) override;
    ErrorCode AddAcceptFilterEntry(const Service::LDN::MacAddress& mac) override;
    ErrorCode GetSecurityParameter(Service::LDN::SecurityParameter& out_param) override;
    ErrorCode GetDisconnectReason(Service::LDN::DisconnectReason& out_reason) override;

private:
    std::shared_ptr<HLE::ConfigurationManager> config_;
    std::shared_ptr<MdnsDiscovery> discovery_;
    bool initialized_ {false};
};

} // namespace Core::Multiplayer::ModelB

