// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "benchmark_mocks.h"
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Benchmarks {

using namespace std::chrono;

// =============================================================================
// Test Data Generation Utilities
// =============================================================================

/**
 * Generate random binary data for testing
 */
inline std::vector<uint8_t> GenerateRandomData(size_t size) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    std::vector<uint8_t> data;
    data.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        data.push_back(dis(gen));
    }
    
    return data;
}

/**
 * Generate test data with specific patterns for validation
 */
inline std::vector<uint8_t> GenerateTestData(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<uint8_t>(i % 256));
    }
    
    return data;
}

/**
 * Generate realistic game packet data
 */
inline std::vector<uint8_t> GenerateGamePacket(size_t size) {
    std::vector<uint8_t> packet;
    packet.reserve(size);
    
    // Simulate typical game packet structure
    // Header: 4 bytes
    packet.push_back(0x47); // 'G'
    packet.push_back(0x41); // 'A'
    packet.push_back(0x4D); // 'M'
    packet.push_back(0x45); // 'E'
    
    // Player ID: 4 bytes
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x01);
    
    // Timestamp: 8 bytes
    auto now = high_resolution_clock::now().time_since_epoch().count();
    for (int i = 0; i < 8; ++i) {
        packet.push_back(static_cast<uint8_t>((now >> (i * 8)) & 0xFF));
    }
    
    // Fill remaining space with simulated game data
    size_t remaining = size - packet.size();
    auto game_data = GenerateRandomData(remaining);
    packet.insert(packet.end(), game_data.begin(), game_data.end());
    
    return packet;
}

// =============================================================================
// Performance Measurement Utilities
// =============================================================================

/**
 * High-precision timer for micro-benchmarks
 */
class PrecisionTimer {
public:
    void Start() {
        start_time_ = high_resolution_clock::now();
    }
    
    void Stop() {
        end_time_ = high_resolution_clock::now();
    }
    
    double GetElapsedMicroseconds() const {
        return duration_cast<microseconds>(end_time_ - start_time_).count();
    }
    
    double GetElapsedMilliseconds() const {
        return duration_cast<nanoseconds>(end_time_ - start_time_).count() / 1000000.0;
    }
    
    double GetElapsedSeconds() const {
        return duration_cast<nanoseconds>(end_time_ - start_time_).count() / 1000000000.0;
    }

private:
    high_resolution_clock::time_point start_time_;
    high_resolution_clock::time_point end_time_;
};

/**
 * Throughput calculator for bandwidth measurements
 */
class ThroughputCalculator {
public:
    void StartMeasurement() {
        start_time_ = high_resolution_clock::now();
        bytes_processed_ = 0;
        operations_count_ = 0;
    }
    
    void RecordBytes(size_t bytes) {
        bytes_processed_ += bytes;
    }
    
    void RecordOperation() {
        operations_count_++;
    }
    
    double GetThroughputMBps() const {
        auto duration = high_resolution_clock::now() - start_time_;
        double seconds = duration_cast<nanoseconds>(duration).count() / 1000000000.0;
        return (bytes_processed_ / (1024.0 * 1024.0)) / seconds;
    }
    
    double GetOperationsPerSecond() const {
        auto duration = high_resolution_clock::now() - start_time_;
        double seconds = duration_cast<nanoseconds>(duration).count() / 1000000000.0;
        return operations_count_ / seconds;
    }

private:
    high_resolution_clock::time_point start_time_;
    std::atomic<size_t> bytes_processed_{0};
    std::atomic<size_t> operations_count_{0};
};

/**
 * Statistical analyzer for performance metrics
 */
class StatisticalAnalyzer {
public:
    void AddSample(double value) {
        samples_.push_back(value);
    }
    
    double GetMean() const {
        if (samples_.empty()) return 0.0;
        double sum = 0.0;
        for (double sample : samples_) {
            sum += sample;
        }
        return sum / samples_.size();
    }
    
    double GetMedian() const {
        if (samples_.empty()) return 0.0;
        
        auto sorted_samples = samples_;
        std::sort(sorted_samples.begin(), sorted_samples.end());
        
        size_t mid = sorted_samples.size() / 2;
        if (sorted_samples.size() % 2 == 0) {
            return (sorted_samples[mid - 1] + sorted_samples[mid]) / 2.0;
        } else {
            return sorted_samples[mid];
        }
    }
    
