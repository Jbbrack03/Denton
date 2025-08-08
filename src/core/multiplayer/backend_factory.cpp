// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backend_factory.h"

#include <memory>

#include "model_a/model_a_backend.h"
#include "model_a/p2p_network_factory.h"
#include "model_b/model_b_backend.h"
#include "model_b/mdns_discovery.h"

namespace Core::Multiplayer::HLE {

/**
 * Concrete Backend Factory Implementation
 * Minimal implementation to make tests pass
 */
class ConcreteBackendFactory : public BackendFactory {
public:
    ConcreteBackendFactory(std::shared_ptr<ConfigurationManager> config)
        : config_manager_(config) {}
    
    std::unique_ptr<MultiplayerBackend> CreateBackend(BackendType type) override {
        switch (type) {
        case BackendType::ModelA_Internet: {
            auto p2p_network = ModelA::P2PNetworkFactory::Create(
                ModelA::P2PNetworkFactory::GetDefaultImplementation(),
                ModelA::P2PNetworkConfig{}
            );
            return std::make_unique<ModelA::ModelABackend>(config_manager_, std::move(p2p_network));
        }
        case BackendType::ModelB_AdHoc: {
            auto discovery = std::make_shared<ModelB::MdnsDiscovery>(nullptr, nullptr, nullptr);
            return std::make_unique<ModelB::ModelBBackend>(config_manager_, discovery);
        }
        default:
            return nullptr;
        }
    }
    
    BackendType GetPreferredBackend() override {
        auto mode = config_manager_->GetPreferredMode();
        
        switch (mode) {
        case ConfigurationManager::MultiplayerMode::Internet:
            return BackendType::ModelA_Internet;
        case ConfigurationManager::MultiplayerMode::AdHoc:
            return BackendType::ModelB_AdHoc;
        case ConfigurationManager::MultiplayerMode::Auto:
            if (config_manager_->IsModelBAvailable() && ModelB::ModelBBackend::IsSupported()) {
                return BackendType::ModelB_AdHoc;
            }
            if (config_manager_->IsModelAAvailable() && ModelA::ModelABackend::IsSupported()) {
                return BackendType::ModelA_Internet;
            }
            if (ModelA::ModelABackend::IsSupported()) {
                return BackendType::ModelA_Internet;
            }
            return BackendType::ModelB_AdHoc;
        default:
            return BackendType::ModelA_Internet;
        }
    }

private:
    std::shared_ptr<ConfigurationManager> config_manager_;
};

} // namespace Core::Multiplayer::HLE
