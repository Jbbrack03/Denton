// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <atomic>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "benchmark_mocks.h"
#include "benchmark_utilities.h"

namespace Benchmarks {

using namespace std::chrono;

/**
 * PRD Requirements for Latency:
 * - Local Ad-hoc (Model B): < 5ms additional overhead
 * - P2P Direct (Model A): < 20ms additional overhead  
 * - Relay Server (Model A): < 50ms additional overhead
 * - Maximum jitter: < 10ms for stable connections
 */

// =============================================================================
// Round-Trip Time (RTT) Latency Benchmarks
// =============================================================================

/**
 * Benchmark: Local Ad-hoc Network Latency
 * Target: < 5ms additional overhead
 */
class AdHocLatencyFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_adhoc_network_ = std::make_unique<MockAdHocNetwork>();
        
        // Configure minimal latency for local network
        mock_adhoc_network_->SetBaseLatency(1ms); // Hardware/OS overhead
        mock_adhoc_network_->SetProcessingOverhead(2ms); // Our processing overhead
        mock_adhoc_network_->SetJitter(0.5ms); // Minimal jitter
    }

    void TearDown(const benchmark::State& state) override {
        mock_adhoc_network_.reset();
    }

protected:
    std::unique_ptr<MockAdHocNetwork> mock_adhoc_network_;
    std::vector<double> latency_samples_;
};

BENCHMARK_DEFINE_F(AdHocLatencyFixture, RoundTripTime)(benchmark::State& state) {
    latency_samples_.clear();
    latency_samples_.reserve(1000);
    
    auto network_client = CreateMockAdHocClient(mock_adhoc_network_.get());
    
    for (auto _ : state) {
        // Create test packet
        MockPacket test_packet;
        test_packet.data = GenerateTestData(64); // Small test packet
        test_packet.timestamp = high_resolution_clock::now();
        
        auto start = high_resolution_clock::now();
        
        // Send packet and wait for echo response
        auto result = network_client->SendPacketWithEcho(test_packet);
        
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Packet echo failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double latency_ms = duration.count() / 1000.0;
        
        latency_samples_.push_back(latency_ms);
        
        // Record individual measurement
        state.SetIterationTime(latency_ms / 1000.0); // Convert to seconds for benchmark
    }
    
    // Statistical analysis
    if (!latency_samples_.empty()) {
        double avg_latency = std::accumulate(latency_samples_.begin(), latency_samples_.end(), 0.0) / latency_samples_.size();
        
        std::sort(latency_samples_.begin(), latency_samples_.end());
        double p95_latency = latency_samples_[static_cast<size_t>(latency_samples_.size() * 0.95)];
        double p99_latency = latency_samples_[static_cast<size_t>(latency_samples_.size() * 0.99)];
        
        // Custom counters for detailed analysis
        state.counters["AvgLatency_ms"] = avg_latency;
        state.counters["P95Latency_ms"] = p95_latency;
        state.counters["P99Latency_ms"] = p99_latency;
        state.counters["PRDTarget_ms"] = 5.0; // PRD target
        state.counters["TargetMet"] = (p95_latency <= 5.0) ? 1 : 0;
        
        // Calculate jitter (standard deviation)
        double variance = 0.0;
        for (double sample : latency_samples_) {
            variance += std::pow(sample - avg_latency, 2);
        }
        double jitter = std::sqrt(variance / latency_samples_.size());
        state.counters["Jitter_ms"] = jitter;
        state.counters["JitterTargetMet"] = (jitter <= 10.0) ? 1 : 0;
    }
}

BENCHMARK_REGISTER_F(AdHocLatencyFixture, RoundTripTime)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1000)
    ->UseManualTime()
    ->Name("Latency/AdHoc/RoundTripTime");

/**
 * Benchmark: P2P Direct Connection Latency  
 * Target: < 20ms additional overhead
 */
class P2PLatencyFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_p2p_network_ = std::make_unique<MockP2PNetwork>();
        
        // Configure P2P with internet routing overhead
        mock_p2p_network_->SetBaseLatency(15ms); // Internet routing
        mock_p2p_network_->SetProcessingOverhead(3ms); // Our processing
        mock_p2p_network_->SetJitter(2ms);
    }

    void TearDown(const benchmark::State& state) override {
        mock_p2p_network_.reset();
    }

