// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "network_security.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core::Multiplayer {

/**
 * Secure network handler that integrates security validation with network operations
 * This class demonstrates how to use the network security framework in multiplayer components
 */
class SecureNetworkHandler {
public:
    using PacketHandler = std::function<void(const std::string& client_id, const std::vector<uint8_t>& data)>;
    using JsonMessageHandler = std::function<void(const std::string& client_id, const std::string& message)>;
    using ConnectionHandler = std::function<void(const std::string& client_id, const std::string& client_ip)>;
    
    /**
     * Create a secure network handler with default security configuration
     */
    SecureNetworkHandler();
    
    /**
     * Create a secure network handler with custom security configuration
     * @param rate_config Rate limiting configuration
     * @param ddos_config DDoS protection configuration
     */
    SecureNetworkHandler(
        const Security::RateLimitConfig& rate_config,
        const Security::DDoSProtectionConfig& ddos_config
    );
    
    /**
     * Handle an incoming packet with security validation
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param data The packet data
     * @param protocol The protocol name
     * @return True if packet was processed, false if rejected
     */
    bool HandleIncomingPacket(
        const std::string& client_id,
        const std::string& client_ip,
        const std::vector<uint8_t>& data,
        const std::string& protocol
    );
    
    /**
     * Handle an incoming JSON message with security validation
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param json_message The JSON message
     * @return True if message was processed, false if rejected
     */
    bool HandleIncomingJsonMessage(
        const std::string& client_id,
        const std::string& client_ip,
        const std::string& json_message
    );
    
    /**
     * Handle a new connection with security validation
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param connection_id The connection identifier
     * @return True if connection was accepted, false if rejected
     */
    bool HandleNewConnection(
        const std::string& client_id,
        const std::string& client_ip,
        const std::string& connection_id
    );
    
    /**
     * Handle client disconnection (cleanup)
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param connection_id The connection identifier
     */
    void HandleClientDisconnection(
        const std::string& client_id,
        const std::string& client_ip,
        const std::string& connection_id
    );
    
    /**
     * Set packet handler callback
     * @param handler Function to call for valid packets
     */
    void SetPacketHandler(PacketHandler handler);
    
    /**
     * Set JSON message handler callback
     * @param handler Function to call for valid JSON messages
     */
    void SetJsonMessageHandler(JsonMessageHandler handler);
    
    /**
     * Set connection handler callback
     * @param handler Function to call for accepted connections
     */
    void SetConnectionHandler(ConnectionHandler handler);
    
    /**
     * Get security statistics
     * @return Security statistics string
     */
    std::string GetSecurityStats() const;
    
    /**
     * Create default rate limiting configuration
     * @return Default rate limiting configuration
     */
    static Security::RateLimitConfig CreateDefaultRateConfig();
    
    /**
     * Create default DDoS protection configuration
     * @return Default DDoS protection configuration
     */
    static Security::DDoSProtectionConfig CreateDefaultDDoSConfig();

private:
    std::unique_ptr<Security::NetworkSecurityManager> security_manager_;
    PacketHandler packet_handler_;
    JsonMessageHandler json_handler_;
    ConnectionHandler connection_handler_;
    
    void LogSecurityViolation(const std::string& client_id, const std::string& client_ip, const std::string& reason);
};

} // namespace Core::Multiplayer
