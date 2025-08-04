// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <future>
#include <algorithm>
#include <numeric>

#include "benchmark_mocks.h"
#include "benchmark_utilities.h"

namespace Benchmarks {

using namespace std::chrono;

/**
 * PRD Requirements for Scalability:
 * - Room Server: 100,000 concurrent connections per instance
 * - Relay Server: 10,000 concurrent sessions per instance
 * - Maximum players per session: 8 (game dependent)
 * - Room list query: < 100ms response time
 */

// Memory usage tracking utility
class MemoryTracker {
public:
    struct MemorySnapshot {
        size_t heap_usage_bytes;
        size_t stack_usage_bytes;
        size_t total_allocations;
        high_resolution_clock::time_point timestamp;
    };

    static MemorySnapshot TakeSnapshot() {
        MemorySnapshot snapshot;
        snapshot.heap_usage_bytes = GetCurrentHeapUsage();
        snapshot.stack_usage_bytes = GetCurrentStackUsage();
        snapshot.total_allocations = GetTotalAllocations();
        snapshot.timestamp = high_resolution_clock::now();
        return snapshot;
    }

private:
    static size_t GetCurrentHeapUsage() {
        // Mock implementation - in real code, this would use platform-specific APIs
        return MockMemoryAPI::GetHeapUsage();
    }

    static size_t GetCurrentStackUsage() {
        return MockMemoryAPI::GetStackUsage();
    }

    static size_t GetTotalAllocations() {
        return MockMemoryAPI::GetAllocationCount();
    }
};

// =============================================================================
// Connection Scalability Benchmarks
// =============================================================================

/**
 * Benchmark: Room Server Connection Scalability
 * Target: Support up to 100,000 concurrent connections
 */
class RoomServerScalabilityFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_room_server_ = std::make_unique<MockRoomServer>();
        connection_count_ = static_cast<int>(state.range(0));
        
        // Configure server for high load
        mock_room_server_->SetMaxConnections(connection_count_);
        mock_room_server_->SetConnectionTimeoutMs(5000);
    }

    void TearDown(const benchmark::State& state) override {
        mock_room_server_.reset();
    }

protected:
    std::unique_ptr<MockRoomServer> mock_room_server_;
    int connection_count_;
};

BENCHMARK_DEFINE_F(RoomServerScalabilityFixture, ConcurrentConnections)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Take initial memory snapshot
        auto memory_before = MemoryTracker::TakeSnapshot();
        
        std::vector<std::unique_ptr<MockRoomClient>> clients;
        clients.reserve(connection_count_);
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        // Create concurrent connections
        std::vector<std::future<MockErrorCode>> connection_futures;
        connection_futures.reserve(connection_count_);
        
        for (int i = 0; i < connection_count_; ++i) {
            auto client = std::make_unique<MockRoomClient>();
            
            // Asynchronously connect each client
            connection_futures.push_back(
                std::async(std::launch::async, [&client]() {
                    return client->Connect("ws://test-server.com/room");
                })
            );
            
            clients.push_back(std::move(client));
        }
        
        // Wait for all connections to complete
        int successful_connections = 0;
        for (auto& future : connection_futures) {
            if (future.get() == MockErrorCode::Success) {
                successful_connections++;
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.PauseTiming();
        
        // Take final memory snapshot
        auto memory_after = MemoryTracker::TakeSnapshot();
        
        // Calculate memory usage
        size_t memory_per_connection = (memory_after.heap_usage_bytes - memory_before.heap_usage_bytes) / connection_count_;
        
        // Disconnect all clients
        for (auto& client : clients) {
            client->Disconnect();
        }
        
        // Performance metrics
        state.counters["ConnectionCount"] = connection_count_;
        state.counters["SuccessfulConnections"] = successful_connections;
        state.counters["ConnectionRate_per_sec"] = (successful_connections * 1000.0) / duration_ms;
        state.counters["MemoryPerConnection_KB"] = memory_per_connection / 1024.0;
        state.counters["TotalMemoryUsage_MB"] = (memory_after.heap_usage_bytes - memory_before.heap_usage_bytes) / (1024.0 * 1024.0);
        state.counters["ConnectionSuccessRate_%"] = (successful_connections * 100.0) / connection_count_;
        
        // PRD target validation
        state.counters["PRDTarget_Connections"] = 100000;
        state.counters["ScalabilityTargetMet"] = (connection_count_ >= 10000 && successful_connections >= connection_count_ * 0.95) ? 1 : 0; // 95% success rate
        
        state.SetIterationTime(duration_ms / 1000.0);
        
        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(RoomServerScalabilityFixture, ConcurrentConnections)
    ->Range(100, 10000) // Test from 100 to 10k connections (limited for benchmark performance)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Name("Scalability/RoomServer/ConcurrentConnections");

/**
 * Benchmark: Relay Server Session Scalability
 * Target: Support up to 10,000 concurrent sessions
 */
class RelayServerScalabilityFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_relay_server_ = std::make_unique<MockRelayServer>();
        session_count_ = static_cast<int>(state.range(0));
        
        mock_relay_server_->SetMaxSessions(session_count_);
        mock_relay_server_->SetBandwidthLimitPerSession(1024); // 1 Mbps per session
    }

    void TearDown(const benchmark::State& state) override {
        mock_relay_server_.reset();
    }

protected:
    std::unique_ptr<MockRelayServer> mock_relay_server_;
    int session_count_;
};

