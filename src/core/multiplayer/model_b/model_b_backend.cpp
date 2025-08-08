// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model_b_backend.h"

namespace Core::Multiplayer::ModelB {

bool ModelBBackend::IsSupported() {
    return true;
}

ModelBBackend::ModelBBackend(std::shared_ptr<HLE::ConfigurationManager> config,
                             std::shared_ptr<MdnsDiscovery> discovery)
    : config_(std::move(config)), discovery_(std::move(discovery)) {}

ErrorCode ModelBBackend::Initialize() {
    initialized_ = true;
    return ErrorCode::Success;
}

ErrorCode ModelBBackend::Finalize() {
    initialized_ = false;
    return ErrorCode::Success;
}

bool ModelBBackend::IsInitialized() const {
    return initialized_;
}

ErrorCode ModelBBackend::CreateNetwork(const Service::LDN::CreateNetworkConfig&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::DestroyNetwork() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::Connect(const Service::LDN::ConnectNetworkData&,
                                 const Service::LDN::NetworkInfo&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::Disconnect() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::Scan(std::vector<Service::LDN::NetworkInfo>&,
                              const Service::LDN::ScanFilter&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::GetNetworkInfo(Service::LDN::NetworkInfo&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::GetCurrentState(Service::LDN::State&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::OpenAccessPoint() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::CloseAccessPoint() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::OpenStation() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::CloseStation() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::SendPacket(const std::vector<uint8_t>&, uint8_t) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::ReceivePacket(std::vector<uint8_t>&, uint8_t&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::SetAdvertiseData(const std::vector<uint8_t>&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::SetStationAcceptPolicy(Service::LDN::AcceptPolicy) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::AddAcceptFilterEntry(const Service::LDN::MacAddress&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::GetSecurityParameter(Service::LDN::SecurityParameter&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelBBackend::GetDisconnectReason(Service::LDN::DisconnectReason&) {
    return ErrorCode::NotImplemented;
}

} // namespace Core::Multiplayer::ModelB

