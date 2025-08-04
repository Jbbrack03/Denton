// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <random>
#include <algorithm>
#include <numeric>

#include "benchmark_mocks.h"
#include "benchmark_utilities.h"

namespace Benchmarks {

using namespace std::chrono;

/**
 * Component-specific performance benchmarks for:
 * - HLE Interface (nn::ldn service calls)
 * - Configuration System (settings validation and loading)
 * - Error Handling (error detection and recovery)
 * - Platform-specific components (Windows, Android)
 */

// =============================================================================
// HLE Interface Performance Benchmarks
// =============================================================================

/**
 * Benchmark: nn::ldn Service Call Performance
 * Tests the performance of individual HLE service calls
 */
class HLEInterfaceFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_hle_service_ = std::make_unique<MockLdnService>();
        mock_backend_ = std::make_unique<MockMultiplayerBackend>();
        
        // Configure HLE service
        mock_hle_service_->SetBackend(mock_backend_.get());
        mock_hle_service_->Initialize();
    }

    void TearDown(const benchmark::State& state) override {
        mock_hle_service_->Shutdown();
        mock_hle_service_.reset();
        mock_backend_.reset();
    }

protected:
    std::unique_ptr<MockLdnService> mock_hle_service_;
    std::unique_ptr<MockMultiplayerBackend> mock_backend_;
};

BENCHMARK_DEFINE_F(HLEInterfaceFixture, InitializeService)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Reset service to uninitialized state
        mock_hle_service_->Shutdown();
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        auto result = mock_hle_service_->Initialize();
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("HLE service initialization failed");
            continue;
        }
        
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: HLE initialization should be < 10ms
    state.counters["PerformanceTarget_ms"] = 10.0;
}

BENCHMARK_REGISTER_F(HLEInterfaceFixture, InitializeService)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(100)
    ->Name("Component/HLE/InitializeService");

BENCHMARK_DEFINE_F(HLEInterfaceFixture, CreateNetwork)(benchmark::State& state) {
    for (auto _ : state) {
        MockNetworkConfig config;
        config.ssid = "TestNetwork";
        config.passphrase = "testpass123";
        config.max_players = 8;
        config.channel = 6;
        
        auto start = high_resolution_clock::now();
        
        auto result = mock_hle_service_->CreateNetwork(config);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        state.PauseTiming();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Network creation failed");
            continue;
        }
        
        // Clean up network
        mock_hle_service_->DestroyNetwork();
        
        state.ResumeTiming();
        
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: Network creation should be < 5ms
    state.counters["PerformanceTarget_ms"] = 5.0;
}

BENCHMARK_REGISTER_F(HLEInterfaceFixture, CreateNetwork)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(1000)
    ->Name("Component/HLE/CreateNetwork");

BENCHMARK_DEFINE_F(HLEInterfaceFixture, ScanForNetworks)(benchmark::State& state) {
    // Pre-populate with mock networks
    std::vector<MockNetworkInfo> mock_networks;
    for (int i = 0; i < 10; ++i) {
        MockNetworkInfo network;
        network.ssid = "Network_" + std::to_string(i);
        network.player_count = (i % 8) + 1;
        network.max_players = 8;
        mock_networks.push_back(network);
    }
    mock_backend_->SetAvailableNetworks(mock_networks);
    
    for (auto _ : state) {
        MockScanConfig scan_config;
        scan_config.timeout_ms = 1000;
        scan_config.max_results = 10;
        
        auto start = high_resolution_clock::now();
        
        auto networks = mock_hle_service_->ScanForNetworks(scan_config);
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        if (networks.empty()) {
            state.SkipWithError("No networks found during scan");
            continue;
        }
        
        state.counters["NetworksFound"] = networks.size();
        state.counters["ScanTime_ms"] = duration_ms;
        state.SetIterationTime(duration_ms / 1000.0);
    }
    
    // Performance target: Network scan should complete within timeout
    state.counters["PerformanceTarget_ms"] = 1000.0;
}

BENCHMARK_REGISTER_F(HLEInterfaceFixture, ScanForNetworks)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(50)
    ->Name("Component/HLE/ScanForNetworks");

