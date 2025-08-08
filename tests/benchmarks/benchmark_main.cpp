// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <benchmark/benchmark.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <algorithm>

/**
 * Custom benchmark main with PRD target validation and performance reporting
 */

namespace Benchmarks {

/**
 * PRD Performance Targets from Section 7.1
 */
struct PRDTargets {
    // Connection Establishment (ms)
    static constexpr double INITIAL_CONNECTION_MS = 3000.0;
    static constexpr double P2P_NEGOTIATION_MS = 5000.0;
    static constexpr double ADHOC_DISCOVERY_MS = 2000.0;
    static constexpr double RECONNECTION_MS = 1000.0;
    
    // Latency Targets (ms)
    static constexpr double LOCAL_ADHOC_OVERHEAD_MS = 5.0;
    static constexpr double P2P_DIRECT_OVERHEAD_MS = 20.0;
    static constexpr double RELAY_SERVER_OVERHEAD_MS = 50.0;
    static constexpr double MAX_JITTER_MS = 10.0;
    
    // Throughput Requirements
    static constexpr double MIN_BANDWIDTH_KBPS = 256.0;
    static constexpr double RECOMMENDED_BANDWIDTH_KBPS = 1024.0;
    static constexpr int MAX_PACKET_SIZE_BYTES = 1400;
    static constexpr double PACKET_FREQUENCY_HZ = 60.0;
    static constexpr double FRAME_BUDGET_MS = 16.67; // 1000ms / 60Hz
    
    // Scalability Targets
    static constexpr int ROOM_SERVER_MAX_CONNECTIONS = 100000;
    static constexpr int RELAY_SERVER_MAX_SESSIONS = 10000;
    static constexpr int MAX_PLAYERS_PER_SESSION = 8;
    static constexpr double ROOM_LIST_QUERY_MS = 100.0;
    
    // Component Performance Targets
    static constexpr double HLE_INITIALIZATION_MS = 10.0;
    static constexpr double CONFIGURATION_LOADING_MS = 5.0;
    static constexpr double ERROR_DETECTION_US = 100.0;
    static constexpr double ERROR_RECOVERY_MS = 5000.0;
};

/**
 * Performance result analyzer
 */
class PerformanceAnalyzer {
public:
    struct BenchmarkResult {
        std::string name;
        double measured_value;
        double target_value;
        std::string unit;
        bool passed;
        double deviation_percent;
        std::string category;
    };
    
    void AnalyzeBenchmarkResult(const std::string& name, double measured, double target, 
                               const std::string& unit, const std::string& category) {
        BenchmarkResult result;
        result.name = name;
        result.measured_value = measured;
        result.target_value = target;
        result.unit = unit;
        result.category = category;
        result.passed = measured <= target;
        result.deviation_percent = ((measured - target) / target) * 100.0;
        
        results_.push_back(result);
    }
    
    void PrintSummaryReport() const {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "SUDACHI MULTIPLAYER PERFORMANCE BENCHMARK REPORT" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        // Group results by category
        std::map<std::string, std::vector<BenchmarkResult>> categorized_results;
        for (const auto& result : results_) {
            categorized_results[result.category].push_back(result);
        }
        
        // Print results by category
        for (const auto& [category, category_results] : categorized_results) {
            PrintCategoryResults(category, category_results);
        }
        
        // Overall summary
        PrintOverallSummary();
    }
    