protected:
    std::unique_ptr<MockP2PNetwork> mock_p2p_network_;
    std::vector<double> latency_samples_;
};

BENCHMARK_DEFINE_F(P2PLatencyFixture, DirectConnectionRTT)(benchmark::State& state) {
    latency_samples_.clear();
    latency_samples_.reserve(1000);
    
    auto p2p_client = CreateMockP2PClient(mock_p2p_network_.get());
    
    for (auto _ : state) {
        MockPacket test_packet;
        test_packet.data = GenerateTestData(128); 
        test_packet.timestamp = high_resolution_clock::now();
        
        auto start = high_resolution_clock::now();
        
        auto result = p2p_client->SendPacketWithEcho(test_packet);
        
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("P2P packet echo failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double latency_ms = duration.count() / 1000.0;
        
        latency_samples_.push_back(latency_ms);
        state.SetIterationTime(latency_ms / 1000.0);
    }
    
    // Statistical analysis for P2P
    if (!latency_samples_.empty()) {
        double avg_latency = std::accumulate(latency_samples_.begin(), latency_samples_.end(), 0.0) / latency_samples_.size();
        
        std::sort(latency_samples_.begin(), latency_samples_.end());
        double p95_latency = latency_samples_[static_cast<size_t>(latency_samples_.size() * 0.95)];
        
        state.counters["AvgLatency_ms"] = avg_latency;
        state.counters["P95Latency_ms"] = p95_latency;
        state.counters["PRDTarget_ms"] = 20.0;
        state.counters["TargetMet"] = (p95_latency <= 20.0) ? 1 : 0;
        
        // Measure our processing overhead (subtract base network latency)
        double processing_overhead = avg_latency - 15.0; // Subtract base internet latency
        state.counters["ProcessingOverhead_ms"] = processing_overhead;
    }
}

BENCHMARK_REGISTER_F(P2PLatencyFixture, DirectConnectionRTT)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1000)
    ->UseManualTime()
    ->Name("Latency/P2P/DirectConnection");

/**
 * Benchmark: Relay Server Latency
 * Target: < 50ms additional overhead
 */
class RelayLatencyFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_relay_client_ = std::make_unique<MockRelayClient>();
        
        // Configure relay with server routing overhead
        mock_relay_client_->SetBaseLatency(25ms); // Internet + server routing
        mock_relay_client_->SetProcessingOverhead(5ms); // Relay processing
        mock_relay_client_->SetJitter(3ms);
    }

    void TearDown(const benchmark::State& state) override {
        mock_relay_client_.reset();
    }

protected:
    std::unique_ptr<MockRelayClient> mock_relay_client_;
    std::vector<double> latency_samples_;
};

BENCHMARK_DEFINE_F(RelayLatencyFixture, RelayServerRTT)(benchmark::State& state) {
    latency_samples_.clear();
    latency_samples_.reserve(1000);
    
    auto relay_client = CreateMockRelayClientConnection(mock_relay_client_.get());
    
    for (auto _ : state) {
        MockPacket test_packet;
        test_packet.data = GenerateTestData(256);
        test_packet.timestamp = high_resolution_clock::now();
        
        auto start = high_resolution_clock::now();
        
        auto result = relay_client->SendPacketWithEcho(test_packet);
        
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Relay packet echo failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double latency_ms = duration.count() / 1000.0;
        
        latency_samples_.push_back(latency_ms);
        state.SetIterationTime(latency_ms / 1000.0);
    }
    
    // Statistical analysis for relay
    if (!latency_samples_.empty()) {
        double avg_latency = std::accumulate(latency_samples_.begin(), latency_samples_.end(), 0.0) / latency_samples_.size();
        
        std::sort(latency_samples_.begin(), latency_samples_.end());
        double p95_latency = latency_samples_[static_cast<size_t>(latency_samples_.size() * 0.95)];
        
        state.counters["AvgLatency_ms"] = avg_latency;
        state.counters["P95Latency_ms"] = p95_latency;
        state.counters["PRDTarget_ms"] = 50.0;
        state.counters["TargetMet"] = (p95_latency <= 50.0) ? 1 : 0;
        
        // Measure relay overhead
        double relay_overhead = avg_latency - 25.0; // Subtract base network latency
        state.counters["RelayOverhead_ms"] = relay_overhead;
    }
}