BENCHMARK_DEFINE_F(HLEInterfaceFixture, SendReceivePackets)(benchmark::State& state) {
    // Setup network session
    MockNetworkConfig config;
    config.ssid = "TestNetwork";
    config.passphrase = "testpass123";
    mock_hle_service_->CreateNetwork(config);
    
    for (auto _ : state) {
        MockPacket test_packet;
        test_packet.data = GenerateGamePacket(256);
        test_packet.packet_type = MockPacketType::GameData;
        
        auto start = high_resolution_clock::now();
        
        // Send packet
        auto send_result = mock_hle_service_->SendPacket(test_packet);
        
        // Receive packet (simulated echo)
        auto received_packet = mock_hle_service_->ReceivePacket();
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (send_result != MockErrorCode::Success || !received_packet.has_value()) {
            state.SkipWithError("Packet send/receive failed");
            continue;
        }
        
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    mock_hle_service_->DestroyNetwork();
    
    // Performance target: Packet round-trip should be < 1ms for local processing
    state.counters["PerformanceTarget_ms"] = 1.0;
}

BENCHMARK_REGISTER_F(HLEInterfaceFixture, SendReceivePackets)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(5000)
    ->Name("Component/HLE/SendReceivePackets");

// =============================================================================
// Configuration System Performance Benchmarks
// =============================================================================

/**
 * Benchmark: Configuration Loading and Validation Performance
 */
class ConfigurationFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_config_manager_ = std::make_unique<MockConfigurationManager>();
        
        // Create test configuration files of varying sizes
        small_config_ = GenerateTestConfiguration(10);   // 10 settings
        medium_config_ = GenerateTestConfiguration(100); // 100 settings
        large_config_ = GenerateTestConfiguration(1000); // 1000 settings
    }

    void TearDown(const benchmark::State& state) override {
        mock_config_manager_.reset();
    }

private:
    MockConfiguration GenerateTestConfiguration(int setting_count) {
        MockConfiguration config;
        
        for (int i = 0; i < setting_count; ++i) {
            std::string key = "setting_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            config.SetValue(key, value);
        }
        
        return config;
    }

protected:
    std::unique_ptr<MockConfigurationManager> mock_config_manager_;
    MockConfiguration small_config_;
    MockConfiguration medium_config_;
    MockConfiguration large_config_;
};

BENCHMARK_DEFINE_F(ConfigurationFixture, LoadConfiguration)(benchmark::State& state) {
    int config_size = static_cast<int>(state.range(0));
    
    MockConfiguration* config_to_load = nullptr;
    if (config_size <= 10) {
        config_to_load = &small_config_;
    } else if (config_size <= 100) {
        config_to_load = &medium_config_;
    } else {
        config_to_load = &large_config_;
    }
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        auto result = mock_config_manager_->LoadConfiguration(*config_to_load);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Configuration loading failed");
            continue;
        }
        
        state.counters["ConfigSize"] = config_size;
        state.counters["LoadTime_us"] = duration_us;
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: Configuration loading should be < 1ms per 100 settings
    double target_us = (config_size / 100.0) * 1000.0;
    state.counters["PerformanceTarget_us"] = target_us;
}

BENCHMARK_REGISTER_F(ConfigurationFixture, LoadConfiguration)
    ->Range(10, 1000) // Test with 10 to 1000 settings
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Name("Component/Configuration/LoadConfiguration");

BENCHMARK_DEFINE_F(ConfigurationFixture, ValidateConfiguration)(benchmark::State& state) {
    // Create configuration with validation rules
    MockConfiguration config = medium_config_;
    
    // Add validation rules
    mock_config_manager_->AddValidationRule("network.port", MockValidationType::IntegerRange, "1-65535");
    mock_config_manager_->AddValidationRule("network.timeout", MockValidationType::IntegerRange, "1000-30000");
    mock_config_manager_->AddValidationRule("player.name", MockValidationType::StringLength, "1-32");
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        auto validation_result = mock_config_manager_->ValidateConfiguration(config);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        state.counters["ValidationRules"] = 3;
        state.counters["ConfigEntries"] = 100;
        state.counters["ValidationTime_us"] = duration_us;
        state.counters["ValidationPassed"] = validation_result.is_valid ? 1 : 0;
        
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: Configuration validation should be < 5ms
    state.counters["PerformanceTarget_ms"] = 5.0;
}

BENCHMARK_REGISTER_F(ConfigurationFixture, ValidateConfiguration)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(1000)
    ->Name("Component/Configuration/ValidateConfiguration");

