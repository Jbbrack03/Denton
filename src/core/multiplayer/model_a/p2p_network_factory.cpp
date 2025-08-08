// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "p2p_network_factory.h"
#include "p2p_network.h"

namespace Core::Multiplayer::ModelA {

std::unique_ptr<IP2PNetwork> P2PNetworkFactory::Create(
    Implementation implementation,
    const P2PNetworkConfig& config) {
    
    switch (implementation) {
        case Implementation::Mock:
            return CreateMock(config);
        case Implementation::Libp2p:
            return CreateLibp2p(config);
        default:
            return CreateLibp2p(config); // Default to libp2p
    }
}

std::unique_ptr<IP2PNetwork> P2PNetworkFactory::CreateMock(const P2PNetworkConfig& config) {
    // Mock implementation uses the same facade with testing configuration
    return std::make_unique<P2PNetwork>(config);
}

std::unique_ptr<IP2PNetwork> P2PNetworkFactory::CreateLibp2p(const P2PNetworkConfig& config) {
    // Create facade which wraps real libp2p components
    return std::make_unique<P2PNetwork>(config);
}

P2PNetworkFactory::Implementation P2PNetworkFactory::GetDefaultImplementation() {
#ifdef SUDACHI_ENABLE_TESTING
    // Use mock implementation for testing builds
    return Implementation::Mock;
#else
    // Use real libp2p implementation for production builds
    return Implementation::Libp2p;
#endif
}

} // namespace Core::Multiplayer::ModelA