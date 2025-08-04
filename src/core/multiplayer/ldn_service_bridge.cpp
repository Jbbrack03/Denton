// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ldn_service_bridge.h"

#include "error_code_mapper.h"

#include "sudachi/src/core/hle/service/ldn/ldn_types.h"
#include "sudachi/src/core/hle/service/ldn/ldn_results.h"
#include "sudachi/src/core/hle/result.h"

namespace Service::LDN {

/**
 * Concrete LDN Service Bridge Implementation
 * Minimal implementation to make tests pass
 */
class ConcreteLdnServiceBridge : public LdnServiceBridge {
public:
    using LdnServiceBridge::LdnServiceBridge;
    
    Result Initialize() override {
        if (current_state_ != State::None) {
            return ResultBadState;
        }
        
        // Create backend using factory
        auto backend_type = backend_factory_->GetPreferredBackend();
        current_backend_ = backend_factory_->CreateBackend(backend_type);
        
        if (!current_backend_) {
            return ResultInternalError;  // Backend creation failed
        }
        
        current_backend_type_ = backend_type;
        
        // Initialize the backend
        auto error = current_backend_->Initialize();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::Initialized;
        return ResultSuccess;
    }
    
    Result Finalize() override {
        if (current_backend_) {
            current_backend_->Finalize();
            current_backend_.reset();
        }
        current_state_ = State::None;
        return ResultSuccess;
    }
    
    Result GetState(State& out_state) override {
        out_state = current_state_;
        return ResultSuccess;
    }
    
    Result CreateNetwork(const CreateNetworkConfig& config) override {
        if (current_state_ != State::AccessPointOpened) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->CreateNetwork(config);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::AccessPointCreated;
        return ResultSuccess;
    }
    
    Result DestroyNetwork() override {
        if (current_state_ != State::AccessPointCreated) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->DestroyNetwork();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::AccessPointOpened;
        return ResultSuccess;
    }
    
    Result OpenAccessPoint() override {
        if (current_state_ != State::Initialized) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->OpenAccessPoint();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::AccessPointOpened;
        return ResultSuccess;
    }
    
    Result CloseAccessPoint() override {
        if (current_state_ != State::AccessPointOpened && 
            current_state_ != State::AccessPointCreated) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        // Destroy network first if it exists
        if (current_state_ == State::AccessPointCreated) {
            current_backend_->DestroyNetwork();
        }
        
        auto error = current_backend_->CloseAccessPoint();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::Initialized;
        return ResultSuccess;
    }
    
    Result OpenStation() override {
        if (current_state_ != State::Initialized) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->OpenStation();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::StationOpened;
        return ResultSuccess;
    }
    
    Result CloseStation() override {
        if (current_state_ != State::StationOpened && 
            current_state_ != State::StationConnected) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        // Disconnect first if connected
        if (current_state_ == State::StationConnected) {
            current_backend_->Disconnect();
        }
        
        auto error = current_backend_->CloseStation();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::Initialized;
        return ResultSuccess;
    }
    
    Result Connect(const ConnectNetworkData& connect_data, 
                   const NetworkInfo& network_info) override {
        if (current_state_ != State::StationOpened) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->Connect(connect_data, network_info);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::StationConnected;
        return ResultSuccess;
    }
    
    Result Disconnect() override {
        if (current_state_ != State::StationConnected) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->Disconnect();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        current_state_ = State::StationOpened;
        return ResultSuccess;
    }
    