BENCHMARK_DEFINE_F(ConfigurationFixture, ConfigurationSerialization)(benchmark::State& state) {
    MockConfiguration config = large_config_;
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        // Serialize to JSON
        auto json_data = mock_config_manager_->SerializeToJson(config);
        
        // Deserialize from JSON
        auto deserialized_config = mock_config_manager_->DeserializeFromJson(json_data);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (json_data.empty() || !deserialized_config.has_value()) {
            state.SkipWithError("Configuration serialization failed");
            continue;
        }
        
        state.counters["JsonSize_bytes"] = json_data.size();
        state.counters["SerializationTime_us"] = duration_us;
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * large_config_.GetSize());
}

BENCHMARK_REGISTER_F(ConfigurationFixture, ConfigurationSerialization)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(100)
    ->Name("Component/Configuration/Serialization");

// =============================================================================
// Error Handling Performance Benchmarks
// =============================================================================

/**
 * Benchmark: Error Detection and Recovery Performance
 */
class ErrorHandlingFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_error_handler_ = std::make_unique<MockErrorHandler>();
        mock_recovery_manager_ = std::make_unique<MockRecoveryManager>();
        
        // Configure error handling
        mock_error_handler_->SetRecoveryManager(mock_recovery_manager_.get());
        mock_error_handler_->EnableAutoRecovery(true);
    }

    void TearDown(const benchmark::State& state) override {
        mock_error_handler_.reset();
        mock_recovery_manager_.reset();
    }

protected:    
    std::unique_ptr<MockErrorHandler> mock_error_handler_;
    std::unique_ptr<MockRecoveryManager> mock_recovery_manager_;
};

BENCHMARK_DEFINE_F(ErrorHandlingFixture, ErrorDetection)(benchmark::State& state) {
    // Simulate various error conditions
    std::vector<MockErrorCondition> error_conditions = {
        MockErrorCondition::NetworkTimeout,
        MockErrorCondition::ConnectionLost,
        MockErrorCondition::InvalidPacket,
        MockErrorCondition::ResourceExhausted,
        MockErrorCondition::PermissionDenied
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, error_conditions.size() - 1);
    
    for (auto _ : state) {
        // Select random error condition
        auto error_condition = error_conditions[dis(gen)];
        
        auto start = high_resolution_clock::now();
        
        // Trigger error detection
        auto error_info = mock_error_handler_->DetectError(error_condition);
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        if (!error_info.has_value()) {
            state.SkipWithError("Error detection failed");
            continue;
        }
        
        state.counters["ErrorType"] = static_cast<int>(error_condition);
        state.counters["DetectionTime_us"] = duration_us;
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: Error detection should be < 100μs
    state.counters["PerformanceTarget_us"] = 100.0;
}

BENCHMARK_REGISTER_F(ErrorHandlingFixture, ErrorDetection)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(10000)
    ->Name("Component/ErrorHandling/ErrorDetection");

BENCHMARK_DEFINE_F(ErrorHandlingFixture, ErrorRecovery)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create error scenario
        MockErrorInfo error;
        error.error_code = MockErrorCode::ConnectionLost;
        error.category = MockErrorCategory::NetworkConnectivity;
        error.is_recoverable = true;
        error.suggested_recovery = MockRecoveryAction::Reconnect;
        
        state.ResumeTiming();
        
        auto start = high_resolution_clock::now();
        
        // Attempt error recovery
        auto recovery_result = mock_recovery_manager_->RecoverFromError(error);
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.PauseTiming();
        
        bool recovery_successful = (recovery_result == MockRecoveryResult::Success);
        
        state.counters["RecoveryTime_ms"] = duration_ms;
        state.counters["RecoverySuccessful"] = recovery_successful ? 1 : 0;
        state.SetIterationTime(duration_ms / 1000.0);
        
        state.ResumeTiming();
    }
    
    // Performance target: Error recovery should complete within 5 seconds
    state.counters["PerformanceTarget_ms"] = 5000.0;
}

BENCHMARK_REGISTER_F(ErrorHandlingFixture, ErrorRecovery)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(100)
    ->Name("Component/ErrorHandling/ErrorRecovery");

// =============================================================================
// Platform-Specific Component Benchmarks
// =============================================================================

/**
 * Benchmark: Windows Mobile Hotspot Performance
 */
