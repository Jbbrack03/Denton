// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <numeric>

#include "benchmark_mocks.h"
#include "benchmark_utilities.h"

namespace Benchmarks {

using namespace std::chrono;

/**
 * PRD Requirements for Packet Processing:
 * - Packet frequency: Up to 60 Hz for real-time games
 * - Maximum packet size: 1400 bytes (MTU safe)
 * - Minimum bandwidth: 256 Kbps per player
 * - Recommended bandwidth: 1 Mbps per player
 * - Target: 16.67ms per frame processing budget (60Hz)
 */

// =============================================================================
// Packet Protocol Processing Benchmarks
// =============================================================================

/**
 * Benchmark: Binary Packet Serialization Performance
 * Tests the speed of packet encoding/decoding
 */
class PacketSerializationFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_packet_protocol_ = std::make_unique<MockPacketProtocol>();
        
        // Pre-generate test data
        small_packet_ = GenerateGamePacket(64);   // Small packet
        medium_packet_ = GenerateGamePacket(512); // Medium packet
        large_packet_ = GenerateGamePacket(1400); // MTU-size packet
    }

    void TearDown(const benchmark::State& state) override {
        mock_packet_protocol_.reset();
    }

protected:
    std::unique_ptr<MockPacketProtocol> mock_packet_protocol_;
    std::vector<uint8_t> small_packet_;
    std::vector<uint8_t> medium_packet_;
    std::vector<uint8_t> large_packet_;
};

BENCHMARK_DEFINE_F(PacketSerializationFixture, SmallPacketSerialization)(benchmark::State& state) {
    MockLdnPacket packet;
    packet.header.magic = 0x4C44;  // "LD"
    packet.header.version = 1;
    packet.header.session_id = 12345;
    packet.header.packet_type = 1; // GameData
    packet.payload = small_packet_;
    
    for (auto _ : state) {
        // Serialize packet
        auto serialized = mock_packet_protocol_->SerializePacket(packet);
        
        // Deserialize packet
        auto deserialized = mock_packet_protocol_->DeserializePacket(serialized);
        
        // Verify integrity (in real benchmark, this would be minimal)
        if (deserialized.header.magic != 0x4C44) {
            state.SkipWithError("Packet serialization corrupted");
        }
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * small_packet_.size() * 2); // *2 for serialize + deserialize
}

BENCHMARK_REGISTER_F(PacketSerializationFixture, SmallPacketSerialization)
    ->Name("PacketProcessing/Serialization/SmallPacket_64B");

BENCHMARK_DEFINE_F(PacketSerializationFixture, MediumPacketSerialization)(benchmark::State& state) {
    MockLdnPacket packet;
    packet.header.magic = 0x4C44;
    packet.header.version = 1;
    packet.header.session_id = 12345;
    packet.header.packet_type = 1;
    packet.payload = medium_packet_;
    
    for (auto _ : state) {
        auto serialized = mock_packet_protocol_->SerializePacket(packet);
        auto deserialized = mock_packet_protocol_->DeserializePacket(serialized);
        
        if (deserialized.header.magic != 0x4C44) {
            state.SkipWithError("Packet serialization corrupted");
        }
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * medium_packet_.size() * 2);
}

BENCHMARK_REGISTER_F(PacketSerializationFixture, MediumPacketSerialization)
    ->Name("PacketProcessing/Serialization/MediumPacket_512B");

BENCHMARK_DEFINE_F(PacketSerializationFixture, LargePacketSerialization)(benchmark::State& state) {
    MockLdnPacket packet;
    packet.header.magic = 0x4C44;
    packet.header.version = 1;
    packet.header.session_id = 12345;
    packet.header.packet_type = 1;
    packet.payload = large_packet_;
    
    for (auto _ : state) {
        auto serialized = mock_packet_protocol_->SerializePacket(packet);
        auto deserialized = mock_packet_protocol_->DeserializePacket(serialized);
        
        if (deserialized.header.magic != 0x4C44) {
            state.SkipWithError("Packet serialization corrupted");
        }
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * large_packet_.size() * 2);
}

BENCHMARK_REGISTER_F(PacketSerializationFixture, LargePacketSerialization)
    ->Name("PacketProcessing/Serialization/LargePacket_1400B");

// =============================================================================
// CRC32 Validation Performance Benchmarks
// =============================================================================

/**
 * Benchmark: CRC32 Checksum Performance
 * Tests the speed of CRC32 calculation for packet validation
 */
class CRC32Fixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_crc_calculator_ = std::make_unique<MockCRC32Calculator>();
        
        // Pre-generate test data of various sizes
        test_data_64_ = GenerateRandomData(64);
        test_data_512_ = GenerateRandomData(512);
        test_data_1400_ = GenerateRandomData(1400);
        test_data_32kb_ = GenerateRandomData(32768); // Large packet for stress test
    }

    void TearDown(const benchmark::State& state) override {
        mock_crc_calculator_.reset();
    }