    Result Scan(std::vector<NetworkInfo>& out_networks, 
                WifiChannel channel, const ScanFilter& filter) override {
        if (current_state_ != State::Initialized && 
            current_state_ != State::StationOpened) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->Scan(out_networks, filter);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Result GetNetworkInfo(NetworkInfo& out_info) override {
        if (current_state_ != State::AccessPointCreated && 
            current_state_ != State::StationConnected) {
            return ResultBadState;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->GetNetworkInfo(out_info);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Result GetNetworkInfoLatestUpdate(NetworkInfo& out_info,
                                    std::vector<NodeLatestUpdate>& out_updates) override {
        // First get basic network info
        auto result = GetNetworkInfo(out_info);
        if (result != ResultSuccess) {
            return result;
        }
        
        // Clear and populate node updates (implementation will be enhanced)
        out_updates.clear();
        // TODO: Implement actual node update tracking
        
        return ResultSuccess;
    }
    
    Result GetIpv4Address(Ipv4Address& out_address, Ipv4Address& out_subnet) override {
        // Return placeholder IP addresses - implementation will be enhanced
        out_address = {192, 168, 1, 100};
        out_subnet = {255, 255, 255, 0};
        return ResultSuccess;
    }
    
    Result GetNetworkConfig(NetworkConfig& out_config) override {
        // Return default config - implementation will be enhanced
        out_config = {};
        return ResultSuccess;
    }
    
    Result GetSecurityParameter(SecurityParameter& out_param) override {
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->GetSecurityParameter(out_param);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Result GetDisconnectReason(DisconnectReason& out_reason) override {
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->GetDisconnectReason(out_reason);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Result SetAdvertiseData(const std::vector<uint8_t>& data) override {
        if (current_state_ != State::AccessPointCreated) {
            return ResultBadState;
        }
        
        if (data.size() > AdvertiseDataSizeMax) {
            return ResultAdvertiseDataTooLarge;
        }
        
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        auto error = current_backend_->SetAdvertiseData(data);
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Result SetStationAcceptPolicy(AcceptPolicy policy) override {
        // Implementation placeholder - will be enhanced
        return ResultSuccess;
    }
    
    Result AddAcceptFilterEntry(const MacAddress& mac_address) override {
        // Implementation placeholder - will be enhanced
        return ResultSuccess;
    }
    
    Result SwitchBackend(Core::Multiplayer::HLE::BackendFactory::BackendType type) override {
        if (current_state_ != State::Initialized) {
            return ResultBadState;
        }
        
        // Finalize current backend
        if (current_backend_) {
            current_backend_->Finalize();
        }
        
        // Create new backend
        current_backend_ = backend_factory_->CreateBackend(type);
        if (!current_backend_) {
            return ResultInternalError;
        }
        
        current_backend_type_ = type;
        
        // Initialize new backend
        auto error = current_backend_->Initialize();
        if (error != Core::Multiplayer::ErrorCode::Success) {
            return MapErrorToResult(error);
        }
        
        return ResultSuccess;
    }
    
    Core::Multiplayer::HLE::BackendFactory::BackendType GetCurrentBackendType() override {
        return current_backend_type_;
    }

private:
    Result MapErrorToResult(Core::Multiplayer::ErrorCode error) {
        // Minimal error mapping to make tests pass
        switch (error) {
        case Core::Multiplayer::ErrorCode::Success:
            return ResultSuccess;
        case Core::Multiplayer::ErrorCode::ConnectionFailed:
        case Core::Multiplayer::ErrorCode::ConnectionTimeout:
        case Core::Multiplayer::ErrorCode::ConnectionRefused:
            return ResultConnectionFailed;
        case Core::Multiplayer::ErrorCode::AuthenticationFailed:
            return ResultAuthenticationFailed;
        case Core::Multiplayer::ErrorCode::NetworkTimeout:
            return ResultAuthenticationTimeout;
        case Core::Multiplayer::ErrorCode::MessageTooLarge:
            return ResultAdvertiseDataTooLarge;
        case Core::Multiplayer::ErrorCode::InvalidParameter:
        case Core::Multiplayer::ErrorCode::ConfigurationInvalid:
            return ResultBadInput;
        case Core::Multiplayer::ErrorCode::InvalidState:
            return ResultBadState;
        case Core::Multiplayer::ErrorCode::MaxPeersExceeded:
            return ResultMaximumNodeCount;
        case Core::Multiplayer::ErrorCode::PermissionDenied:
            return ResultAccessPointConnectionFailed;
        default:
            return ResultInternalError;
        }
    }
};

} // namespace Service::LDN