BENCHMARK_REGISTER_F(RelayLatencyFixture, RelayServerRTT)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1000)
    ->UseManualTime()
    ->Name("Latency/Relay/ServerRouting");

// =============================================================================
// Packet Processing Latency Benchmarks
// =============================================================================

/**
 * Benchmark: Packet Processing Pipeline Latency
 * Measures time from packet receive to game delivery
 */
class PacketProcessingFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_packet_processor_ = std::make_unique<MockPacketProcessor>();
        mock_hle_interface_ = std::make_unique<MockHLEInterface>();
        
        // Configure processing pipeline
        mock_packet_processor_->SetProcessingTime(1ms); // Packet validation/parsing
        mock_hle_interface_->SetDeliveryTime(0.5ms); // HLE delivery
    }

    void TearDown(const benchmark::State& state) override {
        mock_packet_processor_.reset();
        mock_hle_interface_.reset();
    }

protected:
    std::unique_ptr<MockPacketProcessor> mock_packet_processor_;
    std::unique_ptr<MockHLEInterface> mock_hle_interface_;
};

BENCHMARK_DEFINE_F(PacketProcessingFixture, ReceiveToDelivery)(benchmark::State& state) {
    auto packet_pipeline = CreateMockPacketPipeline(
        mock_packet_processor_.get(), mock_hle_interface_.get());
    
    for (auto _ : state) {
        // Create realistic game packet
        MockPacket game_packet;
        game_packet.data = GenerateGamePacket(512); // Typical game packet size
        game_packet.packet_type = MockPacketType::GameData;
        
        auto start = high_resolution_clock::now();
        
        // Process packet through full pipeline
        auto result = packet_pipeline->ProcessIncomingPacket(game_packet);
        
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Packet processing failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double processing_time_ms = duration.count() / 1000.0;
        
        state.SetIterationTime(processing_time_ms / 1000.0);
    }
}

BENCHMARK_REGISTER_F(PacketProcessingFixture, ReceiveToDelivery)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000)
    ->UseManualTime()
    ->Name("Latency/PacketProcessing/ReceiveToDelivery");

/**
 * Benchmark: 60Hz Packet Processing Capability
 * Target: Process packets at 60Hz (16.67ms budget per frame)
 */
