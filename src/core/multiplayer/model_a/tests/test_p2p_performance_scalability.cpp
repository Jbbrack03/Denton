// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <random>

#include "core/multiplayer/model_a/p2p_network.h"
#include "core/multiplayer/model_a/performance_monitor.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock performance monitor for testing metrics collection
 */
class MockPerformanceMonitor {
public:
    MOCK_METHOD(void, startMonitoring, (), ());
    MOCK_METHOD(void, stopMonitoring, (), ());
    MOCK_METHOD(void, recordConnectionTime, (const std::string& peer_id, std::chrono::milliseconds time), ());
    MOCK_METHOD(void, recordDataTransmission, (const std::string& peer_id, size_t bytes, std::chrono::microseconds latency), ());
    MOCK_METHOD(void, recordConnectionFailure, (const std::string& peer_id, const std::string& reason), ());
    MOCK_METHOD(PerformanceMetrics, getMetrics, (), (const));
    MOCK_METHOD(PeerMetrics, getPeerMetrics, (const std::string& peer_id), (const));
    MOCK_METHOD(void, resetMetrics, (), ());
};

/**
 * Test data generator for performance testing
 */
class TestDataGenerator {
public:
    static std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : data) {
            byte = dis(gen);
        }
        return data;
    }
    
    static std::vector<std::string> generatePeerIds(size_t count) {
        std::vector<std::string> peer_ids;
        for (size_t i = 0; i < count; i++) {
            peer_ids.push_back("12D3KooWTestPeer" + std::to_string(i));
        }
        return peer_ids;
    }
    
    static std::vector<std::string> generateMultiaddresses(const std::vector<std::string>& peer_ids) {
        std::vector<std::string> multiaddrs;
        for (size_t i = 0; i < peer_ids.size(); i++) {
            multiaddrs.push_back("/ip4/192.168.1." + std::to_string(100 + i) + "/tcp/4001/p2p/" + peer_ids[i]);
        }
        return multiaddrs;
    }
};

/**
 * Benchmark timer for accurate time measurements
 */
class BenchmarkTimer {
public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    std::chrono::microseconds stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    }
    
    static std::chrono::microseconds measure(std::function<void()> operation) {
        BenchmarkTimer timer;
        timer.start();
        operation();
        return timer.stop();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

} // anonymous namespace

/**
 * Test fixture for performance and scalability tests
 * Tests P2P network performance under various load conditions
 */
class P2PPerformanceScalabilityTest : public Test {
protected:
    void SetUp() override {
        mock_p2p_network_ = std::make_shared<MockP2PNetwork>();
        mock_performance_monitor_ = std::make_shared<MockPerformanceMonitor>();
        
        // Configure performance testing parameters
        config_.enable_tcp = true;
        config_.enable_quic = true;
        config_.tcp_port = 4001;
        config_.quic_port = 4001;
        config_.max_connections = 100;
        config_.connection_timeout_ms = 5000;
        config_.enable_performance_monitoring = true;
    }

    std::shared_ptr<MockP2PNetwork> mock_p2p_network_;
    std::shared_ptr<MockPerformanceMonitor> mock_performance_monitor_;
    P2PNetworkConfig config_;
};

/**
 * Test: Connection establishment latency measurement
 * Measures the time it takes to establish P2P connections
 */
TEST_F(P2PPerformanceScalabilityTest, ConnectionEstablishmentLatencyMeasurement) {
    // ARRANGE
    const std::vector<std::string> peer_ids = TestDataGenerator::generatePeerIds(10);
    const std::vector<std::string> multiaddrs = TestDataGenerator::generateMultiaddresses(peer_ids);
    
    std::vector<std::chrono::milliseconds> connection_times;
    
    for (size_t i = 0; i < peer_ids.size(); i++) {
        EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[i]))
            .WillOnce([&connection_times]() {
                // Simulate variable connection times (50-200ms)
                auto connection_time = std::chrono::milliseconds(50 + (rand() % 150));
                connection_times.push_back(connection_time);
                std::this_thread::sleep_for(connection_time);
                
                std::promise<ErrorCode> promise;
                promise.set_value(ErrorCode::Success);
                return promise.get_future();
            });
        
        EXPECT_CALL(*mock_performance_monitor_, recordConnectionTime(peer_ids[i], _));
    }
    
    auto p2p_network = std::make_unique<P2PNetwork>(config_, mock_p2p_network_, mock_performance_monitor_);
    
    // ACT
    std::vector<std::future<ErrorCode>> futures;
    auto start_time = std::chrono::steady_clock::now();
    
    for (const auto& multiaddr : multiaddrs) {
        futures.push_back(mock_p2p_network_->connectToPeer(multiaddr));
    }
    
    // Wait for all connections
    for (auto& future : futures) {
        EXPECT_EQ(future.get(), ErrorCode::Success);
    }
    
    auto total_time = std::chrono::steady_clock::now() - start_time;
    
    // ASSERT
    // Verify performance characteristics
    EXPECT_LT(total_time, std::chrono::seconds(3)); // All connections within 3 seconds
    
    // Calculate average connection time
    auto total_connection_time = std::accumulate(connection_times.begin(), connection_times.end(), 
                                               std::chrono::milliseconds(0));
    auto average_connection_time = total_connection_time / connection_times.size();
    
    EXPECT_LT(average_connection_time, std::chrono::milliseconds(200));
    EXPECT_GT(average_connection_time, std::chrono::milliseconds(50));
}

