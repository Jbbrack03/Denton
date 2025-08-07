#include "model_a_backend.h"

namespace Core::Multiplayer::ModelA {

bool ModelABackend::IsSupported() {
    return true;
}

ModelABackend::ModelABackend(std::shared_ptr<HLE::ConfigurationManager> config,
                             std::unique_ptr<IP2PNetwork> network)
    : config_(std::move(config)), network_(std::move(network)) {}

ErrorCode ModelABackend::Initialize() {
    initialized_ = true;
    return ErrorCode::Success;
}

ErrorCode ModelABackend::Finalize() {
    initialized_ = false;
    return ErrorCode::Success;
}

bool ModelABackend::IsInitialized() const {
    return initialized_;
}

ErrorCode ModelABackend::CreateNetwork(const Service::LDN::CreateNetworkConfig&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::DestroyNetwork() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::Connect(const Service::LDN::ConnectNetworkData&,
                                 const Service::LDN::NetworkInfo&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::Disconnect() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::Scan(std::vector<Service::LDN::NetworkInfo>&,
                              const Service::LDN::ScanFilter&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::GetNetworkInfo(Service::LDN::NetworkInfo&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::GetCurrentState(Service::LDN::State&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::OpenAccessPoint() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::CloseAccessPoint() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::OpenStation() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::CloseStation() {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::SendPacket(const std::vector<uint8_t>&, uint8_t) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::ReceivePacket(std::vector<uint8_t>&, uint8_t&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::SetAdvertiseData(const std::vector<uint8_t>&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::GetSecurityParameter(Service::LDN::SecurityParameter&) {
    return ErrorCode::NotImplemented;
}

ErrorCode ModelABackend::GetDisconnectReason(Service::LDN::DisconnectReason&) {
    return ErrorCode::NotImplemented;
}

} // namespace Core::Multiplayer::ModelA

