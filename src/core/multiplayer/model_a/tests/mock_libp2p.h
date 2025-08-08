// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <memory>

namespace Core::Multiplayer::ModelA::Test {

/**
 * Mock libp2p Host interface
 * Represents the main libp2p host that manages the networking stack
 */
class MockLibp2pHost {
public:
    virtual ~MockLibp2pHost() = default;

    // Core lifecycle methods
    MOCK_METHOD(void, start, (), ());
    MOCK_METHOD(void, stop, (), ());
    MOCK_METHOD(bool, isStarted, (), (const));
    
    // Identity and addressing
    MOCK_METHOD(std::string, getId, (), (const));
    MOCK_METHOD(std::vector<std::string>, getListenAddresses, (), (const));
    MOCK_METHOD(std::vector<std::string>, getAddresses, (), (const));
    
    // Connection management
    MOCK_METHOD(void, connect, (const std::string& multiaddr), ());
    MOCK_METHOD(void, disconnect, (const std::string& peer_id), ());
    MOCK_METHOD(bool, isConnected, (const std::string& peer_id), (const));
    MOCK_METHOD(std::vector<std::string>, getConnectedPeers, (), (const));
    MOCK_METHOD(size_t, getConnectionCount, (), (const));
    
    // Protocol handling
    MOCK_METHOD(void, setProtocolHandler, (const std::string& protocol, std::function<void(const std::string&, const std::vector<uint8_t>&)> handler), ());
    MOCK_METHOD(void, removeProtocolHandler, (const std::string& protocol), ());
    
    // Message transmission
    MOCK_METHOD(bool, sendMessage, (const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data), ());
    MOCK_METHOD(void, broadcast, (const std::string& protocol, const std::vector<uint8_t>& data), ());
    
    // Event callbacks
    MOCK_METHOD(void, setOnPeerConnectedCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, setOnPeerDisconnectedCallback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, setOnConnectionFailedCallback, (std::function<void(const std::string&, const std::string&)> callback), ());
};

/**
 * Mock Transport Manager interface
 * Manages different transport protocols (TCP, QUIC, WebSocket)
 */
class MockTransportManager {
public:
    virtual ~MockTransportManager() = default;

    // Transport configuration
    MOCK_METHOD(void, addTransport, (const std::string& transport_type, uint16_t port), ());
    MOCK_METHOD(void, removeTransport, (const std::string& transport_type), ());
    MOCK_METHOD(bool, hasTransport, (const std::string& transport_type), (const));
    MOCK_METHOD(std::vector<std::string>, getEnabledTransports, (), (const));
    
    // Transport-specific methods
    MOCK_METHOD(void, configureTCP, (uint16_t port, bool enable_reuseaddr), ());
    MOCK_METHOD(void, configureQUIC, (uint16_t port, const std::string& cert_path, const std::string& key_path), ());
    MOCK_METHOD(void, configureWebSocket, (uint16_t port, const std::string& path), ());
    MOCK_METHOD(void, configureWebSocketSecure, (uint16_t port, const std::string& path, const std::string& cert_path, const std::string& key_path), ());
    
    // Connection preferences
    MOCK_METHOD(void, setTransportPriority, (const std::string& transport_type, int priority), ());
    MOCK_METHOD(int, getTransportPriority, (const std::string& transport_type), (const));
    MOCK_METHOD(std::string, selectBestTransport, (const std::vector<std::string>& available_transports), (const));
};

/**
 * Mock Security Manager interface
 * Handles Noise protocol and encryption
 */
class MockSecurityManager {
public:
    virtual ~MockSecurityManager() = default;

    // Noise protocol configuration
    MOCK_METHOD(void, initializeNoise, (const std::string& static_private_key), ());
    MOCK_METHOD(std::string, getPublicKey, (), (const));
    MOCK_METHOD(std::string, getPrivateKey, (), (const));
    MOCK_METHOD(bool, isNoiseInitialized, (), (const));
    
    // Handshake management
    MOCK_METHOD(bool, performHandshake, (const std::string& peer_id, const std::string& remote_public_key), ());
    MOCK_METHOD(bool, isHandshakeComplete, (const std::string& peer_id), (const));
    MOCK_METHOD(void, resetHandshake, (const std::string& peer_id), ());
    
    // Encryption/Decryption
    MOCK_METHOD(std::vector<uint8_t>, encrypt, (const std::string& peer_id, const std::vector<uint8_t>& plaintext), ());
    MOCK_METHOD(std::vector<uint8_t>, decrypt, (const std::string& peer_id, const std::vector<uint8_t>& ciphertext), ());
    
    // Key management
    MOCK_METHOD(void, rotateKeys, (const std::string& peer_id), ());
    MOCK_METHOD(std::chrono::system_clock::time_point, getLastKeyRotation, (const std::string& peer_id), (const));
    
    // Certificate handling
    MOCK_METHOD(bool, verifyCertificate, (const std::string& certificate), (const));
    MOCK_METHOD(std::string, generateSelfSignedCertificate, (), ());
};

/**
 * Mock AutoNAT Service interface
 * Handles NAT detection and traversal
 */
class MockAutoNATService {
public:
    virtual ~MockAutoNATService() = default;

    enum class NATType {
        UNKNOWN,
        FULL_CONE,
        RESTRICTED_CONE,
        PORT_RESTRICTED_CONE,
        SYMMETRIC,
        NO_NAT
    };