BENCHMARK(Latency_60HzPacketProcessing) {
    MockPacketProcessor processor;
    MockHLEInterface hle;
    
    auto packet_pipeline = CreateMockPacketPipeline(&processor, &hle);
    
    // Simulate 60Hz packet rate (16.67ms between packets)
    const auto frame_budget = microseconds(16667); // 16.67ms
    
    for (auto _ : state) {
        // Process multiple packets within frame budget
        std::vector<MockPacket> frame_packets;
        
        // Generate typical frame packet load (4-8 packets per frame)
        int packet_count = 6;
        for (int i = 0; i < packet_count; ++i) {
            MockPacket packet;
            packet.data = GenerateGamePacket(256);
            packet.packet_type = MockPacketType::GameData;
            frame_packets.push_back(packet);
        }
        
        auto start = high_resolution_clock::now();
        
        // Process all packets in frame
        for (const auto& packet : frame_packets) {
            auto result = packet_pipeline->ProcessIncomingPacket(packet);
            if (result != MockErrorCode::Success) {
                state.SkipWithError("Packet processing failed during 60Hz test");
                break;
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        // Check if we met the 60Hz budget
        bool met_budget = (duration <= frame_budget);
        
        state.SetIterationTime(duration.count() / 1000000.0); // Convert to seconds
        state.counters["FrameBudget_us"] = frame_budget.count();
        state.counters["ActualTime_us"] = duration.count();
        state.counters["PacketsProcessed"] = packet_count;
        state.counters["BudgetMet"] = met_budget ? 1 : 0;
        state.counters["UtilizationPercent"] = (duration.count() * 100.0) / frame_budget.count();
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(Latency_60HzPacketProcessing)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000)
    ->UseManualTime()
    ->Name("Latency/PacketProcessing/60HzCapability");

// =============================================================================
// Jitter Analysis Benchmarks
// =============================================================================

/**
 * Benchmark: Network Jitter Measurement
 * Target: < 10ms maximum jitter for stable connections
 */
class JitterAnalysisFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        // Test different jitter levels
        jitter_levels_ = {1.0, 5.0, 10.0, 15.0, 20.0}; // ms
        current_jitter_index_ = 0;
    }

protected:
    std::vector<double> jitter_levels_;
    size_t current_jitter_index_;
};

BENCHMARK_DEFINE_F(JitterAnalysisFixture, NetworkStability)(benchmark::State& state) {
    // Test with increasing jitter levels
    double jitter_level = jitter_levels_[current_jitter_index_ % jitter_levels_.size()];
    
    MockConnection connection;
    connection.SetBaseLatency(20ms);
    connection.SetJitter(static_cast<std::chrono::milliseconds>(static_cast<int>(jitter_level)));
    
    std::vector<double> latency_samples;
    latency_samples.reserve(100);
    
    for (auto _ : state) {
        MockPacket packet;
        packet.data = GenerateTestData(128);
        
        auto start = high_resolution_clock::now();
        auto result = connection.SendPacketWithEcho(packet);
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Jitter test packet failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double latency_ms = duration.count() / 1000.0;
        latency_samples.push_back(latency_ms);
    }
    
    // Calculate jitter statistics
    if (latency_samples.size() >= 10) {
        double avg_latency = std::accumulate(latency_samples.begin(), latency_samples.end(), 0.0) / latency_samples.size();
        
        // Calculate standard deviation (jitter)
        double variance = 0.0;
        for (double sample : latency_samples) {
            variance += std::pow(sample - avg_latency, 2);
        }
        double measured_jitter = std::sqrt(variance / latency_samples.size());
        
        // Calculate min/max spread
        auto minmax = std::minmax_element(latency_samples.begin(), latency_samples.end());
        double jitter_spread = *minmax.second - *minmax.first;
        
        state.counters["ConfiguredJitter_ms"] = jitter_level;
        state.counters["MeasuredJitter_ms"] = measured_jitter;
        state.counters["JitterSpread_ms"] = jitter_spread;
        state.counters["AvgLatency_ms"] = avg_latency;
        state.counters["JitterTargetMet"] = (measured_jitter <= 10.0) ? 1 : 0;
    }
    
    current_jitter_index_++;
}

BENCHMARK_REGISTER_F(JitterAnalysisFixture, NetworkStability)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100)
    ->Repetitions(5) // Test each jitter level multiple times
    ->Name("Latency/Jitter/NetworkStability");

// =============================================================================
// Latency Under Load Benchmarks
// =============================================================================

/**
 * Benchmark: Latency with Concurrent Players
 * Test latency degradation with increasing player count
 */
BENCHMARK(Latency_ConcurrentPlayers) {
    int player_count = state.range(0);
    
    MockMultiplayerSession session;
    session.SetPlayerCount(player_count);
    
    std::vector<double> latency_samples;
    
    for (auto _ : state) {
        MockPacket packet;
        packet.data = GenerateGamePacket(256);
        
        auto start = high_resolution_clock::now();
        
        // Simulate packet being processed in session with multiple players
        auto result = session.BroadcastPacket(packet);
        
        auto end = high_resolution_clock::now();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Broadcast failed");
            continue;
        }
        
        auto duration = duration_cast<microseconds>(end - start);
        double latency_ms = duration.count() / 1000.0;
        latency_samples.push_back(latency_ms);
    }
    
    // Analysis
    if (!latency_samples.empty()) {
        double avg_latency = std::accumulate(latency_samples.begin(), latency_samples.end(), 0.0) / latency_samples.size();
        state.counters["PlayerCount"] = player_count;
        state.counters["AvgLatency_ms"] = avg_latency;
        state.counters["LatencyPerPlayer_ms"] = avg_latency / player_count;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(Latency_ConcurrentPlayers)
    ->Range(2, 8) // Test with 2 to 8 players
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100)
    ->Name("Latency/Load/ConcurrentPlayers");

} // namespace Benchmarks
