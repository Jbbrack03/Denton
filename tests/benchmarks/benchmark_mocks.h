// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <optional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>
#include <map>

/**
 * Mock implementations for performance benchmarking
 * These mocks provide controlled, predictable behavior for measuring performance
 */

namespace Benchmarks {

using namespace std::chrono;

// =============================================================================
// Basic Types and Enums
// =============================================================================

enum class MockErrorCode {
    Success = 0,
    ConnectionFailed,
    NetworkTimeout,
    InvalidParameter,
    ResourceExhausted,
    PermissionDenied,
    NotInitialized,
    InternalError
};

enum class MockPacketType {
    GameData = 1,
    Control = 2,
    Heartbeat = 3,
    Discovery = 4
};

enum class MockErrorCategory {
    NetworkConnectivity,
    PermissionDenied,
    ConfigurationError,
    ResourceExhausted
};

enum class MockErrorCondition {
    NetworkTimeout,
    ConnectionLost,
    InvalidPacket,
    ResourceExhausted,
    PermissionDenied
};

enum class MockRecoveryAction {
    Retry,
    Reconnect,
    Fallback,
    UserIntervention
};

enum class MockRecoveryResult {
    Success,
    Failed,
    PartialRecovery,
    RequiresUserAction
};

enum class MockBroadcastMode {
    Simple,
    Efficient,
    Parallel
};

enum class MockLockingStrategy {
    Pessimistic,
    Optimistic,
    LockFree
};

enum class MockContentionHandling {
    Block,
    Backoff,
    Skip
};

enum class MockValidationType {
    StringLength,
    IntegerRange,
    FloatRange,
    Regex
};

enum class MockTrackingGranularity {
    Coarse,
    Fine,
    Detailed
};

// =============================================================================
// Data Structures
// =============================================================================

struct MockPacket {
    std::vector<uint8_t> data;
    MockPacketType packet_type;
    high_resolution_clock::time_point timestamp;
    uint32_t sequence_number = 0;
    
    size_t GetSize() const { return data.size(); }
};

struct MockLdnPacketHeader {
    uint16_t magic;
    uint16_t version;
    uint32_t session_id;
    uint16_t packet_type;
    uint16_t payload_size;
    uint32_t crc32;
};

struct MockLdnPacket {
    MockLdnPacketHeader header;
    std::vector<uint8_t> payload;
};

struct MockServiceInfo {
    std::string service_name;
    std::string ip_address;
    uint16_t port;
    std::map<std::string, std::string> txt_records;
};

struct MockNetworkInfo {
    std::string ssid;
    int player_count;
    int max_players;
    bool has_password;
    int signal_strength;
};

struct MockNetworkConfig {
    std::string ssid;
    std::string passphrase;
    int max_players;
    int channel;
};

struct MockScanConfig {
    int timeout_ms;
    int max_results;
    std::string game_filter;
};

struct MockRoomInfo {
    std::string room_id;
    std::string game_title;
    int current_players;
    int max_players;
    bool has_password;
    std::string region;
};

struct MockRoomQuery {
    std::string game_filter;
    int max_results;
    bool include_full_rooms;
    bool include_password_protected;
};

struct MockHotspotConfiguration {
    std::string ssid;
    std::string passphrase;
    int max_clients;
    int channel;
};

struct MockWiFiDirectPeer {
    std::string device_address;
    std::string device_name;
    bool is_group_owner;
    int signal_strength;
};

struct MockErrorInfo {
    MockErrorCode error_code;
    MockErrorCategory category;
    std::string message;
    bool is_recoverable;
    MockRecoveryAction suggested_recovery;
};

struct MockValidationResult {
    bool is_valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// =============================================================================
// Mock Network Components
// =============================================================================

class MockWebSocketClient {
public:
    void SetLatency(milliseconds latency) { latency_ = latency; }
    void SetPacketLoss(double loss_rate) { packet_loss_rate_ = loss_rate; }
    void SetJitter(milliseconds jitter) { jitter_ = jitter; }
    