protected:
    std::unique_ptr<MockCRC32Calculator> mock_crc_calculator_;
    std::vector<uint8_t> test_data_64_;
    std::vector<uint8_t> test_data_512_;
    std::vector<uint8_t> test_data_1400_;
    std::vector<uint8_t> test_data_32kb_;
};

BENCHMARK_DEFINE_F(CRC32Fixture, CRC32_SmallPacket)(benchmark::State& state) {
    for (auto _ : state) {
        uint32_t crc = mock_crc_calculator_->CalculateCRC32(test_data_64_);
        benchmark::DoNotOptimize(crc);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * test_data_64_.size());
}

BENCHMARK_REGISTER_F(CRC32Fixture, CRC32_SmallPacket)
    ->Name("PacketProcessing/CRC32/SmallPacket_64B");

BENCHMARK_DEFINE_F(CRC32Fixture, CRC32_MediumPacket)(benchmark::State& state) {
    for (auto _ : state) {
        uint32_t crc = mock_crc_calculator_->CalculateCRC32(test_data_512_);
        benchmark::DoNotOptimize(crc);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * test_data_512_.size());
}

BENCHMARK_REGISTER_F(CRC32Fixture, CRC32_MediumPacket)
    ->Name("PacketProcessing/CRC32/MediumPacket_512B");

BENCHMARK_DEFINE_F(CRC32Fixture, CRC32_LargePacket)(benchmark::State& state) {
    for (auto _ : state) {
        uint32_t crc = mock_crc_calculator_->CalculateCRC32(test_data_1400_);
        benchmark::DoNotOptimize(crc);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * test_data_1400_.size());
}

BENCHMARK_REGISTER_F(CRC32Fixture, CRC32_LargePacket)
    ->Name("PacketProcessing/CRC32/LargePacket_1400B");

/**
 * Benchmark: CRC32 Performance Target Validation
 * PRD requirement: >100MB/s CRC32 throughput
 */
BENCHMARK_DEFINE_F(CRC32Fixture, CRC32_ThroughputTest)(benchmark::State& state) {
    int64_t total_bytes = 0;
    
    for (auto _ : state) {
        // Process large chunk to measure sustained throughput
        uint32_t crc = mock_crc_calculator_->CalculateCRC32(test_data_32kb_);
        benchmark::DoNotOptimize(crc);
        total_bytes += test_data_32kb_.size();
    }
    
    state.SetBytesProcessed(total_bytes);
    
    // Calculate throughput in MB/s
    double duration_s = state.iterations() * (state.range(0) / 1000000.0); // Convert to seconds
    double throughput_mbs = (total_bytes / (1024.0 * 1024.0)) / duration_s;
    
    state.counters["Throughput_MBps"] = throughput_mbs;
    state.counters["PRDTarget_MBps"] = 100.0;
    state.counters["TargetMet"] = (throughput_mbs >= 100.0) ? 1 : 0;
}

BENCHMARK_REGISTER_F(CRC32Fixture, CRC32_ThroughputTest)
    ->Name("PacketProcessing/CRC32/ThroughputValidation");

// =============================================================================
// 60Hz Processing Capability Benchmarks
// =============================================================================

/**
 * Benchmark: 60Hz Frame Processing
 * Tests ability to process all packets within 16.67ms frame budget
 */
class SixtyHzProcessingFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_packet_queue_ = std::make_unique<MockPacketQueue>();
        mock_hle_interface_ = std::make_unique<MockHLEInterface>();
        
        // Configure frame processing parameters
        frame_budget_us_ = 16667; // 16.67ms in microseconds
        max_packets_per_frame_ = 8; // Realistic game packet count
    }

    void TearDown(const benchmark::State& state) override {
        mock_packet_queue_.reset();
        mock_hle_interface_.reset();
    }

protected:
    std::unique_ptr<MockPacketQueue> mock_packet_queue_;
    std::unique_ptr<MockHLEInterface> mock_hle_interface_;
    int frame_budget_us_;
    int max_packets_per_frame_;
};