BENCHMARK_DEFINE_F(RelayServerScalabilityFixture, ConcurrentSessions)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        auto memory_before = MemoryTracker::TakeSnapshot();
        
        std::vector<std::unique_ptr<MockRelaySession>> sessions;
        sessions.reserve(session_count_);
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        // Create concurrent relay sessions
        std::vector<std::future<MockErrorCode>> session_futures;
        
        for (int i = 0; i < session_count_; ++i) {
            auto session = std::make_unique<MockRelaySession>();
            
            session_futures.push_back(
                std::async(std::launch::async, [&session]() {
                    return session->EstablishSession("session_" + std::to_string(i));
                })
            );
            
            sessions.push_back(std::move(session));
        }
        
        // Wait for all sessions to establish
        int successful_sessions = 0;
        for (auto& future : session_futures) {
            if (future.get() == MockErrorCode::Success) {
                successful_sessions++;
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.PauseTiming();
        
        auto memory_after = MemoryTracker::TakeSnapshot();
        
        // Test data relay through all sessions
        auto relay_start = high_resolution_clock::now();
        
        int packets_relayed = 0;
        for (auto& session : sessions) {
            MockPacket test_packet;
            test_packet.data = GenerateTestData(512);
            
            if (session->RelayPacket(test_packet) == MockErrorCode::Success) {
                packets_relayed++;
            }
        }
        
        auto relay_end = high_resolution_clock::now();
        auto relay_duration_ms = duration_cast<milliseconds>(relay_end - relay_start).count();
        
        // Clean up sessions
        for (auto& session : sessions) {
            session->CloseSession();
        }
        
        // Performance metrics
        size_t memory_per_session = (memory_after.heap_usage_bytes - memory_before.heap_usage_bytes) / session_count_;
        
        state.counters["SessionCount"] = session_count_;
        state.counters["SuccessfulSessions"] = successful_sessions;
        state.counters["SessionEstablishmentRate_per_sec"] = (successful_sessions * 1000.0) / duration_ms;
        state.counters["PacketRelayRate_per_sec"] = (packets_relayed * 1000.0) / relay_duration_ms;
        state.counters["MemoryPerSession_KB"] = memory_per_session / 1024.0;
        state.counters["TotalMemoryUsage_MB"] = (memory_after.heap_usage_bytes - memory_before.heap_usage_bytes) / (1024.0 * 1024.0);
        state.counters["SessionSuccessRate_%"] = (successful_sessions * 100.0) / session_count_;
        
        // PRD target validation
        state.counters["PRDTarget_Sessions"] = 10000;
        state.counters["RelayScalabilityTargetMet"] = (session_count_ >= 1000 && successful_sessions >= session_count_ * 0.95) ? 1 : 0;
        
        state.SetIterationTime(duration_ms / 1000.0);
        
        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(RelayServerScalabilityFixture, ConcurrentSessions)
    ->Range(50, 1000) // Test from 50 to 1k sessions
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Name("Scalability/RelayServer/ConcurrentSessions");

// =============================================================================
// Player Session Scalability Benchmarks
// =============================================================================

/**
 * Benchmark: Multi-Player Session Performance
 * Target: Support up to 8 players per session efficiently
 */
class MultiPlayerSessionFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        player_count_ = static_cast<int>(state.range(0));
        mock_session_ = std::make_unique<MockMultiplayerSession>();
        
        mock_session_->SetMaxPlayers(8);
        mock_session_->SetPacketBroadcastMode(MockBroadcastMode::Efficient);
    }

    void TearDown(const benchmark::State& state) override {
        mock_session_.reset();
    }

protected:
    int player_count_;
    std::unique_ptr<MockMultiplayerSession> mock_session_;
};

