// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "error_codes.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core::Multiplayer::Security {

// Forward declarations
class NetworkInputValidator;
class TokenBucketRateLimit;
class ClientRateManager;
class DDoSProtection;

/**
 * Maximum sizes for network inputs to prevent buffer overflow attacks
 */
constexpr size_t MAX_PACKET_SIZE = 65536;  // 64KB max packet size
constexpr size_t MIN_PACKET_SIZE = 12;     // Minimum for packet header
constexpr size_t MAX_JSON_SIZE = 32768;    // 32KB max JSON message
constexpr size_t MAX_PROTOCOL_NAME_SIZE = 256;
constexpr size_t MAX_PEER_ID_SIZE = 128;
constexpr size_t MAX_MULTIADDR_SIZE = 512;
// Maximum allowed JSON depth to prevent excessive recursion
constexpr size_t MAX_JSON_DEPTH = 10;
// Limits on array/object entries to mitigate JSON bomb attacks
constexpr size_t MAX_JSON_ARRAY_SIZE = 256;
constexpr size_t MAX_JSON_OBJECT_SIZE = 256;

/**
 * Rate limiting configuration
 */
struct RateLimitConfig {
    double packets_per_second = 60.0;     // 60 packets per second per client
    double burst_capacity = 120.0;        // Allow bursts up to 120 packets
    double bytes_per_second = 1048576.0;  // 1MB per second per client
    double byte_burst_capacity = 2097152.0; // 2MB burst capacity
};

/**
 * DDoS protection configuration
 */
struct DDoSProtectionConfig {
    size_t max_total_connections = 10000;    // Maximum total connections
    size_t max_connections_per_ip = 50;      // Maximum connections per IP
    size_t max_packets_per_second = 1000;    // Global packet rate limit
    std::chrono::seconds connection_timeout{300}; // 5 minute connection timeout
    std::chrono::seconds blacklist_duration{3600}; // 1 hour blacklist
};

/**
 * Network input validation result
 */
struct ValidationResult {
    bool is_valid;
    std::string error_message;
    ErrorCode error_code;
    
    ValidationResult(bool valid, const std::string& message = "", ErrorCode code = ErrorCode::Success)
        : is_valid(valid), error_message(message), error_code(code) {}
        
    static ValidationResult Success() {
        return ValidationResult(true);
    }
    
    static ValidationResult Failure(const std::string& message, ErrorCode code) {
        return ValidationResult(false, message, code);
    }
};

/**
 * Validates network inputs to prevent injection and buffer overflow attacks
 */
class NetworkInputValidator {
public:
    /**
     * Validate a binary packet
     * @param data The packet data to validate
     * @return Validation result
     */
    static ValidationResult ValidatePacket(const std::vector<uint8_t>& data);
    
    /**
     * Validate a JSON message
     * @param json_str The JSON string to validate
     * @return Validation result
     */
    static ValidationResult ValidateJsonMessage(const std::string& json_str);
    
    /**
     * Validate a protocol name
     * @param protocol The protocol name to validate
     * @return Validation result
     */
    static ValidationResult ValidateProtocolName(const std::string& protocol);
    
    /**
     * Validate a peer ID
     * @param peer_id The peer ID to validate
     * @return Validation result
     */
    static ValidationResult ValidatePeerId(const std::string& peer_id);
    
    /**
     * Validate a multiaddress
     * @param multiaddr The multiaddress to validate
     * @return Validation result
     */
    static ValidationResult ValidateMultiaddress(const std::string& multiaddr);
    
    /**
     * Validate packet content based on protocol
     * @param data The packet data
     * @param protocol The protocol name
     * @return Validation result
     */
    static ValidationResult ValidatePacketContent(const std::vector<uint8_t>& data, const std::string& protocol);

private:
    static bool IsValidUtf8(const std::string& str);
    static bool ContainsSuspiciousPatterns(const std::string& str);
    static ValidationResult ValidateJsonStructure(const std::string& json_str);
};

/**
 * Token bucket rate limiting implementation
 * Thread-safe rate limiter using token bucket algorithm
 */
class TokenBucketRateLimit {
public:
    /**
     * Create a token bucket rate limiter
     * @param capacity Maximum number of tokens (burst capacity)
     * @param refill_rate Tokens added per second
     */
    TokenBucketRateLimit(double capacity, double refill_rate);
    
    /**
     * Try to consume tokens
     * @param tokens Number of tokens to consume (default: 1)
     * @return True if tokens were available and consumed, false otherwise
     */
    bool TryConsume(double tokens = 1.0);
    
    /**
     * Get current token count
     * @return Current number of available tokens
     */
    double GetTokens() const;
    
    /**
     * Reset the bucket to full capacity
     */
    void Reset();

private:
    void Refill();
    
    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point last_refill_;
    double tokens_;
    const double capacity_;
    const double refill_rate_;
};

/**
 * Manages rate limiting for multiple clients
 */
class ClientRateManager {
public:
    explicit ClientRateManager(const RateLimitConfig& config);
    
    /**
     * Check if a client is within rate limits for packet count
     * @param client_id The client identifier
     * @return True if within limits, false if rate limited
     */
    bool CheckPacketRateLimit(const std::string& client_id);
    
