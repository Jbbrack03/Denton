#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>

#include "src/core/multiplayer/common/error_codes.h"
#include "sudachi/src/core/hle/service/ldn/ldn_types.h"

namespace Core::Multiplayer::HLE {

class MultiplayerBackend {
public:
    virtual ~MultiplayerBackend() = default;

    virtual ErrorCode Initialize() = 0;
    virtual ErrorCode Finalize() = 0;
    virtual bool IsInitialized() const = 0;

    virtual ErrorCode CreateNetwork(const Service::LDN::CreateNetworkConfig& config) = 0;
    virtual ErrorCode DestroyNetwork() = 0;
    virtual ErrorCode Connect(const Service::LDN::ConnectNetworkData& connect_data,
                             const Service::LDN::NetworkInfo& network_info) = 0;
    virtual ErrorCode Disconnect() = 0;

    virtual ErrorCode Scan(std::vector<Service::LDN::NetworkInfo>& out_networks,
                          const Service::LDN::ScanFilter& filter) = 0;
    virtual ErrorCode GetNetworkInfo(Service::LDN::NetworkInfo& out_info) = 0;
    virtual ErrorCode GetCurrentState(Service::LDN::State& out_state) = 0;

    virtual ErrorCode OpenAccessPoint() = 0;
    virtual ErrorCode CloseAccessPoint() = 0;
    virtual ErrorCode OpenStation() = 0;
    virtual ErrorCode CloseStation() = 0;

    virtual ErrorCode SendPacket(const std::vector<uint8_t>& data, uint8_t node_id) = 0;
    virtual ErrorCode ReceivePacket(std::vector<uint8_t>& out_data, uint8_t& out_node_id) = 0;

    virtual ErrorCode SetAdvertiseData(const std::vector<uint8_t>& data) = 0;
    virtual ErrorCode SetStationAcceptPolicy(Service::LDN::AcceptPolicy policy) = 0;
    virtual ErrorCode AddAcceptFilterEntry(const Service::LDN::MacAddress& mac) = 0;
    virtual ErrorCode GetSecurityParameter(Service::LDN::SecurityParameter& out_param) = 0;
    virtual ErrorCode GetDisconnectReason(Service::LDN::DisconnectReason& out_reason) = 0;
    virtual ErrorCode GetIpv4Address(Service::LDN::Ipv4Address& out_address,
                                     Service::LDN::Ipv4Address& out_subnet) = 0;
    virtual ErrorCode GetNetworkConfig(Service::LDN::NetworkConfig& out_config) = 0;

    virtual void RegisterNodeEventCallbacks(std::function<void(uint8_t)> on_node_joined,
                                            std::function<void(uint8_t)> on_node_left) = 0;
};

class MockMultiplayerBackend : public MultiplayerBackend {
public:
    MOCK_METHOD(ErrorCode, Initialize, (), (override));
    MOCK_METHOD(ErrorCode, Finalize, (), (override));
    MOCK_METHOD(bool, IsInitialized, (), (const, override));
    MOCK_METHOD(ErrorCode, CreateNetwork, (const Service::LDN::CreateNetworkConfig&), (override));
    MOCK_METHOD(ErrorCode, DestroyNetwork, (), (override));
    MOCK_METHOD(ErrorCode, Connect,
                (const Service::LDN::ConnectNetworkData&, const Service::LDN::NetworkInfo&), (override));
    MOCK_METHOD(ErrorCode, Disconnect, (), (override));
    MOCK_METHOD(ErrorCode, Scan,
                (std::vector<Service::LDN::NetworkInfo>&, const Service::LDN::ScanFilter&), (override));
    MOCK_METHOD(ErrorCode, GetNetworkInfo, (Service::LDN::NetworkInfo&), (override));
    MOCK_METHOD(ErrorCode, GetCurrentState, (Service::LDN::State&), (override));
    MOCK_METHOD(ErrorCode, OpenAccessPoint, (), (override));
    MOCK_METHOD(ErrorCode, CloseAccessPoint, (), (override));
    MOCK_METHOD(ErrorCode, OpenStation, (), (override));
    MOCK_METHOD(ErrorCode, CloseStation, (), (override));
    MOCK_METHOD(ErrorCode, SendPacket, (const std::vector<uint8_t>&, uint8_t), (override));
    MOCK_METHOD(ErrorCode, ReceivePacket, (std::vector<uint8_t>&, uint8_t&), (override));
    MOCK_METHOD(ErrorCode, SetAdvertiseData, (const std::vector<uint8_t>&), (override));
    MOCK_METHOD(ErrorCode, SetStationAcceptPolicy, (Service::LDN::AcceptPolicy), (override));
    MOCK_METHOD(ErrorCode, AddAcceptFilterEntry, (const Service::LDN::MacAddress&), (override));
    MOCK_METHOD(ErrorCode, GetSecurityParameter, (Service::LDN::SecurityParameter&), (override));
    MOCK_METHOD(ErrorCode, GetDisconnectReason, (Service::LDN::DisconnectReason&), (override));
    MOCK_METHOD(ErrorCode, GetIpv4Address,
                (Service::LDN::Ipv4Address&, Service::LDN::Ipv4Address&), (override));
    MOCK_METHOD(ErrorCode, GetNetworkConfig, (Service::LDN::NetworkConfig&), (override));
    MOCK_METHOD(void, RegisterNodeEventCallbacks,
                (std::function<void(uint8_t)>, std::function<void(uint8_t)>), (override));
};

class DummyMultiplayerBackend : public MultiplayerBackend {
public:
    ErrorCode Initialize() override {
        initialized_ = true;
        return ErrorCode::Success;
    }