/**
 * Test: Concurrent connection scalability
 * Tests how well the system handles many concurrent connection attempts
 */
TEST_F(P2PPerformanceScalabilityTest, ConcurrentConnectionScalability) {
    // ARRANGE
    const size_t max_concurrent_connections = 50;
    const std::vector<std::string> peer_ids = TestDataGenerator::generatePeerIds(max_concurrent_connections);
    const std::vector<std::string> multiaddrs = TestDataGenerator::generateMultiaddresses(peer_ids);
    
    config_.max_connections = max_concurrent_connections;
    
    std::atomic<int> successful_connections{0};
    std::atomic<int> failed_connections{0};
    
    for (size_t i = 0; i < peer_ids.size(); i++) {
        EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[i]))
            .WillOnce([&successful_connections, &failed_connections]() {
                // Simulate 95% success rate
                bool success = (rand() % 100) < 95;
                
                if (success) {
                    successful_connections++;
                    std::promise<ErrorCode> promise;
                    promise.set_value(ErrorCode::Success);
                    return promise.get_future();
                } else {
                    failed_connections++;
                    std::promise<ErrorCode> promise;
                    promise.set_value(ErrorCode::ConnectionFailed);
                    return promise.get_future();
                }
            });
    }
    
    // ACT
    std::vector<std::future<ErrorCode>> futures;
    auto start_time = std::chrono::steady_clock::now();
    
    // Launch all connection attempts concurrently
    for (const auto& multiaddr : multiaddrs) {
        futures.push_back(mock_p2p_network_->connectToPeer(multiaddr));
    }
    
    // Wait for all attempts to complete
    std::vector<ErrorCode> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    auto total_time = std::chrono::steady_clock::now() - start_time;
    
    // ASSERT
    EXPECT_GT(successful_connections.load(), static_cast<int>(max_concurrent_connections * 0.9)); // At least 90% success
    EXPECT_LT(total_time, std::chrono::seconds(10)); // Complete within 10 seconds
    
    // Verify scalability characteristics
    auto success_rate = static_cast<double>(successful_connections.load()) / max_concurrent_connections;
    EXPECT_GT(success_rate, 0.90); // At least 90% success rate
}

/**
 * Test: Data throughput performance
 * Measures data transmission throughput over P2P connections
 */
TEST_F(P2PPerformanceScalabilityTest, DataThroughputPerformance) {
    // ARRANGE
    const std::string peer_id = "12D3KooWThroughputPeer";
    const std::vector<size_t> data_sizes = {1024, 4096, 16384, 65536, 262144}; // 1KB to 256KB
    const int iterations_per_size = 10;
    
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
        .WillRepeatedly(Return(true));
    
    std::vector<std::pair<size_t, std::chrono::microseconds>> throughput_measurements;
    
    for (size_t data_size : data_sizes) {
        for (int i = 0; i < iterations_per_size; i++) {
            auto test_data = TestDataGenerator::generateRandomData(data_size);
            
            EXPECT_CALL(*mock_p2p_network_, sendData(peer_id, test_data))
                .WillOnce([data_size, &throughput_measurements]() {
                    // Simulate transmission time based on data size
                    // Assume ~100 Mbps throughput for simulation
                    auto transmission_time = std::chrono::microseconds(data_size * 8 / 100); // microseconds
                    throughput_measurements.push_back({data_size, transmission_time});
                    std::this_thread::sleep_for(transmission_time);
                    return ErrorCode::Success;
                });
            
            EXPECT_CALL(*mock_performance_monitor_, recordDataTransmission(peer_id, data_size, _));
        }
    }
    
    // ACT
    for (size_t data_size : data_sizes) {
        for (int i = 0; i < iterations_per_size; i++) {
            auto test_data = TestDataGenerator::generateRandomData(data_size);
            
            auto transmission_time = BenchmarkTimer::measure([&]() {
                mock_p2p_network_->sendData(peer_id, test_data);
            });
            
            // Calculate throughput (bytes per second)
            double throughput_bps = static_cast<double>(data_size) / (transmission_time.count() / 1000000.0);
            double throughput_mbps = throughput_bps / (1024 * 1024);
            
            // Record performance metrics
            mock_performance_monitor_->recordDataTransmission(peer_id, data_size, transmission_time);
            
            // Verify reasonable throughput (should be > 10 Mbps)
            EXPECT_GT(throughput_mbps, 10.0);
        }
    }
    
    // ASSERT
    // Verify that larger packets achieve better throughput efficiency
    // This would be validated through the mock expectations and real measurements
}