    /**
     * Check if a client is within rate limits for byte count
     * @param client_id The client identifier
     * @param bytes Number of bytes to check
     * @return True if within limits, false if rate limited
     */
    bool CheckByteRateLimit(const std::string& client_id, size_t bytes);
    
    /**
     * Remove rate limiting data for a client (cleanup on disconnect)
     * @param client_id The client identifier
     */
    void RemoveClient(const std::string& client_id);
    
    /**
     * Get rate limiting statistics for a client
     * @param client_id The client identifier
     * @return Statistics string (for debugging)
     */
    std::string GetClientStats(const std::string& client_id) const;

private:
    struct ClientLimits {
        std::unique_ptr<TokenBucketRateLimit> packet_limiter;
        std::unique_ptr<TokenBucketRateLimit> byte_limiter;
        std::chrono::steady_clock::time_point last_activity;
        size_t total_packets;
        size_t total_bytes;
    };
    
    mutable std::mutex clients_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ClientLimits>> client_limits_;
    RateLimitConfig config_;
    
    ClientLimits* GetOrCreateClientLimits(const std::string& client_id);
    void CleanupInactiveClients();
};

/**
 * DDoS protection implementation
 * Protects against connection flooding and resource exhaustion
 */
class DDoSProtection {
public:
    explicit DDoSProtection(const DDoSProtectionConfig& config);
    
    /**
     * Check if a new connection from an IP should be allowed
     * @param ip_address The IP address of the connecting client
     * @return True if connection should be allowed, false if blocked
     */
    bool AllowNewConnection(const std::string& ip_address);
    
    /**
     * Register a successful connection
     * @param ip_address The IP address of the client
     * @param connection_id Unique connection identifier
     */
    void RegisterConnection(const std::string& ip_address, const std::string& connection_id);
    
    /**
     * Remove a connection (cleanup on disconnect)
     * @param ip_address The IP address of the client
     * @param connection_id The connection identifier
     */
    void RemoveConnection(const std::string& ip_address, const std::string& connection_id);
    
    /**
     * Check if the global packet rate limit is exceeded
     * @return True if within limits, false if exceeded
     */
    bool CheckGlobalPacketRate();
    
    /**
     * Add an IP address to the blacklist
     * @param ip_address The IP address to blacklist
     * @param reason The reason for blacklisting
     */
    void BlacklistIP(const std::string& ip_address, const std::string& reason);
    
    /**
     * Check if an IP address is blacklisted
     * @param ip_address The IP address to check
     * @return True if blacklisted, false otherwise
     */
    bool IsBlacklisted(const std::string& ip_address) const;
    
    /**
     * Get DDoS protection statistics
     * @return Statistics string (for monitoring)
     */
    std::string GetProtectionStats() const;

private:
    struct IPInfo {
        size_t connection_count;
        std::vector<std::string> connection_ids;
        std::chrono::steady_clock::time_point first_connection;
        std::chrono::steady_clock::time_point last_activity;
    };
    
    struct BlacklistEntry {
        std::chrono::steady_clock::time_point blacklist_time;
        std::string reason;
    };
    
    mutable std::mutex protection_mutex_;
    DDoSProtectionConfig config_;
    std::atomic<size_t> active_connections_{0};
    std::unordered_map<std::string, IPInfo> ip_connections_;
    std::unordered_map<std::string, BlacklistEntry> blacklisted_ips_;
    
    // Global rate limiting
    std::unique_ptr<TokenBucketRateLimit> global_packet_limiter_;
    
    void CleanupExpiredBlacklist();
    void CleanupInactiveConnections();
    bool IsValidIPAddress(const std::string& ip_address) const;
};

/**
 * Comprehensive network security manager
 * Combines input validation, rate limiting, and DDoS protection
 */
class NetworkSecurityManager {
public:
    NetworkSecurityManager(
        const RateLimitConfig& rate_config,
        const DDoSProtectionConfig& ddos_config
    );
    
    /**
     * Validate and check rate limits for an incoming packet
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param data The packet data
     * @param protocol The protocol name
     * @return Validation result
     */
    ValidationResult ValidateIncomingPacket(
        const std::string& client_id,
        const std::string& client_ip,
        const std::vector<uint8_t>& data,
        const std::string& protocol
    );
    
    /**
     * Validate and check rate limits for an incoming JSON message
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param json_message The JSON message
     * @return Validation result
     */
    ValidationResult ValidateIncomingJsonMessage(
        const std::string& client_id,
        const std::string& client_ip,
        const std::string& json_message
    );
    
    /**
     * Check if a new connection should be allowed
     * @param client_ip The client IP address
     * @param connection_id The connection identifier
     * @return Validation result
     */
    ValidationResult ValidateNewConnection(
        const std::string& client_ip,
        const std::string& connection_id
    );
    
    /**
     * Remove a client (cleanup on disconnect)
     * @param client_id The client identifier
     * @param client_ip The client IP address
     * @param connection_id The connection identifier
     */
    void RemoveClient(
        const std::string& client_id,
        const std::string& client_ip,
        const std::string& connection_id
    );
    
    /**
     * Get comprehensive security statistics
     * @return Statistics string (for monitoring)
     */
    std::string GetSecurityStats() const;

private:
    std::unique_ptr<ClientRateManager> rate_manager_;
    std::unique_ptr<DDoSProtection> ddos_protection_;
};

} // namespace Core::Multiplayer::Security