    // NAT detection
    MOCK_METHOD(void, detectNATType, (), ());
    MOCK_METHOD(NATType, getNATType, (), (const));
    MOCK_METHOD(bool, isReachable, (), (const));
    MOCK_METHOD(std::string, getExternalAddress, (), (const));
    
    // Traversal capabilities
    MOCK_METHOD(bool, canTraverseNAT, (NATType local_nat, NATType remote_nat), (const));
    MOCK_METHOD(std::vector<std::string>, getTraversalStrategies, (NATType local_nat, NATType remote_nat), (const));
    
    // Hole punching
    MOCK_METHOD(bool, attemptHolePunch, (const std::string& peer_id, const std::string& peer_multiaddr), ());
    MOCK_METHOD(void, setHolePunchTimeout, (std::chrono::milliseconds timeout), ());
    MOCK_METHOD(std::chrono::milliseconds, getHolePunchTimeout, (), (const));
    
    // Callbacks
    MOCK_METHOD(void, setOnNATDetectedCallback, (std::function<void(NATType)> callback), ());
    MOCK_METHOD(void, setOnHolePunchResultCallback, (std::function<void(const std::string&, bool)> callback), ());
};

/**
 * Mock Circuit Relay interface
 * Handles relay connections when direct P2P fails
 */
class MockCircuitRelay {
public:
    virtual ~MockCircuitRelay() = default;

    // Relay server management
    MOCK_METHOD(void, addRelayServer, (const std::string& relay_multiaddr), ());
    MOCK_METHOD(void, removeRelayServer, (const std::string& relay_multiaddr), ());
    MOCK_METHOD(std::vector<std::string>, getRelayServers, (), (const));
    MOCK_METHOD(std::string, selectBestRelay, (const std::string& target_peer_id), (const));
    
    // Relay connections
    MOCK_METHOD(bool, connectViaRelay, (const std::string& peer_id, const std::string& relay_multiaddr), ());
    MOCK_METHOD(bool, isConnectedViaRelay, (const std::string& peer_id), (const));
    MOCK_METHOD(std::string, getRelayForPeer, (const std::string& peer_id), (const));
    
    // Performance monitoring
    MOCK_METHOD(uint64_t, getRelayLatency, (const std::string& relay_multiaddr), (const));
    MOCK_METHOD(uint64_t, getRelayBandwidth, (const std::string& relay_multiaddr), (const));
    MOCK_METHOD(bool, isRelayHealthy, (const std::string& relay_multiaddr), (const));
    
    // Configuration
    MOCK_METHOD(void, setMaxRelayConnections, (size_t max_connections), ());
    MOCK_METHOD(size_t, getMaxRelayConnections, (), (const));
    MOCK_METHOD(void, setRelayTimeout, (std::chrono::milliseconds timeout), ());
    
    // Callbacks
    MOCK_METHOD(void, setOnRelayConnectedCallback, (std::function<void(const std::string&, const std::string&)> callback), ());
    MOCK_METHOD(void, setOnRelayDisconnectedCallback, (std::function<void(const std::string&, const std::string&)> callback), ());
    MOCK_METHOD(void, setOnRelayFailedCallback, (std::function<void(const std::string&, const std::string&)> callback), ());
};

/**
 * Mock Performance Monitor interface
 * Tracks P2P network performance metrics
 */
class MockPerformanceMonitor {
public:
    virtual ~MockPerformanceMonitor() = default;

    struct ConnectionMetrics {
        std::chrono::milliseconds latency{0};
        uint64_t bytes_sent{0};
        uint64_t bytes_received{0};
        uint64_t messages_sent{0};
        uint64_t messages_received{0};
        std::chrono::system_clock::time_point connected_at;
        std::chrono::system_clock::time_point last_activity;
    };

    // Metrics collection
    MOCK_METHOD(void, recordConnectionEstablished, (const std::string& peer_id, std::chrono::milliseconds duration), ());
    MOCK_METHOD(void, recordMessageSent, (const std::string& peer_id, size_t bytes), ());
    MOCK_METHOD(void, recordMessageReceived, (const std::string& peer_id, size_t bytes), ());
    MOCK_METHOD(void, recordLatency, (const std::string& peer_id, std::chrono::milliseconds latency), ());
    
    // Metrics retrieval
    MOCK_METHOD(ConnectionMetrics, getConnectionMetrics, (const std::string& peer_id), (const));
    MOCK_METHOD(std::vector<std::string>, getActiveConnections, (), (const));
    MOCK_METHOD(double, getAverageLatency, (), (const));
    MOCK_METHOD(uint64_t, getTotalBytesTransferred, (), (const));
    MOCK_METHOD(uint64_t, getTotalMessagesTransferred, (), (const));
    
    // Performance analysis
    MOCK_METHOD(bool, isConnectionHealthy, (const std::string& peer_id), (const));
    MOCK_METHOD(std::vector<std::string>, getSlowConnections, (std::chrono::milliseconds threshold), (const));
    MOCK_METHOD(void, setPerformanceThresholds, (std::chrono::milliseconds latency_threshold, uint64_t bandwidth_threshold), ());
    
    // Reporting
    MOCK_METHOD(std::string, generatePerformanceReport, (), (const));
    MOCK_METHOD(void, resetMetrics, (), ());
    MOCK_METHOD(void, setMetricsCallback, (std::function<void(const std::string&, const ConnectionMetrics&)> callback), ());
};

} // namespace Core::Multiplayer::ModelA::Test
