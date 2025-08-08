// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * Simple test to validate cpp-libp2p integration compiles correctly
 * This file can be compiled to verify all cpp-libp2p dependencies are working
 */

#include "libp2p_p2p_network.h"
#include "p2p_network_factory.h"
#include <iostream>

using namespace Core::Multiplayer::ModelA;

int main() {
    try {
        // Test factory creation
        P2PNetworkConfig config;
        config.enable_tcp = true;
        config.enable_websocket = true;
        config.enable_nat_traversal = true;
        config.enable_relay = true;
        config.relay_servers = {"relay1.example.com", "relay2.example.com"};
        
        // Test mock creation
        auto mock_network = P2PNetworkFactory::CreateMock(config);
        if (mock_network) {
            std::cout << "Mock P2P network created successfully" << std::endl;
        }
        
        // Test libp2p creation  
        auto libp2p_network = P2PNetworkFactory::CreateLibp2p(config);
        if (libp2p_network) {
            std::cout << "Libp2p P2P network created successfully" << std::endl;
            std::cout << "Peer ID: " << libp2p_network->GetPeerId() << std::endl;
        }
        
        std::cout << "cpp-libp2p integration test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "cpp-libp2p integration test failed: " << e.what() << std::endl;
        return 1;
    }
}