    MockErrorCode Connect(const std::string& url) {
        std::this_thread::sleep_for(latency_);
        return SimulatePacketLoss() ? MockErrorCode::ConnectionFailed : MockErrorCode::Success;
    }
    
    void Disconnect() {
        std::this_thread::sleep_for(latency_ / 2);
    }

private:
    milliseconds latency_{20};
    double packet_loss_rate_{0.0};
    milliseconds jitter_{0};
    
    bool SimulatePacketLoss() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen) < packet_loss_rate_;
    }
};

class MockP2PNetwork {
public:
    void SetConnectionSuccess(bool success) { connection_success_ = success; }
    void SetTimeoutDuration(milliseconds timeout) { timeout_duration_ = timeout; }
    void SetLatency(milliseconds latency) { latency_ = latency; }
    
    MockErrorCode EstablishConnection(const std::string& peer_id) {
        std::this_thread::sleep_for(connection_success_ ? latency_ : timeout_duration_);
        return connection_success_ ? MockErrorCode::Success : MockErrorCode::ConnectionFailed;
    }

private:
    bool connection_success_{true};
    milliseconds timeout_duration_{5000};
    milliseconds latency_{100};
};

class MockRelayClient {
public:
    void SetConnectionSuccess(bool success) { connection_success_ = success; }
    void SetLatency(milliseconds latency) { latency_ = latency; }
    
    MockErrorCode Connect(const std::string& relay_url) {
        std::this_thread::sleep_for(latency_);
        return connection_success_ ? MockErrorCode::Success : MockErrorCode::ConnectionFailed;
    }
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        std::this_thread::sleep_for(latency_ * 2); // Round trip
        return MockErrorCode::Success;
    }

private:
    bool connection_success_{true};
    milliseconds latency_{25};
};

class MockAdHocNetwork {
public:
    void SetBaseLatency(milliseconds latency) { base_latency_ = latency; }
    void SetProcessingOverhead(milliseconds overhead) { processing_overhead_ = overhead; }
    void SetJitter(milliseconds jitter) { jitter_ = jitter; }
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        auto total_latency = base_latency_ + processing_overhead_;
        
        // Add jitter
        if (jitter_.count() > 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(-jitter_.count(), jitter_.count());
            total_latency += milliseconds(dis(gen));
        }
        
        std::this_thread::sleep_for(total_latency);
        return MockErrorCode::Success;
    }

private:
    milliseconds base_latency_{1};
    milliseconds processing_overhead_{2};
    milliseconds jitter_{0};
};

class MockConnection {
public:
    void SetReconnectionTime(milliseconds time) { reconnection_time_ = time; }
    void SetReconnectionSuccess(bool success) { reconnection_success_ = success; }
    void SetLatency(milliseconds latency) { latency_ = latency; }
    void SetJitter(milliseconds jitter) { jitter_ = jitter; }
    
    MockErrorCode Connect(const std::string& endpoint = "") {
        std::this_thread::sleep_for(latency_);
        connected_ = true;
        return MockErrorCode::Success;
    }
    
    void Disconnect() {
        connected_ = false;
    }
    
    void SimulateDisconnect() {
        connected_ = false;
    }
    
    MockErrorCode Reconnect() {
        std::this_thread::sleep_for(reconnection_time_);
        connected_ = reconnection_success_;
        return reconnection_success_ ? MockErrorCode::Success : MockErrorCode::ConnectionFailed;
    }
    
    MockErrorCode SendPacketWithEcho(const MockPacket& packet) {
        if (!connected_) return MockErrorCode::ConnectionFailed;
        
        auto total_latency = latency_;
        
        // Add jitter
        if (jitter_.count() > 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(-jitter_.count(), jitter_.count());
            total_latency += milliseconds(dis(gen));
        }
        
        std::this_thread::sleep_for(total_latency);
        return MockErrorCode::Success;
    }

private:
    bool connected_{false};
    milliseconds reconnection_time_{500};
    bool reconnection_success_{true};
    milliseconds latency_{20};
    milliseconds jitter_{0};
};

// =============================================================================
// Mock Server Components
// =============================================================================