    void ExportToJSON(const std::string& filename) const {
        std::ofstream file(filename);
        file << "{\n";
        file << "  \"sudachi_multiplayer_performance_report\": {\n";
        file << "    \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n";
        file << "    \"categories\": {\n";
        
        std::map<std::string, std::vector<BenchmarkResult>> categorized_results;
        for (const auto& result : results_) {
            categorized_results[result.category].push_back(result);
        }
        
        bool first_category = true;
        for (const auto& [category, category_results] : categorized_results) {
            if (!first_category) file << ",\n";
            file << "      \"" << category << "\": {\n";
            file << "        \"tests\": [\n";
            
            bool first_test = true;
            for (const auto& result : category_results) {
                if (!first_test) file << ",\n";
                file << "          {\n";
                file << "            \"name\": \"" << result.name << "\",\n";
                file << "            \"measured_value\": " << result.measured_value << ",\n";
                file << "            \"target_value\": " << result.target_value << ",\n";
                file << "            \"unit\": \"" << result.unit << "\",\n";
                file << "            \"passed\": " << (result.passed ? "true" : "false") << ",\n";
                file << "            \"deviation_percent\": " << result.deviation_percent << "\n";
                file << "          }";
                first_test = false;
            }
            
            file << "\n        ],\n";
            
            // Category summary
            int passed = std::count_if(category_results.begin(), category_results.end(),
                                     [](const BenchmarkResult& r) { return r.passed; });
            file << "        \"summary\": {\n";
            file << "          \"total_tests\": " << category_results.size() << ",\n";
            file << "          \"passed_tests\": " << passed << ",\n";
            file << "          \"pass_rate\": " << (passed * 100.0 / category_results.size()) << "\n";
            file << "        }\n";
            file << "      }";
            first_category = false;
        }
        
        file << "\n    },\n";
        
        // Overall summary
        int total_tests = results_.size();
        int total_passed = std::count_if(results_.begin(), results_.end(),
                                       [](const BenchmarkResult& r) { return r.passed; });
        
        file << "    \"overall_summary\": {\n";
        file << "      \"total_tests\": " << total_tests << ",\n";
        file << "      \"passed_tests\": " << total_passed << ",\n";
        file << "      \"overall_pass_rate\": " << (total_passed * 100.0 / total_tests) << ",\n";
        file << "      \"prd_compliance\": \"" << (total_passed == total_tests ? "PASS" : "FAIL") << "\"\n";
        file << "    }\n";
        file << "  }\n";
        file << "}\n";
        
        file.close();
        std::cout << "Performance report exported to: " << filename << std::endl;
    }

private:
    std::vector<BenchmarkResult> results_;
    
    void PrintCategoryResults(const std::string& category, const std::vector<BenchmarkResult>& results) const {
        std::cout << "\n" << category << " Performance Results:" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        std::cout << std::left << std::setw(40) << "Test Name";
        std::cout << std::setw(12) << "Measured";
        std::cout << std::setw(12) << "Target";
        std::cout << std::setw(8) << "Status";
        std::cout << "Deviation" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(40) << TruncateString(result.name, 38);
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << result.measured_value;
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << result.target_value;
            std::cout << std::setw(8) << (result.passed ? "PASS" : "FAIL");
            std::cout << std::showpos << std::fixed << std::setprecision(1) << result.deviation_percent << "%" << std::endl;
        }
        
        // Category summary
        int passed = std::count_if(results.begin(), results.end(),
                                 [](const BenchmarkResult& r) { return r.passed; });
        double pass_rate = (passed * 100.0) / results.size();
        
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "Category Summary: " << passed << "/" << results.size() 
                  << " tests passed (" << std::fixed << std::setprecision(1) << pass_rate << "%)" << std::endl;
    }
    