    double GetPercentile(double percentile) const {
        if (samples_.empty()) return 0.0;
        
        auto sorted_samples = samples_;
        std::sort(sorted_samples.begin(), sorted_samples.end());
        
        size_t index = static_cast<size_t>(samples_.size() * percentile / 100.0);
        if (index >= sorted_samples.size()) {
            index = sorted_samples.size() - 1;
        }
        
        return sorted_samples[index];
    }
    
    double GetStandardDeviation() const {
        if (samples_.size() < 2) return 0.0;
        
        double mean = GetMean();
        double variance = 0.0;
        
        for (double sample : samples_) {
            variance += (sample - mean) * (sample - mean);
        }
        
        variance /= (samples_.size() - 1);
        return std::sqrt(variance);
    }

private:
    std::vector<double> samples_;
};

// =============================================================================
// Network Simulation Utilities
// =============================================================================

/**
 * Network condition simulator for testing under various conditions
 */
class NetworkConditionSimulator {
public:
    struct NetworkCondition {
        milliseconds latency{20};
        double packet_loss_rate{0.0};
        milliseconds jitter{0};
        int bandwidth_kbps{1000};
        
        static NetworkCondition GoodNetwork() {
            return {20ms, 0.0, 1ms, 10000}; // 20ms latency, no loss, 1ms jitter, 10Mbps
        }
        
        static NetworkCondition MobileNetwork() {
            return {100ms, 0.01, 20ms, 1000}; // 100ms latency, 1% loss, 20ms jitter, 1Mbps
        }
        
        static NetworkCondition PoorNetwork() {
            return {300ms, 0.05, 50ms, 256}; // 300ms latency, 5% loss, 50ms jitter, 256Kbps
        }
        
        static NetworkCondition LocalNetwork() {
            return {1ms, 0.0, 0ms, 100000}; // 1ms latency, no loss, no jitter, 100Mbps
        }
    };
    
    void SetCondition(const NetworkCondition& condition) {
        condition_ = condition;
    }
    
    void SimulateNetworkDelay() {
        // Base latency
        std::this_thread::sleep_for(condition_.latency);
        
        // Add jitter if configured
        if (condition_.jitter.count() > 0) {
            static thread_local std::random_device rd;
            static thread_local std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(-condition_.jitter.count(), condition_.jitter.count());
            
            auto jitter_delay = milliseconds(dis(gen));
            std::this_thread::sleep_for(jitter_delay);
        }
    }
    
    bool ShouldDropPacket() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        static thread_local std::uniform_real_distribution<> dis(0.0, 1.0);
        
        return dis(gen) < condition_.packet_loss_rate;
    }

private:
    NetworkCondition condition_;
};

// =============================================================================
// Mock Component Implementations
// =============================================================================

/**
 * Mock implementations of component interfaces for benchmarking
 */

class MockRoomClient {
public:
    MockRoomClient(MockWebSocketClient* websocket) : websocket_(websocket) {}
    
    MockErrorCode Connect(const std::string& url) {
        return websocket_->Connect(url);
    }
    
    void Disconnect() {
        websocket_->Disconnect();
    }

private:
    MockWebSocketClient* websocket_;
};

class MockP2PClient {
public:
    MockP2PClient(MockP2PNetwork* network) : network_(network) {}
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        // Simulate P2P packet processing
        std::this_thread::sleep_for(microseconds(100));
        return MockErrorCode::Success;
    }

private:
    MockP2PNetwork* network_;
};

class MockAdHocClient {
public:
    MockAdHocClient(MockAdHocNetwork* network) : network_(network) {}
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        return network_->SendPacketWithEcho(packet);
    }

private:
    MockAdHocNetwork* network_;
};

class MockRelayClientConnection {
public:
    MockRelayClientConnection(MockRelayClient* client) : client_(client) {}
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        return client_->SendPacketWithEcho(packet);
    }

private:
    MockRelayClient* client_;
};

class MockNetworkManager {
public:
    MockNetworkManager(MockP2PNetwork* p2p, MockRelayClient* relay) 
        : p2p_network_(p2p), relay_client_(relay) {}
    
    MockErrorCode EstablishConnection(const std::string& peer_id) {
        // Try P2P first
        auto p2p_result = p2p_network_->EstablishConnection(peer_id);
        if (p2p_result == MockErrorCode::Success) {
            return MockErrorCode::Success;
        }
        
        // Fallback to relay
        return relay_client_->Connect("relay://server.com");
    }
    