class MockRoomServer {
public:
    void SetMaxConnections(int max_conn) { max_connections_ = max_conn; }
    void SetConnectionTimeoutMs(int timeout) { connection_timeout_ms_ = timeout; }
    void SetResponseTime(milliseconds response_time) { response_time_ = response_time; }
    
    MockErrorCode HandleConnection() {
        std::this_thread::sleep_for(response_time_);
        
        if (current_connections_ >= max_connections_) {
            return MockErrorCode::ResourceExhausted;
        }
        
        current_connections_++;
        return MockErrorCode::Success;
    }
    
    void CloseConnection() {
        if (current_connections_ > 0) {
            current_connections_--;
        }
    }

private:
    int max_connections_{1000};
    int connection_timeout_ms_{5000};
    milliseconds response_time_{100};
    std::atomic<int> current_connections_{0};
};

class MockRelayServer {
public:
    void SetMaxSessions(int max_sessions) { max_sessions_ = max_sessions; }
    void SetBandwidthLimitPerSession(int kbps) { bandwidth_limit_kbps_ = kbps; }
    
    MockErrorCode CreateSession(const std::string& session_id) {
        if (active_sessions_ >= max_sessions_) {
            return MockErrorCode::ResourceExhausted;
        }
        
        active_sessions_++;
        return MockErrorCode::Success;
    }
    
    void CloseSession() {
        if (active_sessions_ > 0) {
            active_sessions_--;
        }
    }

private:
    int max_sessions_{1000};
    int bandwidth_limit_kbps_{1024};
    std::atomic<int> active_sessions_{0};
};

// =============================================================================
// Mock Discovery Components
// =============================================================================

class MockMdnsDiscovery {
public:
    void SetAvailableServices(const std::vector<MockServiceInfo>& services) {
        available_services_ = services;
    }
    
    void SetDiscoveryLatency(milliseconds latency) { discovery_latency_ = latency; }
    
    std::vector<MockServiceInfo> DiscoverServices(const std::string& service_type, int timeout_ms) {
        std::this_thread::sleep_for(discovery_latency_);
        return available_services_;
    }

private:
    std::vector<MockServiceInfo> available_services_;
    milliseconds discovery_latency_{500};
};

class MockRoomDirectory {
public:
    void AddRoom(const MockRoomInfo& room) {
        std::lock_guard<std::mutex> lock(rooms_mutex_);
        rooms_.push_back(room);
    }
    
    std::vector<MockRoomInfo> QueryRooms(const MockRoomQuery& query) {
        std::lock_guard<std::mutex> lock(rooms_mutex_);
        
        // Simulate query processing time
        auto processing_time = microseconds(rooms_.size() * 10); // 10μs per room
        std::this_thread::sleep_for(processing_time);
        
        std::vector<MockRoomInfo> results;
        for (const auto& room : rooms_) {
            if (results.size() >= static_cast<size_t>(query.max_results)) break;
            
            if (!query.game_filter.empty() && room.game_title != query.game_filter) continue;
            if (!query.include_full_rooms && room.current_players >= room.max_players) continue;
            if (!query.include_password_protected && room.has_password) continue;
            
            results.push_back(room);
        }
        
        return results;
    }

private:
    std::vector<MockRoomInfo> rooms_;
    std::mutex rooms_mutex_;
};

// =============================================================================
// Mock Protocol Components
// =============================================================================

class MockPacketProtocol {
public:
    std::vector<uint8_t> SerializePacket(const MockLdnPacket& packet) {
        // Simulate serialization overhead
        std::this_thread::sleep_for(microseconds(1));
        
        std::vector<uint8_t> result;
        result.resize(sizeof(MockLdnPacketHeader) + packet.payload.size());
        
        // Copy header
        std::memcpy(result.data(), &packet.header, sizeof(MockLdnPacketHeader));
        
        // Copy payload
        if (!packet.payload.empty()) {
            std::memcpy(result.data() + sizeof(MockLdnPacketHeader), 
                       packet.payload.data(), packet.payload.size());
        }
        
        return result;
    }
    