    void PrintOverallSummary() const {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "OVERALL PERFORMANCE SUMMARY" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        int total_tests = results_.size();
        int total_passed = std::count_if(results_.begin(), results_.end(),
                                       [](const BenchmarkResult& r) { return r.passed; });
        
        double overall_pass_rate = (total_passed * 100.0) / total_tests;
        
        std::cout << "Total Performance Tests: " << total_tests << std::endl;
        std::cout << "Tests Passed: " << total_passed << std::endl;
        std::cout << "Tests Failed: " << (total_tests - total_passed) << std::endl;
        std::cout << "Overall Pass Rate: " << std::fixed << std::setprecision(1) << overall_pass_rate << "%" << std::endl;
        
        std::cout << "\nPRD Compliance Status: ";
        if (total_passed == total_tests) {
            std::cout << "✅ PASS - All performance targets met!" << std::endl;
        } else {
            std::cout << "❌ FAIL - " << (total_tests - total_passed) << " performance targets not met" << std::endl;
        }
        
        // Critical failures
        std::vector<BenchmarkResult> critical_failures;
        for (const auto& result : results_) {
            if (!result.passed && result.deviation_percent > 50.0) { // More than 50% over target
                critical_failures.push_back(result);
            }
        }
        
        if (!critical_failures.empty()) {
            std::cout << "\nCRITICAL PERFORMANCE ISSUES:" << std::endl;
            for (const auto& failure : critical_failures) {
                std::cout << "  ⚠️  " << failure.name << " exceeded target by " 
                          << std::fixed << std::setprecision(1) << failure.deviation_percent << "%" << std::endl;
            }
        }
    }
    
    std::string TruncateString(const std::string& str, size_t max_length) const {
        if (str.length() <= max_length) {
            return str;
        }
        return str.substr(0, max_length - 3) + "...";
    }
    
    std::string GetCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Global analyzer instance
PerformanceAnalyzer g_analyzer;

/**
 * Custom benchmark reporter for PRD validation
 */
class PRDValidationReporter : public benchmark::ConsoleReporter {
public:
    void ReportRuns(const std::vector<benchmark::Run>& reports) override {
        // Call base implementation
        benchmark::ConsoleReporter::ReportRuns(reports);
        
        // Analyze results against PRD targets
        for (const auto& run : reports) {
            AnalyzeBenchmarkRun(run);
        }
    }

private:
    void AnalyzeBenchmarkRun(const benchmark::Run& run) {
        std::string name = run.benchmark_name();
        
        // Parse benchmark results and compare against PRD targets
        if (name.find("ConnectionEstablishment") != std::string::npos) {
            AnalyzeConnectionBenchmark(run);
        } else if (name.find("Latency") != std::string::npos) {
            AnalyzeLatencyBenchmark(run);
        } else if (name.find("PacketProcessing") != std::string::npos) {
            AnalyzePacketProcessingBenchmark(run);
        } else if (name.find("Scalability") != std::string::npos) {
            AnalyzeScalabilityBenchmark(run);
        } else if (name.find("Component") != std::string::npos) {
            AnalyzeComponentBenchmark(run);
        }
    }
    