BENCHMARK_DEFINE_F(SixtyHzProcessingFixture, FrameProcessing)(benchmark::State& state) {
    int packet_count = static_cast<int>(state.range(0));
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Queue packets for frame processing
        std::vector<MockPacket> frame_packets;
        for (int i = 0; i < packet_count; ++i) {
            MockPacket packet;
            packet.data = GenerateGamePacket(256); // Typical game packet size
            packet.packet_type = MockPacketType::GameData;
            packet.timestamp = high_resolution_clock::now();
            frame_packets.push_back(packet);
        }
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        // Process all packets in frame
        int processed_count = 0;
        for (const auto& packet : frame_packets) {
            auto result = mock_hle_interface_->DeliverPacket(packet);
            if (result == MockErrorCode::Success) {
                processed_count++;
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        state.PauseTiming();
        
        // Validate frame budget compliance
        bool met_budget = (duration_us <= frame_budget_us_);
        
        state.counters["PacketCount"] = packet_count;
        state.counters["ProcessedCount"] = processed_count;
        state.counters["FrameBudget_us"] = frame_budget_us_;
        state.counters["ActualTime_us"] = duration_us;
        state.counters["BudgetMet"] = met_budget ? 1 : 0;
        state.counters["UtilizationPercent"] = (duration_us * 100.0) / frame_budget_us_;
        
        state.SetIterationTime(duration_us / 1000000.0); // Convert to seconds
        
        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(SixtyHzProcessingFixture, FrameProcessing)
    ->Range(1, 8) // Test with 1 to 8 packets per frame
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Name("PacketProcessing/60Hz/FrameProcessing");

/**
 * Benchmark: Sustained 60Hz Processing
 * Tests sustained performance over multiple frames
 */
BENCHMARK_DEFINE_F(SixtyHzProcessingFixture, SustainedProcessing)(benchmark::State& state) {
    const int frames_to_test = 60; // 1 second at 60Hz
    const int packets_per_frame = 6; // Typical load
    
    std::vector<double> frame_times;
    frame_times.reserve(frames_to_test);
    
    for (auto _ : state) {
        // Process multiple frames consecutively
        for (int frame = 0; frame < frames_to_test; ++frame) {
            // Generate frame packets
            std::vector<MockPacket> frame_packets;
            for (int p = 0; p < packets_per_frame; ++p) {
                MockPacket packet;
                packet.data = GenerateGamePacket(256);
                packet.packet_type = MockPacketType::GameData;
                frame_packets.push_back(packet);
            }
            
            auto start = high_resolution_clock::now();
            
            // Process frame
            for (const auto& packet : frame_packets) {
                mock_hle_interface_->DeliverPacket(packet);
            }
            
            auto end = high_resolution_clock::now();
            auto duration_us = duration_cast<microseconds>(end - start).count();
            frame_times.push_back(duration_us);
        }
    }
    
    // Analyze sustained performance
    if (!frame_times.empty()) {
        double avg_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0) / frame_times.size();
        double max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());
        
        int frames_within_budget = std::count_if(frame_times.begin(), frame_times.end(),
            [this](double time) { return time <= frame_budget_us_; });
        
        double budget_compliance = (frames_within_budget * 100.0) / frame_times.size();
        
        state.counters["AvgFrameTime_us"] = avg_frame_time;
        state.counters["MaxFrameTime_us"] = max_frame_time;
        state.counters["BudgetCompliance_%"] = budget_compliance;
        state.counters["FramesTested"] = frames_to_test;
        state.counters["PacketsPerFrame"] = packets_per_frame;
        state.counters["SustainedTargetMet"] = (budget_compliance >= 95.0) ? 1 : 0; // 95% compliance target
    }
}

BENCHMARK_REGISTER_F(SixtyHzProcessingFixture, SustainedProcessing)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10)
    ->Name("PacketProcessing/60Hz/SustainedPerformance");

// =============================================================================
// Throughput and Bandwidth Benchmarks
// =============================================================================

/**
 * Benchmark: Packet Throughput at Different Rates
 * Tests packet processing at various frequencies
 */