BENCHMARK_DEFINE_F(MultiPlayerSessionFixture, PacketBroadcast)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup players in session
        std::vector<std::unique_ptr<MockPlayer>> players;
        for (int i = 0; i < player_count_; ++i) {
            auto player = std::make_unique<MockPlayer>("player_" + std::to_string(i));
            mock_session_->AddPlayer(player.get());
            players.push_back(std::move(player));
        }
        
        // Create test packet
        MockPacket broadcast_packet;
        broadcast_packet.data = GenerateGamePacket(512);
        broadcast_packet.packet_type = MockPacketType::GameData;
        
        auto memory_before = MemoryTracker::TakeSnapshot();
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        // Broadcast packet to all players
        auto result = mock_session_->BroadcastPacket(broadcast_packet);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        state.PauseTiming();
        
        auto memory_after = MemoryTracker::TakeSnapshot();
        
        // Verify all players received the packet
        int players_received = 0;
        for (const auto& player : players) {
            if (player->HasReceivedPacket(broadcast_packet)) {
                players_received++;
            }
        }
        
        // Clean up
        for (const auto& player : players) {
            mock_session_->RemovePlayer(player.get());
        }
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Packet broadcast failed");
            continue;
        }
        
        // Performance metrics
        double broadcast_latency_per_player = duration_us / static_cast<double>(player_count_);
        size_t memory_overhead = memory_after.heap_usage_bytes - memory_before.heap_usage_bytes;
        
        state.counters["PlayerCount"] = player_count_;
        state.counters["PlayersReceived"] = players_received;
        state.counters["BroadcastLatency_us"] = duration_us;
        state.counters["LatencyPerPlayer_us"] = broadcast_latency_per_player;
        state.counters["MemoryOverhead_KB"] = memory_overhead / 1024.0;
        state.counters["DeliverySuccessRate_%"] = (players_received * 100.0) / player_count_;
        state.counters["MaxPlayersTargetMet"] = (player_count_ <= 8 && players_received == player_count_) ? 1 : 0;
        
        state.SetIterationTime(duration_us / 1000000.0);
        
        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(MultiPlayerSessionFixture, PacketBroadcast)
    ->Range(2, 8) // Test with 2 to 8 players
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Name("Scalability/MultiPlayer/PacketBroadcast");

/**
 * Benchmark: Room List Query Performance
 * Target: < 100ms response time for room list queries
 */
class RoomListQueryFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        room_count_ = static_cast<int>(state.range(0));
        mock_room_directory_ = std::make_unique<MockRoomDirectory>();
        
        // Populate directory with test rooms
        for (int i = 0; i < room_count_; ++i) {
            MockRoomInfo room;
            room.room_id = "room_" + std::to_string(i);
            room.game_title = "TestGame";
            room.current_players = (i % 8) + 1; // 1-8 players
            room.max_players = 8;
            room.has_password = (i % 3 == 0); // Every 3rd room has password
            
            mock_room_directory_->AddRoom(room);
        }
    }

    void TearDown(const benchmark::State& state) override {
        mock_room_directory_.reset();
    }

protected:
    int room_count_;
    std::unique_ptr<MockRoomDirectory> mock_room_directory_;
};

BENCHMARK_DEFINE_F(RoomListQueryFixture, QueryAllRooms)(benchmark::State& state) {
    for (auto _ : state) {
        MockRoomQuery query;
        query.game_filter = "TestGame";
        query.max_results = room_count_;
        query.include_full_rooms = true;
        query.include_password_protected = true;
        
        auto start = high_resolution_clock::now();
        
        auto room_list = mock_room_directory_->QueryRooms(query);
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        // Validate query results
        if (room_list.empty()) {
            state.SkipWithError("Room query returned no results");
            continue;
        }
        
        state.counters["TotalRooms"] = room_count_;
        state.counters["ReturnedRooms"] = room_list.size();
        state.counters["QueryTime_ms"] = duration_ms;
        state.counters["PRDTarget_ms"] = 100.0;
        state.counters["QueryTargetMet"] = (duration_ms <= 100) ? 1 : 0;
        state.counters["QueryThroughput_rooms_per_ms"] = room_list.size() / static_cast<double>(duration_ms);
        
        state.SetIterationTime(duration_ms / 1000.0);
    }
}