    void AnalyzeConnectionBenchmark(const benchmark::Run& run) {
        double measured_ms = run.real_accumulated_time / 1e6; // Convert nanoseconds to milliseconds
        std::string name = run.benchmark_name();
        
        if (name.find("RoomServer") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::INITIAL_CONNECTION_MS, 
                                            "ms", "Connection Establishment");
        } else if (name.find("P2P") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::P2P_NEGOTIATION_MS, 
                                            "ms", "Connection Establishment");
        } else if (name.find("AdHoc") != std::string::npos || name.find("WiFiDirect") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::ADHOC_DISCOVERY_MS, 
                                            "ms", "Connection Establishment");
        } else if (name.find("Reconnection") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::RECONNECTION_MS, 
                                            "ms", "Connection Establishment");
        }
    }
    
    void AnalyzeLatencyBenchmark(const benchmark::Run& run) {
        double measured_ms = run.real_accumulated_time / 1e6;
        std::string name = run.benchmark_name();
        
        if (name.find("AdHoc") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::LOCAL_ADHOC_OVERHEAD_MS, 
                                            "ms", "Latency");
        } else if (name.find("P2P") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::P2P_DIRECT_OVERHEAD_MS, 
                                            "ms", "Latency");
        } else if (name.find("Relay") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::RELAY_SERVER_OVERHEAD_MS, 
                                            "ms", "Latency");
        }
        
        // Check for jitter in counters
        if (run.counters.find("Jitter_ms") != run.counters.end()) {
            double jitter = run.counters.at("Jitter_ms");
            g_analyzer.AnalyzeBenchmarkResult(name + "_Jitter", jitter, PRDTargets::MAX_JITTER_MS, 
                                            "ms", "Latency");
        }
    }
    
    void AnalyzePacketProcessingBenchmark(const benchmark::Run& run) {
        std::string name = run.benchmark_name();
        
        if (name.find("60Hz") != std::string::npos) {
            double measured_ms = run.real_accumulated_time / 1e6;
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::FRAME_BUDGET_MS, 
                                            "ms", "Packet Processing");
        }
        
        // Check bandwidth performance
        if (run.counters.find("AchievedBandwidth_Kbps") != run.counters.end()) {
            double bandwidth = run.counters.at("AchievedBandwidth_Kbps");
            g_analyzer.AnalyzeBenchmarkResult(name + "_Bandwidth", bandwidth, PRDTargets::MIN_BANDWIDTH_KBPS, 
                                            "Kbps", "Packet Processing");
        }
    }
    
    void AnalyzeScalabilityBenchmark(const benchmark::Run& run) {
        std::string name = run.benchmark_name();
        
        if (name.find("RoomServer") != std::string::npos) {
            if (run.counters.find("ConnectionCount") != run.counters.end()) {
                double connections = run.counters.at("ConnectionCount");
                // For scalability, we check if the system can handle the load, not a specific target
                g_analyzer.AnalyzeBenchmarkResult(name + "_Scalability", connections, 1000.0, 
                                                "connections", "Scalability");
            }
        }
        
        if (name.find("QueryPerformance") != std::string::npos) {
            double measured_ms = run.real_accumulated_time / 1e6;
            g_analyzer.AnalyzeBenchmarkResult(name, measured_ms, PRDTargets::ROOM_LIST_QUERY_MS, 
                                            "ms", "Scalability");
        }
    }
    
    void AnalyzeComponentBenchmark(const benchmark::Run& run) {
        double measured_value = run.real_accumulated_time / 1e6; // Convert to milliseconds
        std::string name = run.benchmark_name();
        
        if (name.find("HLE") != std::string::npos && name.find("Initialize") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_value, PRDTargets::HLE_INITIALIZATION_MS, 
                                            "ms", "Component Performance");
        } else if (name.find("Configuration") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_value, PRDTargets::CONFIGURATION_LOADING_MS, 
                                            "ms", "Component Performance");
        } else if (name.find("ErrorDetection") != std::string::npos) {
            double measured_us = run.real_accumulated_time / 1e3; // Convert to microseconds
            g_analyzer.AnalyzeBenchmarkResult(name, measured_us, PRDTargets::ERROR_DETECTION_US, 
                                            "μs", "Component Performance");
        } else if (name.find("ErrorRecovery") != std::string::npos) {
            g_analyzer.AnalyzeBenchmarkResult(name, measured_value, PRDTargets::ERROR_RECOVERY_MS, 
                                            "ms", "Component Performance");
        }
    }
};

} // namespace Benchmarks

/**
 * Custom main function with enhanced reporting
 */
int main(int argc, char** argv) {
    // Initialize Google Benchmark
    benchmark::Initialize(&argc, argv);
    
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    std::cout << "Sudachi Multiplayer Performance Benchmark Suite" << std::endl;
    std::cout << "Testing against PRD Section 7.1 Performance Requirements" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // Set up custom reporter
    auto custom_reporter = std::make_unique<Benchmarks::PRDValidationReporter>();
    benchmark::RunSpecifiedBenchmarks(custom_reporter.get());
    
    // Print detailed performance analysis
    Benchmarks::g_analyzer.PrintSummaryReport();
    
    // Export results to JSON
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string json_filename = "sudachi_multiplayer_benchmark_results_" + timestamp + ".json";
    Benchmarks::g_analyzer.ExportToJSON(json_filename);
    
    benchmark::Shutdown();
    
    return 0;
}