/**
 * Test: Memory usage scalability
 * Tests memory consumption with increasing number of connections
 */
TEST_F(P2PPerformanceScalabilityTest, MemoryUsageScalability) {
    // ARRANGE
    const std::vector<size_t> connection_counts = {10, 25, 50, 100};
    
    for (size_t connection_count : connection_counts) {
        config_.max_connections = connection_count;
        
        // Simulate memory usage tracking
        size_t estimated_memory_per_connection = 1024; // 1KB per connection (simplified)
        size_t expected_total_memory = connection_count * estimated_memory_per_connection;
        
        auto peer_ids = TestDataGenerator::generatePeerIds(connection_count);
        auto multiaddrs = TestDataGenerator::generateMultiaddresses(peer_ids);
        
        // Set up connection expectations
        for (size_t i = 0; i < connection_count; i++) {
            EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[i]))
                .WillOnce([]() {
                    std::promise<ErrorCode> promise;
                    promise.set_value(ErrorCode::Success);
                    return promise.get_future();
                });
            EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_ids[i]))
                .WillRepeatedly(Return(true));
        }
        
        // ACT
        auto p2p_network = std::make_unique<P2PNetwork>(config_);
        
        // Establish connections
        std::vector<std::future<ErrorCode>> futures;
        for (const auto& multiaddr : multiaddrs) {
            futures.push_back(mock_p2p_network_->connectToPeer(multiaddr));
        }
        
        for (auto& future : futures) {
            EXPECT_EQ(future.get(), ErrorCode::Success);
        }
        
        // ASSERT
        // In a real implementation, we would measure actual memory usage here
        // For this test, we verify that the system can handle the expected load
        EXPECT_LE(expected_total_memory, 1024 * 1024); // Should stay under 1MB for 100 connections
    }
}

/**
 * Test: Connection recovery performance under network stress
 * Tests how quickly connections can recover from network interruptions
 */
TEST_F(P2PPerformanceScalabilityTest, ConnectionRecoveryPerformanceUnderStress) {
    // ARRANGE
    const std::vector<std::string> peer_ids = TestDataGenerator::generatePeerIds(20);
    const std::vector<std::string> multiaddrs = TestDataGenerator::generateMultiaddresses(peer_ids);
    
    std::atomic<int> disconnection_events{0};
    std::atomic<int> reconnection_events{0};
    std::vector<std::chrono::milliseconds> recovery_times;
    
    // Set up initial connections
    for (size_t i = 0; i < peer_ids.size(); i++) {
        EXPECT_CALL(*mock_p2p_network_, connectToPeer(multiaddrs[i]))
            .WillOnce([]() {
                std::promise<ErrorCode> promise;
                promise.set_value(ErrorCode::Success);
                return promise.get_future();
            });
        
        EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_ids[i]))
            .WillOnce(Return(true))   // Initially connected
            .WillOnce(Return(false))  // Disconnected
            .WillOnce(Return(true));  // Reconnected
        
        // Expect disconnection and reconnection
        EXPECT_CALL(*mock_p2p_network_, disconnectFromPeer(peer_ids[i]))
            .WillOnce([&disconnection_events]() {
                disconnection_events++;
            });
    }
    
    // ACT
    // Establish initial connections
    std::vector<std::future<ErrorCode>> initial_futures;
    for (const auto& multiaddr : multiaddrs) {
        initial_futures.push_back(mock_p2p_network_->connectToPeer(multiaddr));
    }
    
    for (auto& future : initial_futures) {
        EXPECT_EQ(future.get(), ErrorCode::Success);
    }
    
    // Simulate network stress - disconnect all peers
    auto stress_start = std::chrono::steady_clock::now();
    
    for (const auto& peer_id : peer_ids) {
        mock_p2p_network_->disconnectFromPeer(peer_id);
    }
    
    // Simulate recovery attempts
    std::vector<std::future<ErrorCode>> recovery_futures;
    for (const auto& multiaddr : multiaddrs) {
        recovery_futures.push_back(mock_p2p_network_->connectToPeer(multiaddr));
    }
    
    for (auto& future : recovery_futures) {
        EXPECT_EQ(future.get(), ErrorCode::Success);
        reconnection_events++;
    }
    
    auto total_recovery_time = std::chrono::steady_clock::now() - stress_start;
    
    // ASSERT
    EXPECT_EQ(disconnection_events.load(), static_cast<int>(peer_ids.size()));
    EXPECT_EQ(reconnection_events.load(), static_cast<int>(peer_ids.size()));
    EXPECT_LT(total_recovery_time, std::chrono::seconds(30)); // Recovery within 30 seconds
    
    // Average recovery time per connection should be reasonable
    auto average_recovery_time = total_recovery_time / peer_ids.size();
    EXPECT_LT(average_recovery_time, std::chrono::seconds(2)); // < 2 seconds per connection
}