#ifdef _WIN32
class WindowsComponentFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_hotspot_manager_ = std::make_unique<MockMobileHotspotManager>();
        mock_winrt_apis_ = std::make_unique<MockWinRTAPIs>();
        
        // Configure Windows environment
        mock_winrt_apis_->SetWindowsVersion(10, 0, 19041); // Windows 10 2004
        mock_winrt_apis_->SetElevated(true);
        
        mock_hotspot_manager_->SetWinRTAPIs(mock_winrt_apis_.get());
    }

    void TearDown(const benchmark::State& state) override {
        mock_hotspot_manager_.reset();
        mock_winrt_apis_.reset();
    }

protected:
    std::unique_ptr<MockMobileHotspotManager> mock_hotspot_manager_;
    std::unique_ptr<MockWinRTAPIs> mock_winrt_apis_;
};

BENCHMARK_DEFINE_F(WindowsComponentFixture, HotspotCapabilityCheck)(benchmark::State& state) {
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        bool is_capable = mock_hotspot_manager_->IsHotspotCapable();
        
        auto end = high_resolution_clock::now();
        auto duration_us = duration_cast<microseconds>(end - start).count();
        
        state.counters["IsCapable"] = is_capable ? 1 : 0;
        state.counters["CheckTime_us"] = duration_us;
        state.SetIterationTime(duration_us / 1000000.0);
    }
    
    // Performance target: Capability check should be < 1ms
    state.counters["PerformanceTarget_ms"] = 1.0;
}

BENCHMARK_REGISTER_F(WindowsComponentFixture, HotspotCapabilityCheck)
    ->Unit(benchmark::kMicrosecond)
    ->UseManualTime()
    ->Iterations(1000)
    ->Name("Component/Windows/HotspotCapabilityCheck");

BENCHMARK_DEFINE_F(WindowsComponentFixture, HotspotStartStop)(benchmark::State& state) {
    // Initialize hotspot manager
    mock_hotspot_manager_->Initialize();
    
    MockHotspotConfiguration config;
    config.ssid = "SudachiTest";
    config.passphrase = "testpass123";
    config.max_clients = 4;
    mock_hotspot_manager_->ConfigureHotspot(config);
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        // Start hotspot
        auto start_result = mock_hotspot_manager_->StartHotspot();
        
        // Stop hotspot
        auto stop_result = mock_hotspot_manager_->StopHotspot();
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        if (start_result != MockErrorCode::Success || stop_result != MockErrorCode::Success) {
            state.SkipWithError("Hotspot start/stop failed");
            continue;
        }
        
        state.counters["StartStopTime_ms"] = duration_ms;
        state.SetIterationTime(duration_ms / 1000.0);
    }
    
    // Performance target: Hotspot start/stop should be < 3 seconds
    state.counters["PerformanceTarget_ms"] = 3000.0;
}

BENCHMARK_REGISTER_F(WindowsComponentFixture, HotspotStartStop)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(10)
    ->Name("Component/Windows/HotspotStartStop");
#endif // _WIN32

/**
 * Benchmark: Android Wi-Fi Direct Performance
 */
#ifdef __ANDROID__
class AndroidComponentFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_wifi_direct_ = std::make_unique<MockWiFiDirectWrapper>();
        mock_jni_env_ = std::make_unique<MockJNIEnvironment>();
        
        // Configure Android environment
        mock_jni_env_->SetSDKVersion(33); // Android 13
        mock_wifi_direct_->SetJNIEnvironment(mock_jni_env_.get());
    }

    void TearDown(const benchmark::State& state) override {
        mock_wifi_direct_.reset();
        mock_jni_env_.reset();
    }

protected:
    std::unique_ptr<MockWiFiDirectWrapper> mock_wifi_direct_;
    std::unique_ptr<MockJNIEnvironment> mock_jni_env_;
};

BENCHMARK_DEFINE_F(AndroidComponentFixture, WiFiDirectDiscovery)(benchmark::State& state) {
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        auto peers = mock_wifi_direct_->DiscoverPeers(5000); // 5 second timeout
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.counters["PeersFound"] = peers.size();
        state.counters["DiscoveryTime_ms"] = duration_ms;
        state.SetIterationTime(duration_ms / 1000.0);
    }
    
    // Performance target: Peer discovery should complete within timeout
    state.counters["PerformanceTarget_ms"] = 5000.0;
}