    ErrorCode Finalize() override {
        initialized_ = false;
        ap_opened_ = false;
        network_created_ = false;
        station_opened_ = false;
        connected_ = false;
        advertise_data_.clear();
        return ErrorCode::Success;
    }

    bool IsInitialized() const override { return initialized_; }

    ErrorCode CreateNetwork(const Service::LDN::CreateNetworkConfig& config) override {
        if (!ap_opened_)
            return ErrorCode::InvalidState;
        network_created_ = true;
        network_info_.network_id.intent_id.local_communication_id =
            config.network_config.intent_id.local_communication_id;
        return ErrorCode::Success;
    }

    ErrorCode DestroyNetwork() override {
        network_created_ = false;
        return ErrorCode::Success;
    }

    ErrorCode Connect(const Service::LDN::ConnectNetworkData&,
                      const Service::LDN::NetworkInfo& info) override {
        if (!station_opened_)
            return ErrorCode::InvalidState;
        connected_ = true;
        network_info_ = info;
        return ErrorCode::Success;
    }

    ErrorCode Disconnect() override {
        connected_ = false;
        return ErrorCode::Success;
    }

    ErrorCode Scan(std::vector<Service::LDN::NetworkInfo>& out_networks,
                   const Service::LDN::ScanFilter&) override {
        Service::LDN::NetworkInfo info{};
        info.network_id.intent_id.local_communication_id = 0x0100000000001234ULL;
        out_networks.push_back(info);
        return ErrorCode::Success;
    }

    ErrorCode GetNetworkInfo(Service::LDN::NetworkInfo& out_info) override {
        out_info = network_info_;
        return ErrorCode::Success;
    }

    ErrorCode GetCurrentState(Service::LDN::State& out_state) override {
        if (!initialized_) {
            out_state = Service::LDN::State::None;
        } else if (ap_opened_) {
            out_state = network_created_ ? Service::LDN::State::AccessPointCreated
                                         : Service::LDN::State::AccessPointOpened;
        } else if (station_opened_) {
            out_state = connected_ ? Service::LDN::State::StationConnected
                                   : Service::LDN::State::StationOpened;
        } else {
            out_state = Service::LDN::State::Initialized;
        }
        return ErrorCode::Success;
    }

    ErrorCode OpenAccessPoint() override {
        if (!initialized_)
            return ErrorCode::InvalidState;
        ap_opened_ = true;
        return ErrorCode::Success;
    }

    ErrorCode CloseAccessPoint() override {
        ap_opened_ = false;
        network_created_ = false;
        return ErrorCode::Success;
    }

    ErrorCode OpenStation() override {
        if (!initialized_)
            return ErrorCode::InvalidState;
        station_opened_ = true;
        return ErrorCode::Success;
    }

    ErrorCode CloseStation() override {
        station_opened_ = false;
        connected_ = false;
        return ErrorCode::Success;
    }

    ErrorCode SendPacket(const std::vector<uint8_t>&, uint8_t) override {
        return ErrorCode::Success;
    }

    ErrorCode ReceivePacket(std::vector<uint8_t>& out_data, uint8_t& out_node_id) override {
        out_data.clear();
        out_node_id = 0;
        return ErrorCode::Success;
    }