    MockLdnPacket DeserializePacket(const std::vector<uint8_t>& data) {
        // Simulate deserialization overhead
        std::this_thread::sleep_for(microseconds(1));
        
        MockLdnPacket packet;
        
        if (data.size() >= sizeof(MockLdnPacketHeader)) {
            std::memcpy(&packet.header, data.data(), sizeof(MockLdnPacketHeader));
            
            if (data.size() > sizeof(MockLdnPacketHeader)) {
                packet.payload.resize(data.size() - sizeof(MockLdnPacketHeader));
                std::memcpy(packet.payload.data(), 
                           data.data() + sizeof(MockLdnPacketHeader), 
                           packet.payload.size());
            }
        }
        
        return packet;
    }
};

class MockCRC32Calculator {
public:
    uint32_t CalculateCRC32(const std::vector<uint8_t>& data) {
        // Simulate CRC32 calculation time based on data size
        auto calc_time = microseconds(data.size() / 100); // 100 bytes per microsecond
        std::this_thread::sleep_for(calc_time);
        
        // Return mock CRC32 (in real implementation, this would be actual CRC32)
        return 0xDEADBEEF;
    }
};

// =============================================================================
// Mock Memory and Resource Management
// =============================================================================

class MockMemoryAPI {
public:
    static size_t GetHeapUsage() {
        // Simulate heap usage tracking
        return heap_usage_.load();
    }
    
    static size_t GetStackUsage() {
        return stack_usage_.load();
    }
    
    static size_t GetAllocationCount() {
        return allocation_count_.load();
    }
    
    static void SimulateAllocation(size_t bytes) {
        heap_usage_ += bytes;
        allocation_count_++;
    }
    
    static void SimulateDeallocation(size_t bytes) {
        heap_usage_ -= bytes;
    }

private:
    static inline std::atomic<size_t> heap_usage_{1024 * 1024}; // Start with 1MB baseline
    static inline std::atomic<size_t> stack_usage_{64 * 1024};  // 64KB stack baseline
    static inline std::atomic<size_t> allocation_count_{0};
};

class MockMemoryTracker {
public:
    void EnableLeakDetection(bool enable) { leak_detection_enabled_ = enable; }
    void SetTrackingGranularity(MockTrackingGranularity granularity) { granularity_ = granularity; }

private:
    bool leak_detection_enabled_{false};
    MockTrackingGranularity granularity_{MockTrackingGranularity::Coarse};
};

// =============================================================================
// Forward Declarations for Factory Functions
// =============================================================================

class MockRoomClient;
class MockP2PClient;
class MockAdHocClient;
class MockRelayClientConnection;
class MockNetworkManager;
class MockDiscoveryClient;
class MockWiFiDirectManager;
class MockPacketPipeline;
class MockPacketProcessor;
class MockHLEInterface;
class MockMultiplayerSession;
class MockPlayer;
class MockLdnService;
class MockMultiplayerBackend;
class MockConfigurationManager;
class MockConfiguration;
class MockErrorHandler;
class MockRecoveryManager;

// Factory function declarations
std::unique_ptr<MockRoomClient> CreateMockRoomClient(MockWebSocketClient* websocket);
std::unique_ptr<MockP2PClient> CreateMockP2PClient(MockP2PNetwork* network);
std::unique_ptr<MockAdHocClient> CreateMockAdHocClient(MockAdHocNetwork* network);
std::unique_ptr<MockRelayClientConnection> CreateMockRelayClientConnection(MockRelayClient* client);
std::unique_ptr<MockNetworkManager> CreateMockNetworkManager(MockP2PNetwork* p2p, MockRelayClient* relay);
std::unique_ptr<MockDiscoveryClient> CreateMockDiscoveryClient(MockMdnsDiscovery* mdns);
std::unique_ptr<MockWiFiDirectManager> CreateMockWiFiDirectManager(MockWiFiDirectWrapper* wrapper);
std::unique_ptr<MockPacketPipeline> CreateMockPacketPipeline(MockPacketProcessor* processor, MockHLEInterface* hle);

} // namespace Benchmarks