BENCHMARK(PacketProcessing_ThroughputTest) {
    int packets_per_second = static_cast<int>(state.range(0));
    MockPacketProcessor processor;
    
    // Calculate per-packet time budget
    int time_budget_us = 1000000 / packets_per_second; // 1 second / packets per second
    
    for (auto _ : state) {
        MockPacket packet;
        packet.data = GenerateGamePacket(512);
        packet.packet_type = MockPacketType::GameData;
        
        auto start = high_resolution_clock::now();
        
        auto result = processor.ProcessPacket(packet);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Packet processing failed");
            continue;
        }
        
        bool met_budget = (duration_us <= time_budget_us);
        
        state.counters["PacketsPerSecond"] = packets_per_second;
        state.counters["TimeBudget_us"] = time_budget_us;
        state.counters["ActualTime_us"] = duration_us;
        state.counters["BudgetMet"] = met_budget ? 1 : 0;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(PacketProcessing_ThroughputTest)
    ->Range(30, 120) // 30Hz to 120Hz
    ->Unit(benchmark::kMicrosecond)
    ->Name("PacketProcessing/Throughput/VariableRates");

/**
 * Benchmark: Bandwidth Utilization Test
 * Tests sustained data throughput at target bandwidth levels
 */
BENCHMARK(PacketProcessing_BandwidthTest) {
    int target_kbps = static_cast<int>(state.range(0));
    MockNetworkInterface network;
    
    // Calculate bytes per second and packet size to achieve target bandwidth
    int bytes_per_second = target_kbps * 1024 / 8; // Convert Kbps to bytes/second
    int packet_size = 512; // Typical packet size
    int packets_per_second = bytes_per_second / packet_size;
    
    std::vector<MockPacket> test_packets;
    test_packets.reserve(packets_per_second);
    
    for (int i = 0; i < packets_per_second; ++i) {
        MockPacket packet;
        packet.data = GenerateGamePacket(packet_size);
        test_packets.push_back(packet);
    }
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        // Send all packets for 1 second worth of data
        int64_t bytes_sent = 0;
        for (const auto& packet : test_packets) {
            auto result = network.SendPacket(packet);
            if (result == MockErrorCode::Success) {
                bytes_sent += packet.data.size();
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration_s = duration_cast<milliseconds>(end - start).count() / 1000.0;
        
        // Calculate achieved bandwidth
        double achieved_kbps = (bytes_sent * 8) / (duration_s * 1024);
        
        state.counters["TargetBandwidth_Kbps"] = target_kbps;
        state.counters["AchievedBandwidth_Kbps"] = achieved_kbps;
        state.counters["BytesSent"] = bytes_sent;
        state.counters["PacketsSent"] = test_packets.size();
        state.counters["BandwidthTargetMet"] = (achieved_kbps >= target_kbps * 0.95) ? 1 : 0; // 95% of target
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * bytes_per_second);
}

BENCHMARK(PacketProcessing_BandwidthTest)
    ->Range(256, 2048) // 256 Kbps to 2 Mbps
    ->Unit(benchmark::kMillisecond)
    ->Name("PacketProcessing/Bandwidth/SustainedThroughput");

// =============================================================================
// Multi-threaded Processing Benchmarks
// =============================================================================

/**
 * Benchmark: Concurrent Packet Processing
 * Tests packet processing with multiple threads
 */
class ConcurrentProcessingFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        thread_count_ = static_cast<int>(state.range(0));
        packet_queue_ = std::make_unique<ThreadSafePacketQueue>();
        
        // Pre-generate packets for all threads
        int packets_per_thread = 100;
        for (int t = 0; t < thread_count_; ++t) {
            std::vector<MockPacket> thread_packets;
            for (int p = 0; p < packets_per_thread; ++p) {
                MockPacket packet;
                packet.data = GenerateGamePacket(256);
                packet.packet_type = MockPacketType::GameData;
                thread_packets.push_back(packet);
            }
            thread_packet_sets_.push_back(thread_packets);
        }
    }

    void TearDown(const benchmark::State& state) override {
        packet_queue_.reset();
        thread_packet_sets_.clear();
    }

protected:
    int thread_count_;
    std::unique_ptr<ThreadSafePacketQueue> packet_queue_;
    std::vector<std::vector<MockPacket>> thread_packet_sets_;
};

BENCHMARK_DEFINE_F(ConcurrentProcessingFixture, MultiThreadProcessing)(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::thread> worker_threads;
        std::atomic<int> total_processed{0};
        
        auto start = high_resolution_clock::now();
        
        // Start worker threads
        for (int t = 0; t < thread_count_; ++t) {
            worker_threads.emplace_back([this, t, &total_processed]() {
                int processed = 0;
                for (const auto& packet : thread_packet_sets_[t]) {
                    if (packet_queue_->ProcessPacket(packet) == MockErrorCode::Success) {
                        processed++;
                    }
                }
                total_processed += processed;
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : worker_threads) {
            thread.join();
        }
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        int expected_packets = thread_count_ * 100;
        double processing_rate = total_processed.load() / (duration_ms / 1000.0);
        
        state.counters["ThreadCount"] = thread_count_;
        state.counters["ExpectedPackets"] = expected_packets;
        state.counters["ProcessedPackets"] = total_processed.load();
        state.counters["ProcessingRate_pps"] = processing_rate;
        state.counters["Efficiency_%"] = (total_processed.load() * 100.0) / expected_packets;
        
        state.SetIterationTime(duration_ms / 1000.0);
    }
}

BENCHMARK_REGISTER_F(ConcurrentProcessingFixture, MultiThreadProcessing)
    ->Range(1, 8) // 1 to 8 threads
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Name("PacketProcessing/Concurrent/MultiThread");

} // namespace Benchmarks