    ErrorCode SetAdvertiseData(const std::vector<uint8_t>& data) override {
        if (data.size() > Service::LDN::AdvertiseDataSizeMax)
            return ErrorCode::MessageTooLarge;
        advertise_data_ = data;
        return ErrorCode::Success;
    }

    ErrorCode SetStationAcceptPolicy(Service::LDN::AcceptPolicy) override {
        return ErrorCode::Success;
    }

    ErrorCode AddAcceptFilterEntry(const Service::LDN::MacAddress&) override {
        return ErrorCode::Success;
    }

    ErrorCode GetSecurityParameter(Service::LDN::SecurityParameter& out_param) override {
        out_param = {};
        return ErrorCode::Success;
    }

    ErrorCode GetDisconnectReason(Service::LDN::DisconnectReason& out_reason) override {
        out_reason = {};
        return ErrorCode::Success;
    }

    ErrorCode GetIpv4Address(Service::LDN::Ipv4Address& out_address,
                             Service::LDN::Ipv4Address& out_subnet) override {
        out_address.fill(0);
        out_subnet.fill(0);
        return ErrorCode::Success;
    }

    ErrorCode GetNetworkConfig(Service::LDN::NetworkConfig& out_config) override {
        out_config = {};
        return ErrorCode::Success;
    }

    void RegisterNodeEventCallbacks(std::function<void(uint8_t)> join,
                                    std::function<void(uint8_t)> left) override {
        on_node_joined_ = std::move(join);
        on_node_left_ = std::move(left);
    }

private:
    bool initialized_ = false;
    bool ap_opened_ = false;
    bool network_created_ = false;
    bool station_opened_ = false;
    bool connected_ = false;
    Service::LDN::NetworkInfo network_info_{};
    std::vector<uint8_t> advertise_data_{};
    std::function<void(uint8_t)> on_node_joined_{};
    std::function<void(uint8_t)> on_node_left_{};
};

class BackendFactory {
public:
    enum class BackendType { ModelA_Internet, ModelB_AdHoc };
    virtual ~BackendFactory() = default;
    virtual std::unique_ptr<MultiplayerBackend> CreateBackend(BackendType type) = 0;
    virtual BackendType GetPreferredBackend() = 0;
};

class ConfigurationManager {
public:
    enum class MultiplayerMode { Internet, AdHoc, Auto };
    virtual ~ConfigurationManager() = default;
    virtual MultiplayerMode GetPreferredMode() = 0;
    virtual void SetPreferredMode(MultiplayerMode mode) = 0;
    virtual bool IsModelAAvailable() = 0;
    virtual bool IsModelBAvailable() = 0;
    virtual std::string GetConfigFilePath() = 0;
};

class MockConfigurationManager : public ConfigurationManager {
public:
    MOCK_METHOD(MultiplayerMode, GetPreferredMode, (), (override));
    MOCK_METHOD(void, SetPreferredMode, (MultiplayerMode), (override));
    MOCK_METHOD(bool, IsModelAAvailable, (), (override));
    MOCK_METHOD(bool, IsModelBAvailable, (), (override));
    MOCK_METHOD(std::string, GetConfigFilePath, (), (override));
};

class ConcreteBackendFactory : public BackendFactory {
public:
    explicit ConcreteBackendFactory(std::shared_ptr<ConfigurationManager> config)
        : config_manager_(std::move(config)) {}

    std::unique_ptr<MultiplayerBackend> CreateBackend(BackendType) override {
        return std::make_unique<DummyMultiplayerBackend>();
    }

    BackendType GetPreferredBackend() override {
        auto mode = config_manager_->GetPreferredMode();
        switch (mode) {
        case ConfigurationManager::MultiplayerMode::Internet:
            return BackendType::ModelA_Internet;
        case ConfigurationManager::MultiplayerMode::AdHoc:
            return BackendType::ModelB_AdHoc;
        case ConfigurationManager::MultiplayerMode::Auto:
            if (config_manager_->IsModelBAvailable())
                return BackendType::ModelB_AdHoc;
            if (config_manager_->IsModelAAvailable())
                return BackendType::ModelA_Internet;
            return BackendType::ModelA_Internet;
        }
        return BackendType::ModelA_Internet;
    }

private:
    std::shared_ptr<ConfigurationManager> config_manager_;
};

} // namespace Core::Multiplayer::HLE