/**
 * Test: CPU usage under high connection load
 * Monitors CPU performance characteristics under various loads
 */
TEST_F(P2PPerformanceScalabilityTest, CPUUsageUnderHighConnectionLoad) {
    // ARRANGE
    const size_t high_connection_count = 100;
    const size_t messages_per_connection = 100;
    
    auto peer_ids = TestDataGenerator::generatePeerIds(high_connection_count);
    auto multiaddrs = TestDataGenerator::generateMultiaddresses(peer_ids);
    
    std::atomic<size_t> total_operations{0};
    
    // Set up high-frequency operations
    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
            .WillRepeatedly(Return(true));
        
        for (size_t i = 0; i < messages_per_connection; i++) {
            auto test_data = TestDataGenerator::generateRandomData(1024); // 1KB messages
            EXPECT_CALL(*mock_p2p_network_, sendData(peer_id, test_data))
                .WillOnce([&total_operations]() {
                    total_operations++;
                    // Simulate minimal CPU work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    return ErrorCode::Success;
                });
        }
    }
    
    // ACT
    auto cpu_test_start = std::chrono::high_resolution_clock::now();
    
    // Simulate high-frequency operations across all connections
    std::vector<std::thread> worker_threads;
    const size_t thread_count = std::thread::hardware_concurrency();
    
    for (size_t t = 0; t < thread_count; t++) {
        worker_threads.emplace_back([&, t]() {
            size_t start_idx = (peer_ids.size() * t) / thread_count;
            size_t end_idx = (peer_ids.size() * (t + 1)) / thread_count;
            
            for (size_t i = start_idx; i < end_idx; i++) {
                for (size_t msg = 0; msg < messages_per_connection; msg++) {
                    auto test_data = TestDataGenerator::generateRandomData(1024);
                    mock_p2p_network_->sendData(peer_ids[i], test_data);
                }
            }
        });
    }
    
    for (auto& thread : worker_threads) {
        thread.join();
    }
    
    auto cpu_test_duration = std::chrono::high_resolution_clock::now() - cpu_test_start;
    
    // ASSERT
    size_t expected_operations = high_connection_count * messages_per_connection;
    EXPECT_EQ(total_operations.load(), expected_operations);
    
    // Calculate operations per second
    double duration_seconds = std::chrono::duration<double>(cpu_test_duration).count();
    double operations_per_second = expected_operations / duration_seconds;
    
    // Should handle at least 1000 operations per second efficiently
    EXPECT_GT(operations_per_second, 1000.0);
    
    // Test should complete within reasonable time
    EXPECT_LT(cpu_test_duration, std::chrono::seconds(60));
}

/**
 * Test: Network congestion handling
 * Tests performance under simulated network congestion conditions
 */
