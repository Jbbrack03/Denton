// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "backend_factory.h"

#include <memory>

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
        // Minimal implementation - returns nullptr to make tests fail appropriately
        // This will be replaced with actual backend implementations
        switch (type) {
        case BackendType::ModelA_Internet:
            // return std::make_unique<ModelABackend>();
            return nullptr;  // Will cause tests to fail as expected
        case BackendType::ModelB_AdHoc:
            // return std::make_unique<ModelBBackend>();
            return nullptr;  // Will cause tests to fail as expected
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
            // Auto-select based on availability
            if (config_manager_->IsModelBAvailable()) {
                return BackendType::ModelB_AdHoc;  // Prefer local play
            } else if (config_manager_->IsModelAAvailable()) {
                return BackendType::ModelA_Internet;
            } else {
                // Fallback to internet mode
                return BackendType::ModelA_Internet;
            }
        default:
            return BackendType::ModelA_Internet;
        }
    }

private:
    std::shared_ptr<ConfigurationManager> config_manager_;
};

} // namespace Core::Multiplayer::HLE