BENCHMARK_REGISTER_F(AndroidComponentFixture, WiFiDirectDiscovery)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(20)
    ->Name("Component/Android/WiFiDirectDiscovery");

BENCHMARK_DEFINE_F(AndroidComponentFixture, WiFiDirectConnection)(benchmark::State& state) {
    // Pre-populate with mock peer
    MockWiFiDirectPeer peer;
    peer.device_address = "aa:bb:cc:dd:ee:ff";
    peer.device_name = "TestDevice";
    mock_wifi_direct_->AddMockPeer(peer);
    
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        auto result = mock_wifi_direct_->ConnectToPeer(peer.device_address);
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.PauseTiming();
        
        if (result != MockErrorCode::Success) {
            state.SkipWithError("Wi-Fi Direct connection failed");
            continue;
        }
        
        // Disconnect for next iteration
        mock_wifi_direct_->Disconnect();
        
        state.ResumeTiming();
        
        state.counters["ConnectionTime_ms"] = duration_ms;
        state.SetIterationTime(duration_ms / 1000.0);
    }
    
    // Performance target: Wi-Fi Direct connection should be < 10 seconds
    state.counters["PerformanceTarget_ms"] = 10000.0;
}

BENCHMARK_REGISTER_F(AndroidComponentFixture, WiFiDirectConnection)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(10)
    ->Name("Component/Android/WiFiDirectConnection");
#endif // __ANDROID__

// =============================================================================
// Cross-Component Integration Benchmarks
// =============================================================================

/**
 * Benchmark: Full Component Integration Performance
 * Tests performance of multiple components working together
 */
class IntegrationFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        mock_hle_service_ = std::make_unique<MockLdnService>();
        mock_config_manager_ = std::make_unique<MockConfigurationManager>();
        mock_error_handler_ = std::make_unique<MockErrorHandler>();
        mock_backend_ = std::make_unique<MockMultiplayerBackend>();
        
        // Setup component integration
        mock_hle_service_->SetConfigurationManager(mock_config_manager_.get());
        mock_hle_service_->SetErrorHandler(mock_error_handler_.get());
        mock_hle_service_->SetBackend(mock_backend_.get());
    }

    void TearDown(const benchmark::State& state) override {
        mock_hle_service_.reset();
        mock_config_manager_.reset();
        mock_error_handler_.reset();
        mock_backend_.reset();
    }

protected:
    std::unique_ptr<MockLdnService> mock_hle_service_;
    std::unique_ptr<MockConfigurationManager> mock_config_manager_;
    std::unique_ptr<MockErrorHandler> mock_error_handler_;
    std::unique_ptr<MockMultiplayerBackend> mock_backend_;
};

BENCHMARK_DEFINE_F(IntegrationFixture, FullSessionLifecycle)(benchmark::State& state) {
    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        
        // 1. Load configuration
        MockConfiguration config = MockConfiguration::GetDefault();
        mock_config_manager_->LoadConfiguration(config);
        
        // 2. Initialize HLE service
        mock_hle_service_->Initialize();
        
        // 3. Create network
        MockNetworkConfig network_config;
        network_config.ssid = "TestSession";
        network_config.passphrase = "testpass123";
        mock_hle_service_->CreateNetwork(network_config);
        
        // 4. Send/receive packets
        for (int i = 0; i < 10; ++i) {
            MockPacket packet;
            packet.data = GenerateGamePacket(256);
            mock_hle_service_->SendPacket(packet);
            mock_hle_service_->ReceivePacket();
        }
        
        // 5. Handle simulated error
        mock_error_handler_->DetectError(MockErrorCondition::NetworkTimeout);
        
        // 6. Clean up
        mock_hle_service_->DestroyNetwork();
        mock_hle_service_->Shutdown();
        
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start).count();
        
        state.counters["SessionLifecycleTime_ms"] = duration_ms;
        state.counters["PacketsProcessed"] = 10;
        state.SetIterationTime(duration_ms / 1000.0);
    }
    
    // Performance target: Full session lifecycle should be < 100ms
    state.counters["PerformanceTarget_ms"] = 100.0;
}

BENCHMARK_REGISTER_F(IntegrationFixture, FullSessionLifecycle)
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()
    ->Iterations(50)
    ->Name("Component/Integration/FullSessionLifecycle");

} // namespace Benchmarks