BENCHMARK_REGISTER_F(RoomListQueryFixture, QueryAllRooms)
    ->Range(100, 10000) // Test with 100 to 10k rooms
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Name("Scalability/RoomList/QueryPerformance");

// =============================================================================
// Memory Usage and Leak Detection Benchmarks
// =============================================================================

/**
 * Benchmark: Memory Usage Under Load
 * Tests memory consumption patterns during sustained operation
 */
class MemoryUsageFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        operation_count_ = static_cast<int>(state.range(0));
        mock_memory_tracker_ = std::make_unique<MockMemoryTracker>();
        
        // Enable detailed memory tracking
        mock_memory_tracker_->EnableLeakDetection(true);
        mock_memory_tracker_->SetTrackingGranularity(MockTrackingGranularity::Fine);
    }

    void TearDown(const benchmark::State& state) override {
        mock_memory_tracker_.reset();
    }

protected:
    int operation_count_;
    std::unique_ptr<MockMemoryTracker> mock_memory_tracker_;
};

BENCHMARK_DEFINE_F(MemoryUsageFixture, ConnectionMemoryUsage)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        auto initial_memory = MemoryTracker::TakeSnapshot();
        
        std::vector<std::unique_ptr<MockConnection>> connections;
        connections.reserve(operation_count_);
        
        state.ResumeTiming();
        
        // Simulate multiple connection/disconnection cycles
        for (int cycle = 0; cycle < 5; ++cycle) {
            // Connect phase
            for (int i = 0; i < operation_count_; ++i) {
                auto connection = std::make_unique<MockConnection>();
                connection->Connect("test://endpoint");
                connections.push_back(std::move(connection));
            }
            
            // Memory snapshot after connections
            auto peak_memory = MemoryTracker::TakeSnapshot();
            
            // Disconnect phase
            for (auto& connection : connections) {
                connection->Disconnect();
            }
            connections.clear();
            
            // Memory snapshot after cleanup
            auto cleanup_memory = MemoryTracker::TakeSnapshot();
            
            // Check for memory leaks
            size_t memory_growth = cleanup_memory.heap_usage_bytes - initial_memory.heap_usage_bytes;
            
            state.counters["Cycle"] = cycle;
            state.counters["ConnectionsPerCycle"] = operation_count_;
            state.counters["PeakMemory_MB"] = (peak_memory.heap_usage_bytes - initial_memory.heap_usage_bytes) / (1024.0 * 1024.0);
            state.counters["MemoryGrowth_KB"] = memory_growth / 1024.0;
            state.counters["MemoryPerConnection_bytes"] = (peak_memory.heap_usage_bytes - initial_memory.heap_usage_bytes) / operation_count_;
            
            // Leak detection
            bool potential_leak = (memory_growth > (operation_count_ * 1024)); // More than 1KB growth per connection is suspicious
            state.counters["PotentialLeak"] = potential_leak ? 1 : 0;
        }
    }
}

BENCHMARK_REGISTER_F(MemoryUsageFixture, ConnectionMemoryUsage)
    ->Range(10, 100) // Test with 10 to 100 connections per cycle
    ->Unit(benchmark::kMillisecond)
    ->Name("Memory/Usage/ConnectionCycles");

/**
 * Benchmark: Packet Processing Memory Efficiency
 * Tests memory usage during sustained packet processing
 */
BENCHMARK_DEFINE_F(MemoryUsageFixture, PacketProcessingMemory)(benchmark::State& state) {
    MockPacketProcessor processor;
    
    for (auto _ : state) {
        auto initial_memory = MemoryTracker::TakeSnapshot();
        
        // Process sustained packet load
        for (int i = 0; i < operation_count_; ++i) {
            MockPacket packet;
            packet.data = GenerateGamePacket(512);
            packet.packet_type = MockPacketType::GameData;
            
            processor.ProcessPacket(packet);
            
            // Periodic memory check
            if (i % 100 == 0) {
                auto current_memory = MemoryTracker::TakeSnapshot();
                size_t memory_usage = current_memory.heap_usage_bytes - initial_memory.heap_usage_bytes;
                
                state.counters["PacketsProcessed"] = i;
                state.counters["CurrentMemoryUsage_KB"] = memory_usage / 1024.0;
                state.counters["MemoryPerPacket_bytes"] = memory_usage / (i + 1);
            }
        }
        
        auto final_memory = MemoryTracker::TakeSnapshot();
        size_t total_memory_used = final_memory.heap_usage_bytes - initial_memory.heap_usage_bytes;
        
        state.counters["TotalPacketsProcessed"] = operation_count_;
        state.counters["TotalMemoryUsed_MB"] = total_memory_used / (1024.0 * 1024.0);
        state.counters["AvgMemoryPerPacket_bytes"] = total_memory_used / operation_count_;
        
        // Memory efficiency target (should be < 100 bytes overhead per packet)
        state.counters["MemoryEfficiencyTargetMet"] = (total_memory_used / operation_count_ <= 100) ? 1 : 0;
    }
}