    void Disconnect() {
        // Cleanup both connections
    }

private:
    MockP2PNetwork* p2p_network_;
    MockRelayClient* relay_client_;
};

class MockDiscoveryClient {
public:
    MockDiscoveryClient(MockMdnsDiscovery* mdns) : mdns_(mdns) {}
    
    std::vector<MockServiceInfo> ScanForServices(const std::string& service_type, int timeout_ms) {
        return mdns_->DiscoverServices(service_type, timeout_ms);
    }

private:
    MockMdnsDiscovery* mdns_;
};

class MockWiFiDirectWrapper {
public:
    void SetConnectionSuccess(bool success) { connection_success_ = success; }
    void SetConnectionTime(milliseconds time) { connection_time_ = time; }
    void AddMockPeer(const MockWiFiDirectPeer& peer) { mock_peers_.push_back(peer); }
    
    std::vector<MockWiFiDirectPeer> DiscoverPeers(int timeout_ms) {
        std::this_thread::sleep_for(milliseconds(timeout_ms / 10)); // Simulate discovery
        return mock_peers_;
    }
    
    MockErrorCode ConnectToPeer(const std::string& device_address) {
        std::this_thread::sleep_for(connection_time_);
        return connection_success_ ? MockErrorCode::Success : MockErrorCode::ConnectionFailed;
    }
    
    void Disconnect() {
        std::this_thread::sleep_for(milliseconds(100));
    }

private:
    bool connection_success_{true};
    milliseconds connection_time_{1000};
    std::vector<MockWiFiDirectPeer> mock_peers_;
};

class MockWiFiDirectManager {
public:
    MockWiFiDirectManager(MockWiFiDirectWrapper* wrapper) : wrapper_(wrapper) {}
    
    MockErrorCode ConnectToPeer(const std::string& device_address) {
        return wrapper_->ConnectToPeer(device_address);
    }
    
    void Disconnect() {
        wrapper_->Disconnect();
    }

private:
    MockWiFiDirectWrapper* wrapper_;
};

class MockPacketProcessor {
public:
    void SetProcessingTime(milliseconds time) { processing_time_ = time; }
    
    MockErrorCode ProcessPacket(const MockPacket& packet) {
        std::this_thread::sleep_for(processing_time_);
        return MockErrorCode::Success;
    }

private:
    milliseconds processing_time_{1};
};

class MockHLEInterface {
public:
    void SetDeliveryTime(milliseconds time) { delivery_time_ = time; }
    
    MockErrorCode DeliverPacket(const MockPacket& packet) {
        std::this_thread::sleep_for(delivery_time_);
        return MockErrorCode::Success;
    }

private:
    milliseconds delivery_time_{0};
};

class MockPacketPipeline {
public:
    MockPacketPipeline(MockPacketProcessor* processor, MockHLEInterface* hle)
        : processor_(processor), hle_interface_(hle) {}
    
    MockErrorCode ProcessIncomingPacket(const MockPacket& packet) {
        auto result = processor_->ProcessPacket(packet);
        if (result != MockErrorCode::Success) {
            return result;
        }
        
        return hle_interface_->DeliverPacket(packet);
    }

private:
    MockPacketProcessor* processor_;
    MockHLEInterface* hle_interface_;
};

class MockPacketQueue {
public:
    MockErrorCode ProcessPacket(const MockPacket& packet) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // Simulate packet processing
        std::this_thread::sleep_for(microseconds(10));
        
        processed_count_++;
        return MockErrorCode::Success;
    }
    
    size_t GetProcessedCount() const {
        return processed_count_.load();
    }

private:
    std::mutex queue_mutex_;
    std::atomic<size_t> processed_count_{0};
};

class ThreadSafePacketQueue {
public:
    MockErrorCode ProcessPacket(const MockPacket& packet) {
        std::lock_guard<std::mutex> lock(processing_mutex_);
        
        // Simulate thread-safe packet processing
        std::this_thread::sleep_for(microseconds(50));
        
        return MockErrorCode::Success;
    }

private:
    std::mutex processing_mutex_;
};

class MockMultiplayerSession {
public:
    void SetMaxPlayers(int max_players) { max_players_ = max_players; }
    void SetPacketBroadcastMode(MockBroadcastMode mode) { broadcast_mode_ = mode; }
    void SetPlayerCount(int count) { player_count_ = count; }
    
