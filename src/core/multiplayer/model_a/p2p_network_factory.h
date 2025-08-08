// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "i_p2p_network.h"
#include "p2p_types.h"
#include <memory>

namespace Core::Multiplayer::ModelA {

/**
 * Factory for creating P2P Network implementations
 * Supports both mock (for testing) and real libp2p implementations
 */
class P2PNetworkFactory {
public:
    enum class Implementation {
        Mock,      // Mock implementation for testing
        Libp2p     // Real cpp-libp2p implementation for production
    };
    
    /**
     * Create a P2P network implementation
     * @param implementation The type of implementation to create
     * @param config Configuration for the P2P network
     * @return Unique pointer to the P2P network implementation
     */
    static std::unique_ptr<IP2PNetwork> Create(
        Implementation implementation,
        const P2PNetworkConfig& config
    );
    
    /**
     * Create a mock P2P network implementation for testing
     * @param config Configuration for the P2P network
     * @return Unique pointer to the mock P2P network
     */
    static std::unique_ptr<IP2PNetwork> CreateMock(const P2PNetworkConfig& config);
    
    /**
     * Create a real libp2p P2P network implementation for production
     * @param config Configuration for the P2P network
     * @return Unique pointer to the libp2p P2P network
     */
    static std::unique_ptr<IP2PNetwork> CreateLibp2p(const P2PNetworkConfig& config);
    
    /**
     * Get the default implementation type based on build configuration
     * @return The default implementation to use
     */
    static Implementation GetDefaultImplementation();
};

} // namespace Core::Multiplayer::ModelA
