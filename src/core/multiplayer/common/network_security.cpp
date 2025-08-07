// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "network_security.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using json = nlohmann::json;

namespace Core::Multiplayer::Security {

// NetworkInputValidator implementation
ValidationResult NetworkInputValidator::ValidatePacket(const std::vector<uint8_t>& data) {
    // Size validation
    if (data.size() > MAX_PACKET_SIZE) {
        return ValidationResult::Failure(
            "Packet size exceeds maximum allowed: " + std::to_string(data.size()) + " > " + std::to_string(MAX_PACKET_SIZE),
            ErrorCode::InvalidParameter
        );
    }
    
    if (data.size() < MIN_PACKET_SIZE) {
        return ValidationResult::Failure(
            "Packet size below minimum required: " + std::to_string(data.size()) + " < " + std::to_string(MIN_PACKET_SIZE),
            ErrorCode::InvalidParameter
        );
    }
    
    // Basic header validation (assuming first 4 bytes are magic + version)
    if (data.size() >= 4) {
        uint16_t magic = (data[1] << 8) | data[0];  // Little-endian
        uint16_t version = (data[3] << 8) | data[2];
        
        if (magic != 0x4C44) {  // "LD" magic
            return ValidationResult::Failure(
                "Invalid packet magic: 0x" + std::to_string(magic),
                ErrorCode::ProtocolError
            );
        }
        
        if (version != 1) {
            return ValidationResult::Failure(
                "Unsupported packet version: " + std::to_string(version),
                ErrorCode::ProtocolError
            );
        }
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkInputValidator::ValidateJsonMessage(const std::string& json_str) {
    // Size validation
    if (json_str.size() > MAX_JSON_SIZE) {
        return ValidationResult::Failure(
            "JSON message too large: " + std::to_string(json_str.size()) + " > " + std::to_string(MAX_JSON_SIZE),
            ErrorCode::MessageTooLarge
        );
    }
    
    // UTF-8 validation
    if (!IsValidUtf8(json_str)) {
        return ValidationResult::Failure(
            "JSON message contains invalid UTF-8",
            ErrorCode::InvalidMessage
        );
    }
    
    // Check for suspicious patterns
    if (ContainsSuspiciousPatterns(json_str)) {
        return ValidationResult::Failure(
            "JSON message contains suspicious patterns",
            ErrorCode::InvalidMessage
        );
    }
    
    // JSON structure validation
    return ValidateJsonStructure(json_str);
}

ValidationResult NetworkInputValidator::ValidateProtocolName(const std::string& protocol) {
    if (protocol.size() > MAX_PROTOCOL_NAME_SIZE) {
        return ValidationResult::Failure(
            "Protocol name too long: " + std::to_string(protocol.size()),
            ErrorCode::InvalidParameter
        );
    }
    
    if (protocol.empty()) {
        return ValidationResult::Failure(
            "Protocol name cannot be empty",
            ErrorCode::InvalidParameter
        );
    }
    
    // Protocol name should match pattern: /sudachi/protocol/version
    std::regex protocol_pattern(R"(^/[a-zA-Z0-9_-]+(/[a-zA-Z0-9_.-]+)*$)");
    if (!std::regex_match(protocol, protocol_pattern)) {
        return ValidationResult::Failure(
            "Invalid protocol name format: " + protocol,
            ErrorCode::InvalidParameter
        );
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkInputValidator::ValidatePeerId(const std::string& peer_id) {
    if (peer_id.size() > MAX_PEER_ID_SIZE) {
        return ValidationResult::Failure(
            "Peer ID too long: " + std::to_string(peer_id.size()),
            ErrorCode::InvalidParameter
        );
    }
    
    if (peer_id.empty()) {
        return ValidationResult::Failure(
            "Peer ID cannot be empty",
            ErrorCode::InvalidParameter
        );
    }
    
    // Base58 validation (simplified - should only contain valid Base58 characters)
    std::regex base58_pattern(R"(^[1-9A-HJ-NP-Za-km-z]+$)");
    if (!std::regex_match(peer_id, base58_pattern)) {
        return ValidationResult::Failure(
            "Invalid peer ID format (not Base58): " + peer_id,
            ErrorCode::InvalidParameter
        );
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkInputValidator::ValidateMultiaddress(const std::string& multiaddr) {
    if (multiaddr.size() > MAX_MULTIADDR_SIZE) {
        return ValidationResult::Failure(
            "Multiaddress too long: " + std::to_string(multiaddr.size()),
            ErrorCode::InvalidParameter
        );
    }
    
    if (multiaddr.empty()) {
        return ValidationResult::Failure(
            "Multiaddress cannot be empty",
            ErrorCode::InvalidParameter
        );
    }
    
    // Basic multiaddress format validation (should start with /)
    if (multiaddr[0] != '/') {
        return ValidationResult::Failure(
            "Invalid multiaddress format (should start with /): " + multiaddr,
            ErrorCode::InvalidParameter
        );
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkInputValidator::ValidatePacketContent(const std::vector<uint8_t>& data, const std::string& protocol) {
    // Protocol-specific validation
    if (protocol == "/sudachi/ldn/1.0.0") {
        // LDN packet validation
        if (data.size() < 12) {  // Minimum LDN packet size
            return ValidationResult::Failure(
                "LDN packet too small: " + std::to_string(data.size()),
                ErrorCode::ProtocolError
            );
        }
        
        // Additional LDN-specific validation can be added here
    }
    
    return ValidationResult::Success();
}

bool NetworkInputValidator::IsValidUtf8(const std::string& str) {
    // Simple UTF-8 validation (can be enhanced with proper UTF-8 library)
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = str[i];
        
        if (c < 0x80) {
            // ASCII character
            continue;
        } else if (c < 0xC0) {
            // Invalid UTF-8 start byte
            return false;
        } else if (c < 0xE0) {
            // 2-byte sequence
            if (i + 1 >= str.length() || (str[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            i += 1;
        } else if (c < 0xF0) {
            // 3-byte sequence
            if (i + 2 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
        } else if (c < 0xF8) {
            // 4-byte sequence
            if (i + 3 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || 
                (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
        } else {
            // Invalid UTF-8
            return false;
        }
    }
    
    return true;
}

bool NetworkInputValidator::ContainsSuspiciousPatterns(const std::string& str) {
    // Compile all patterns into a single case-insensitive regular expression to
    // avoid allocating a lowercased copy of the input and to efficiently
    // perform multi-pattern matching.
    static const std::regex suspicious_regex(
        R"(<script|javascript:|eval\(|exec\(|\.\./|..\\|cmd\.exe|/bin/|drop table|select \* from|union select|'; --)",
        std::regex::icase);

    return std::regex_search(str, suspicious_regex);
}

namespace {
ValidationResult CheckJsonNode(const json& node, size_t depth) {
    if (depth > MAX_JSON_DEPTH) {
        return ValidationResult::Failure(
            "JSON depth exceeds maximum allowed: " + std::to_string(depth) +
                " > " + std::to_string(MAX_JSON_DEPTH),
            ErrorCode::InvalidMessage);
    }

    if (node.is_array()) {
        if (node.size() > MAX_JSON_ARRAY_SIZE) {
            return ValidationResult::Failure(
                "JSON array too large: " + std::to_string(node.size()) +
                    " > " + std::to_string(MAX_JSON_ARRAY_SIZE),
                ErrorCode::InvalidMessage);
        }
        for (const auto& item : node) {
            auto res = CheckJsonNode(item, depth + 1);
            if (!res.is_valid) {
                return res;
            }
        }
    } else if (node.is_object()) {
        if (node.size() > MAX_JSON_OBJECT_SIZE) {
            return ValidationResult::Failure(
                "JSON object too large: " + std::to_string(node.size()) +
                    " > " + std::to_string(MAX_JSON_OBJECT_SIZE),
                ErrorCode::InvalidMessage);
        }
        for (const auto& item : node.items()) {
            auto res = CheckJsonNode(item.value(), depth + 1);
            if (!res.is_valid) {
                return res;
            }
        }
    }

    return ValidationResult::Success();
}
} // namespace

ValidationResult NetworkInputValidator::ValidateJsonStructure(const std::string& json_str) {
    try {
        auto parsed = json::parse(json_str);

        auto res = CheckJsonNode(parsed, 1);
        if (!res.is_valid) {
            return res;
        }

        return ValidationResult::Success();

    } catch (const json::parse_error& e) {
        return ValidationResult::Failure(
            "Invalid JSON format: " + std::string(e.what()),
            ErrorCode::InvalidMessage
        );
    } catch (const std::exception& e) {
        return ValidationResult::Failure(
            "JSON validation error: " + std::string(e.what()),
            ErrorCode::InvalidMessage
        );
    }
}

// TokenBucketRateLimit implementation
TokenBucketRateLimit::TokenBucketRateLimit(double capacity, double refill_rate)
    : capacity_(capacity), refill_rate_(refill_rate), tokens_(capacity) {
    last_refill_ = std::chrono::steady_clock::now();
}

bool TokenBucketRateLimit::TryConsume(double tokens) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Refill();
    
    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    
    return false;
}

double TokenBucketRateLimit::GetTokens() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tokens_;
}

void TokenBucketRateLimit::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_ = capacity_;
    last_refill_ = std::chrono::steady_clock::now();
}

void TokenBucketRateLimit::Refill() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_refill_).count();
    
    double tokens_to_add = elapsed * refill_rate_;
    tokens_ = std::min(capacity_, tokens_ + tokens_to_add);
    
    last_refill_ = now;
}

// ClientRateManager implementation
ClientRateManager::ClientRateManager(const RateLimitConfig& config) : config_(config) {}

bool ClientRateManager::CheckPacketRateLimit(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto* limits = GetOrCreateClientLimits(client_id);
    limits->total_packets++;
    limits->last_activity = std::chrono::steady_clock::now();
    
    return limits->packet_limiter->TryConsume(1.0);
}

bool ClientRateManager::CheckByteRateLimit(const std::string& client_id, size_t bytes) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto* limits = GetOrCreateClientLimits(client_id);
    limits->total_bytes += bytes;
    limits->last_activity = std::chrono::steady_clock::now();
    
    return limits->byte_limiter->TryConsume(static_cast<double>(bytes));
}

void ClientRateManager::RemoveClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    client_limits_.erase(client_id);
}

std::string ClientRateManager::GetClientStats(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = client_limits_.find(client_id);
    if (it == client_limits_.end()) {
        return "Client not found";
    }
    
    const auto& limits = it->second;
    std::ostringstream oss;
    oss << "Client " << client_id << ": "
        << "Packets=" << limits->total_packets << ", "
        << "Bytes=" << limits->total_bytes << ", "
        << "PacketTokens=" << limits->packet_limiter->GetTokens() << ", "
        << "ByteTokens=" << limits->byte_limiter->GetTokens();
    
    return oss.str();
}

ClientRateManager::ClientLimits* ClientRateManager::GetOrCreateClientLimits(const std::string& client_id) {
    auto it = client_limits_.find(client_id);
    if (it == client_limits_.end()) {
        auto limits = std::make_unique<ClientLimits>();
        limits->packet_limiter = std::make_unique<TokenBucketRateLimit>(
            config_.burst_capacity, config_.packets_per_second);
        limits->byte_limiter = std::make_unique<TokenBucketRateLimit>(
            config_.byte_burst_capacity, config_.bytes_per_second);
        limits->last_activity = std::chrono::steady_clock::now();
        limits->total_packets = 0;
        limits->total_bytes = 0;
        
        auto* ptr = limits.get();
        client_limits_[client_id] = std::move(limits);
        return ptr;
    }
    
    return it->second.get();
}

void ClientRateManager::CleanupInactiveClients() {
    auto now = std::chrono::steady_clock::now();
    auto timeout = std::chrono::minutes(30); // Remove clients inactive for 30 minutes
    
    for (auto it = client_limits_.begin(); it != client_limits_.end();) {
        if (now - it->second->last_activity > timeout) {
            it = client_limits_.erase(it);
        } else {
            ++it;
        }
    }
}

// DDoSProtection implementation
DDoSProtection::DDoSProtection(const DDoSProtectionConfig& config) : config_(config) {
    global_packet_limiter_ = std::make_unique<TokenBucketRateLimit>(
        static_cast<double>(config_.max_packets_per_second * 2), // 2x burst capacity
        static_cast<double>(config_.max_packets_per_second)
    );
}

bool DDoSProtection::AllowNewConnection(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(protection_mutex_);
    
    // Validate IP address format
    if (!IsValidIPAddress(ip_address)) {
        return false;
    }
    
    // Check if IP is blacklisted
    if (IsBlacklisted(ip_address)) {
        return false;
    }
    
    // Check global connection limit
    if (active_connections_.load() >= config_.max_total_connections) {
        return false;
    }
    
    // Check per-IP connection limit
    auto& ip_info = ip_connections_[ip_address];
    if (ip_info.connection_count >= config_.max_connections_per_ip) {
        // Blacklist IP for too many connections
        BlacklistIP(ip_address, "Too many connections from IP");
        return false;
    }
    
    return true;
}

void DDoSProtection::RegisterConnection(const std::string& ip_address, const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(protection_mutex_);
    
    active_connections_.fetch_add(1);
    
    auto& ip_info = ip_connections_[ip_address];
    if (ip_info.connection_count == 0) {
        ip_info.first_connection = std::chrono::steady_clock::now();
    }
    
    ip_info.connection_count++;
    ip_info.connection_ids.push_back(connection_id);
    ip_info.last_activity = std::chrono::steady_clock::now();
}

void DDoSProtection::RemoveConnection(const std::string& ip_address, const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(protection_mutex_);
    
    active_connections_.fetch_sub(1);
    
    auto it = ip_connections_.find(ip_address);
    if (it != ip_connections_.end()) {
        auto& ip_info = it->second;
        
        // Remove connection ID
        auto conn_it = std::find(ip_info.connection_ids.begin(), ip_info.connection_ids.end(), connection_id);
        if (conn_it != ip_info.connection_ids.end()) {
            ip_info.connection_ids.erase(conn_it);
        }
        
        if (ip_info.connection_count > 0) {
            ip_info.connection_count--;
        }
        
        // Remove IP info if no connections remain
        if (ip_info.connection_count == 0) {
            ip_connections_.erase(it);
        } else {
            ip_info.last_activity = std::chrono::steady_clock::now();
        }
    }
}

bool DDoSProtection::CheckGlobalPacketRate() {
    return global_packet_limiter_->TryConsume(1.0);
}

void DDoSProtection::BlacklistIP(const std::string& ip_address, const std::string& reason) {
    BlacklistEntry entry;
    entry.blacklist_time = std::chrono::steady_clock::now();
    entry.reason = reason;
    
    blacklisted_ips_[ip_address] = entry;
}

bool DDoSProtection::IsBlacklisted(const std::string& ip_address) const {
    auto it = blacklisted_ips_.find(ip_address);
    if (it == blacklisted_ips_.end()) {
        return false;
    }
    
    // Check if blacklist has expired
    auto now = std::chrono::steady_clock::now();
    if (now - it->second.blacklist_time > config_.blacklist_duration) {
        return false; // Expired
    }
    
    return true;
}

std::string DDoSProtection::GetProtectionStats() const {
    std::lock_guard<std::mutex> lock(protection_mutex_);
    
    std::ostringstream oss;
    oss << "DDoS Protection Stats:\n"
        << "  Active connections: " << active_connections_.load() << "/" << config_.max_total_connections << "\n"
        << "  Unique IPs: " << ip_connections_.size() << "\n"
        << "  Blacklisted IPs: " << blacklisted_ips_.size() << "\n"
        << "  Global packet tokens: " << global_packet_limiter_->GetTokens();
    
    return oss.str();
}

void DDoSProtection::CleanupExpiredBlacklist() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = blacklisted_ips_.begin(); it != blacklisted_ips_.end();) {
        if (now - it->second.blacklist_time > config_.blacklist_duration) {
            it = blacklisted_ips_.erase(it);
        } else {
            ++it;
        }
    }
}

void DDoSProtection::CleanupInactiveConnections() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = ip_connections_.begin(); it != ip_connections_.end();) {
        if (now - it->second.last_activity > config_.connection_timeout) {
            // Adjust active connection count
            active_connections_.fetch_sub(it->second.connection_count);
            it = ip_connections_.erase(it);
        } else {
            ++it;
        }
    }
}

bool DDoSProtection::IsValidIPAddress(const std::string& ip_address) const {
    // Simple IPv4 validation (can be enhanced for IPv6)
    std::regex ipv4_pattern(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip_address, ipv4_pattern);
}

// NetworkSecurityManager implementation
NetworkSecurityManager::NetworkSecurityManager(
    const RateLimitConfig& rate_config,
    const DDoSProtectionConfig& ddos_config) {
    
    rate_manager_ = std::make_unique<ClientRateManager>(rate_config);
    ddos_protection_ = std::make_unique<DDoSProtection>(ddos_config);
}

ValidationResult NetworkSecurityManager::ValidateIncomingPacket(
    const std::string& client_id,
    const std::string& client_ip,
    const std::vector<uint8_t>& data,
    const std::string& protocol) {
    
    // Global rate limiting check
    if (!ddos_protection_->CheckGlobalPacketRate()) {
        return ValidationResult::Failure(
            "Global packet rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    // Input validation
    auto validation_result = NetworkInputValidator::ValidatePacket(data);
    if (!validation_result.is_valid) {
        return validation_result;
    }
    
    auto protocol_result = NetworkInputValidator::ValidateProtocolName(protocol);
    if (!protocol_result.is_valid) {
        return protocol_result;
    }
    
    auto content_result = NetworkInputValidator::ValidatePacketContent(data, protocol);
    if (!content_result.is_valid) {
        return content_result;
    }
    
    // Rate limiting check
    if (!rate_manager_->CheckPacketRateLimit(client_id)) {
        return ValidationResult::Failure(
            "Client packet rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    if (!rate_manager_->CheckByteRateLimit(client_id, data.size())) {
        return ValidationResult::Failure(
            "Client byte rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkSecurityManager::ValidateIncomingJsonMessage(
    const std::string& client_id,
    const std::string& client_ip,
    const std::string& json_message) {
    
    // Global rate limiting check
    if (!ddos_protection_->CheckGlobalPacketRate()) {
        return ValidationResult::Failure(
            "Global packet rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    // Input validation
    auto validation_result = NetworkInputValidator::ValidateJsonMessage(json_message);
    if (!validation_result.is_valid) {
        return validation_result;
    }
    
    // Rate limiting check
    if (!rate_manager_->CheckPacketRateLimit(client_id)) {
        return ValidationResult::Failure(
            "Client packet rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    if (!rate_manager_->CheckByteRateLimit(client_id, json_message.size())) {
        return ValidationResult::Failure(
            "Client byte rate limit exceeded",
            ErrorCode::NetworkTimeout
        );
    }
    
    return ValidationResult::Success();
}

ValidationResult NetworkSecurityManager::ValidateNewConnection(
    const std::string& client_ip,
    const std::string& connection_id) {
    
    if (!ddos_protection_->AllowNewConnection(client_ip)) {
        return ValidationResult::Failure(
            "Connection rejected by DDoS protection",
            ErrorCode::ConnectionRefused
        );
    }
    
    ddos_protection_->RegisterConnection(client_ip, connection_id);
    
    return ValidationResult::Success();
}

void NetworkSecurityManager::RemoveClient(
    const std::string& client_id,
    const std::string& client_ip,
    const std::string& connection_id) {
    
    rate_manager_->RemoveClient(client_id);
    ddos_protection_->RemoveConnection(client_ip, connection_id);
}

std::string NetworkSecurityManager::GetSecurityStats() const {
    std::ostringstream oss;
    oss << "=== Network Security Statistics ===\n";
    oss << ddos_protection_->GetProtectionStats() << "\n";
    oss << "Rate limiting active for various clients\n";
    oss << "=====================================";
    
    return oss.str();
}

} // namespace Core::Multiplayer::Security