    MockErrorCode AddPlayer(MockPlayer* player) {
        if (players_.size() >= static_cast<size_t>(max_players_)) {
            return MockErrorCode::ResourceExhausted;
        }
        players_.push_back(player);
        return MockErrorCode::Success;
    }
    
    void RemovePlayer(MockPlayer* player) {
        players_.erase(std::remove(players_.begin(), players_.end(), player), players_.end());
    }
    
    MockErrorCode BroadcastPacket(const MockPacket& packet) {
        // Simulate broadcast latency based on player count
        auto broadcast_latency = microseconds(players_.size() * 10);
        std::this_thread::sleep_for(broadcast_latency);
        
        // Simulate packet delivery to all players
        for (auto* player : players_) {
            DeliverPacketToPlayer(player, packet);
        }
        
        return MockErrorCode::Success;
    }

private:
    void DeliverPacketToPlayer(MockPlayer* player, const MockPacket& packet);
    
    int max_players_{8};
    int player_count_{0};
    MockBroadcastMode broadcast_mode_{MockBroadcastMode::Efficient};
    std::vector<MockPlayer*> players_;
};

class MockPlayer {
public:
    MockPlayer(const std::string& name) : name_(name) {}
    
    bool HasReceivedPacket(const MockPacket& packet) const {
        // Simulate packet delivery check
        return received_packet_count_ > 0;
    }
    
    void ReceivePacket(const MockPacket& packet) {
        received_packet_count_++;
    }

private:
    std::string name_;
    std::atomic<int> received_packet_count_{0};
};

class MockNetworkInterface {
public:
    MockErrorCode SendPacket(const MockPacket& packet) {
        // Simulate network send time
        auto send_time = microseconds(packet.data.size() / 100); // 100 bytes per microsecond
        std::this_thread::sleep_for(send_time);
        return MockErrorCode::Success;
    }
};

class ThreadSafeResourceManager {
public:
    void SetLockingStrategy(MockLockingStrategy strategy) { locking_strategy_ = strategy; }
    void SetContentionHandling(MockContentionHandling handling) { contention_handling_ = handling; }
    
    MockErrorCode AccessResource(const std::string& resource_id) {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        
        // Simulate resource access time
        std::this_thread::sleep_for(microseconds(10));
        
        return MockErrorCode::Success;
    }

private:
    MockLockingStrategy locking_strategy_{MockLockingStrategy::Optimistic};
    MockContentionHandling contention_handling_{MockContentionHandling::Backoff};
    std::mutex resource_mutex_;
};

// =============================================================================
// Factory Function Implementations
// =============================================================================

inline std::unique_ptr<MockRoomClient> CreateMockRoomClient(MockWebSocketClient* websocket) {
    return std::make_unique<MockRoomClient>(websocket);
}

inline std::unique_ptr<MockP2PClient> CreateMockP2PClient(MockP2PNetwork* network) {
    return std::make_unique<MockP2PClient>(network);
}

inline std::unique_ptr<MockAdHocClient> CreateMockAdHocClient(MockAdHocNetwork* network) {
    return std::make_unique<MockAdHocClient>(network);
}

inline std::unique_ptr<MockRelayClientConnection> CreateMockRelayClientConnection(MockRelayClient* client) {
    return std::make_unique<MockRelayClientConnection>(client);
}

inline std::unique_ptr<MockNetworkManager> CreateMockNetworkManager(MockP2PNetwork* p2p, MockRelayClient* relay) {
    return std::make_unique<MockNetworkManager>(p2p, relay);
}

inline std::unique_ptr<MockDiscoveryClient> CreateMockDiscoveryClient(MockMdnsDiscovery* mdns) {
    return std::make_unique<MockDiscoveryClient>(mdns);
}

inline std::unique_ptr<MockWiFiDirectManager> CreateMockWiFiDirectManager(MockWiFiDirectWrapper* wrapper) {
    return std::make_unique<MockWiFiDirectManager>(wrapper);
}

inline std::unique_ptr<MockPacketPipeline> CreateMockPacketPipeline(MockPacketProcessor* processor, MockHLEInterface* hle) {
    return std::make_unique<MockPacketPipeline>(processor, hle);
}

// Implement the missing method
inline void MockMultiplayerSession::DeliverPacketToPlayer(MockPlayer* player, const MockPacket& packet) {
    player->ReceivePacket(packet);
}

} // namespace Benchmarks