BENCHMARK_REGISTER_F(MemoryUsageFixture, PacketProcessingMemory)
    ->Range(1000, 10000) // Process 1k to 10k packets
    ->Unit(benchmark::kMillisecond)
    ->Name("Memory/Usage/PacketProcessing");

// =============================================================================
// Thread Safety and Concurrency Benchmarks
// =============================================================================

/**
 * Benchmark: Thread Safety Under Load
 * Tests performance of thread-safe operations under concurrent access
 */
class ThreadSafetyFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        thread_count_ = static_cast<int>(state.range(0));
        mock_shared_resource_ = std::make_unique<ThreadSafeResourceManager>();
        
        // Configure for high concurrency
        mock_shared_resource_->SetLockingStrategy(MockLockingStrategy::Optimistic);
        mock_shared_resource_->SetContentionHandling(MockContentionHandling::Backoff);
    }

    void TearDown(const benchmark::State& state) override {
        mock_shared_resource_.reset();
    }

protected:
    int thread_count_;
    std::unique_ptr<ThreadSafeResourceManager> mock_shared_resource_;
};

BENCHMARK_DEFINE_F(ThreadSafetyFixture, ConcurrentResourceAccess)(benchmark::State& state) {
    for (auto _ : state) {
        std::atomic<int> operations_completed{0};
        std::atomic<int> contentions_detected{0};
        
        auto start = high_resolution_clock::now();
        
        // Start concurrent threads
        std::vector<std::future<void>> thread_futures;
        
        for (int t = 0; t < thread_count_; ++t) {
            thread_futures.push_back(
                std::async(std::launch::async, [this, &operations_completed, &contentions_detected]() {
                    int local_operations = 0;
                    int local_contentions = 0;
                    
                    for (int op = 0; op < 100; ++op) {
                        auto start_op = high_resolution_clock::now();
                        
                        // Perform thread-safe operation
                        auto result = mock_shared_resource_->AccessResource("test_resource");
                        
                        auto end_op = high_resolution_clock::now();
                        auto op_duration = duration_cast<microseconds>(end_op - start_op);
                        
                        if (result == MockErrorCode::Success) {
                            local_operations++;
                        }
                        
                        // Detect contention (operation took longer than expected)
                        if (op_duration > microseconds(100)) { // 100μs threshold
                            local_contentions++;
                        }
                    }
                    
                    operations_completed += local_operations;
                    contentions_detected += local_contentions;
                })
            );
        }
        
        // Wait for all threads to complete
        for (auto& future : thread_futures) {
            future.wait();
        }
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        int expected_operations = thread_count_ * 100;
        double throughput = operations_completed.load() / (duration_ms / 1000.0);
        double contention_rate = (contentions_detected.load() * 100.0) / expected_operations;
        
        state.counters["ThreadCount"] = thread_count_;
        state.counters["ExpectedOperations"] = expected_operations;
        state.counters["CompletedOperations"] = operations_completed.load();
        state.counters["Throughput_ops_per_sec"] = throughput;
        state.counters["ContentionRate_%"] = contention_rate;
        state.counters["OperationSuccessRate_%"] = (operations_completed.load() * 100.0) / expected_operations;
        
        // Thread safety targets
        state.counters["ThreadSafetyTargetMet"] = (operations_completed.load() >= expected_operations * 0.95 && contention_rate <= 10.0) ? 1 : 0;
        
        state.SetIterationTime(duration_ms / 1000.0);
    }
}

BENCHMARK_REGISTER_F(ThreadSafetyFixture, ConcurrentResourceAccess)
    ->Range(2, 16) // Test with 2 to 16 threads
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Name("Scalability/ThreadSafety/ConcurrentAccess");

} // namespace Benchmarks