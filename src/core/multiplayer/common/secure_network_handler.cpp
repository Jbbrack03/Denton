// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "secure_network_handler.h"
#include <spdlog/spdlog.h>

namespace Core::Multiplayer {

SecureNetworkHandler::SecureNetworkHandler() {
    auto rate_config = CreateDefaultRateConfig();
    auto ddos_config = CreateDefaultDDoSConfig();
    security_manager_ = std::make_unique<Security::NetworkSecurityManager>(rate_config, ddos_config);
}

SecureNetworkHandler::SecureNetworkHandler(
    const Security::RateLimitConfig& rate_config,
    const Security::DDoSProtectionConfig& ddos_config) {
    
    security_manager_ = std::make_unique<Security::NetworkSecurityManager>(rate_config, ddos_config);
}

bool SecureNetworkHandler::HandleIncomingPacket(
    const std::string& client_id,
    const std::string& client_ip,
    const std::vector<uint8_t>& data,
    const std::string& protocol) {
    
    // Validate packet through security manager
    auto validation_result = security_manager_->ValidateIncomingPacket(client_id, client_ip, data, protocol);
    
    if (!validation_result.is_valid) {
        LogSecurityViolation(client_id, client_ip, validation_result.error_message);
        return false;
    }
    
    // Forward to packet handler if valid
    if (packet_handler_) {
        packet_handler_(client_id, data);
    }
    
    return true;
}

bool SecureNetworkHandler::HandleIncomingJsonMessage(
    const std::string& client_id,
    const std::string& client_ip,
    const std::string& json_message) {
    
    // Validate JSON message through security manager
    auto validation_result = security_manager_->ValidateIncomingJsonMessage(client_id, client_ip, json_message);
    
    if (!validation_result.is_valid) {
        LogSecurityViolation(client_id, client_ip, validation_result.error_message);
        return false;
    }
    
    // Forward to JSON handler if valid
    if (json_handler_) {
        json_handler_(client_id, json_message);
    }
    
    return true;
}

bool SecureNetworkHandler::HandleNewConnection(
    const std::string& client_id,
    const std::string& client_ip,
    const std::string& connection_id) {
    
    // Validate connection through security manager
    auto validation_result = security_manager_->ValidateNewConnection(client_ip, connection_id);
    
    if (!validation_result.is_valid) {
        LogSecurityViolation(client_id, client_ip, validation_result.error_message);
        return false;
    }
    
    // Forward to connection handler if valid
    if (connection_handler_) {
        connection_handler_(client_id, client_ip);
    }
    
    spdlog::info("Accepted connection from client {} (IP: {})", client_id, client_ip);
    return true;
}

void SecureNetworkHandler::HandleClientDisconnection(
    const std::string& client_id,
    const std::string& client_ip,
    const std::string& connection_id) {
    
    // Cleanup security state
    security_manager_->RemoveClient(client_id, client_ip, connection_id);
    
    spdlog::info("Client {} disconnected (IP: {})", client_id, client_ip);
}

void SecureNetworkHandler::SetPacketHandler(PacketHandler handler) {
    packet_handler_ = handler;
}

void SecureNetworkHandler::SetJsonMessageHandler(JsonMessageHandler handler) {
    json_handler_ = handler;
}

void SecureNetworkHandler::SetConnectionHandler(ConnectionHandler handler) {
    connection_handler_ = handler;
}

std::string SecureNetworkHandler::GetSecurityStats() const {
    return security_manager_->GetSecurityStats();
}

Security::RateLimitConfig SecureNetworkHandler::CreateDefaultRateConfig() {
    Security::RateLimitConfig config;
    config.packets_per_second = 60.0;        // 60 packets per second per client
    config.burst_capacity = 120.0;           // Allow bursts up to 120 packets
    config.bytes_per_second = 1048576.0;     // 1MB per second per client
    config.byte_burst_capacity = 2097152.0;  // 2MB burst capacity
    return config;
}

Security::DDoSProtectionConfig SecureNetworkHandler::CreateDefaultDDoSConfig() {
    Security::DDoSProtectionConfig config;
    config.max_total_connections = 10000;                     // Maximum total connections
    config.max_connections_per_ip = 50;                       // Maximum connections per IP
    config.max_packets_per_second = 1000;                     // Global packet rate limit
    config.connection_timeout = std::chrono::seconds(300);    // 5 minute connection timeout
    config.blacklist_duration = std::chrono::seconds(3600);   // 1 hour blacklist
    return config;
}

void SecureNetworkHandler::LogSecurityViolation(
    const std::string& client_id, 
    const std::string& client_ip, 
    const std::string& reason) {
    
    spdlog::warn("Security violation from client {} (IP: {}): {}", client_id, client_ip, reason);
}

} // namespace Core::Multiplayer