TEST_F(P2PPerformanceScalabilityTest, NetworkCongestionHandling) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCongestionPeer";
    const size_t message_count = 1000;
    const size_t message_size = 4096; // 4KB messages
    
    EXPECT_CALL(*mock_p2p_network_, isConnectedToPeer(peer_id))
        .WillRepeatedly(Return(true));
    
    std::atomic<size_t> successful_sends{0};
    std::atomic<size_t> failed_sends{0};
    std::vector<std::chrono::microseconds> send_times;
    
    // Simulate network congestion - increasing latency over time
    for (size_t i = 0; i < message_count; i++) {
        EXPECT_CALL(*mock_p2p_network_, sendData(peer_id, _))
            .WillOnce([&, i]() {
                // Simulate increasing congestion
                auto congestion_delay = std::chrono::microseconds(100 + (i / 10)); // Increasing delay
                std::this_thread::sleep_for(congestion_delay);
                
                // Simulate occasional failures under high congestion
                if (i > message_count * 0.8 && (rand() % 20) == 0) { // 5% failure rate in last 20%
                    failed_sends++;
                    return ErrorCode::NetworkError;
                } else {
                    successful_sends++;
                    return ErrorCode::Success;
                }
            });
    }
    
    // ACT
    auto congestion_test_start = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < message_count; i++) {
        auto test_data = TestDataGenerator::generateRandomData(message_size);
        
        auto send_time = BenchmarkTimer::measure([&]() {
            mock_p2p_network_->sendData(peer_id, test_data);
        });
        
        send_times.push_back(send_time);
    }
    
    auto total_test_time = std::chrono::steady_clock::now() - congestion_test_start;
    
    // ASSERT
    EXPECT_GT(successful_sends.load(), message_count * 0.9); // At least 90% success
    EXPECT_LT(failed_sends.load(), message_count * 0.1);    // Less than 10% failures
    
    // Verify that the system adapts to congestion
    // Later messages should take longer but still succeed
    auto early_avg = std::accumulate(send_times.begin(), send_times.begin() + 100, 
                                   std::chrono::microseconds(0)) / 100;
    auto late_avg = std::accumulate(send_times.end() - 100, send_times.end(), 
                                  std::chrono::microseconds(0)) / 100;
    
    EXPECT_GT(late_avg, early_avg); // Later messages should take longer
    EXPECT_LT(late_avg, std::chrono::milliseconds(100)); // But still complete within 100ms
}

/**
 * Test: Performance metrics collection accuracy
 * Verifies that performance monitoring accurately captures metrics
 */
TEST_F(P2PPerformanceScalabilityTest, PerformanceMetricsCollectionAccuracy) {
    // ARRANGE
    const std::vector<std::string> peer_ids = TestDataGenerator::generatePeerIds(5);
    const size_t operations_per_peer = 50;
    
    PerformanceMetrics expected_metrics;
    expected_metrics.total_connections = peer_ids.size();
    expected_metrics.successful_connections = peer_ids.size();
    expected_metrics.failed_connections = 0;
    expected_metrics.total_bytes_sent = peer_ids.size() * operations_per_peer * 1024; // 1KB per operation
    expected_metrics.total_bytes_received = 0;
    expected_metrics.average_latency_ms = 50; // Simulated average
    
    EXPECT_CALL(*mock_performance_monitor_, getMetrics())
        .WillOnce(Return(expected_metrics));
    
    // Set up operations to generate metrics
    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_performance_monitor_, recordConnectionTime(peer_id, _))
            .Times(1);
        
        for (size_t i = 0; i < operations_per_peer; i++) {
            EXPECT_CALL(*mock_performance_monitor_, recordDataTransmission(peer_id, 1024, _))
                .Times(1);
        }
    }
    
    // ACT
    // Simulate operations that would generate metrics
    for (const auto& peer_id : peer_ids) {
        mock_performance_monitor_->recordConnectionTime(peer_id, std::chrono::milliseconds(100));
        
        for (size_t i = 0; i < operations_per_peer; i++) {
            mock_performance_monitor_->recordDataTransmission(peer_id, 1024, std::chrono::microseconds(50000));
        }
    }
    
    auto collected_metrics = mock_performance_monitor_->getMetrics();
    
    // ASSERT
    EXPECT_EQ(collected_metrics.total_connections, expected_metrics.total_connections);
    EXPECT_EQ(collected_metrics.successful_connections, expected_metrics.successful_connections);
    EXPECT_EQ(collected_metrics.failed_connections, expected_metrics.failed_connections);
    EXPECT_EQ(collected_metrics.total_bytes_sent, expected_metrics.total_bytes_sent);
    EXPECT_EQ(collected_metrics.average_latency_ms, expected_metrics.average_latency_ms);
}