# Sudachi Multiplayer Performance Benchmark Suite

## Overview

This benchmark suite provides comprehensive performance validation for the Sudachi multiplayer system against the requirements specified in PRD Section 7.1. The benchmarks are designed to be measurable, repeatable, and provide clear pass/fail criteria.

## Performance Targets (PRD Section 7.1)

### Connection Establishment
- **Initial Connection**: < 3 seconds (Model A with good network)
- **P2P Negotiation**: < 5 seconds (including fallback decision)
- **Ad-hoc Discovery**: < 2 seconds (local network scan)
- **Reconnection**: < 1 second (after disconnect)

### Latency Requirements
- **Local Ad-hoc (Model B)**: < 5ms additional overhead
- **P2P Direct (Model A)**: < 20ms additional overhead
- **Relay Server (Model A)**: < 50ms additional overhead
- **Maximum Jitter**: < 10ms for stable connections

### Throughput Requirements
- **Packet Frequency**: Up to 60Hz for real-time games
- **Frame Budget**: 16.67ms per frame processing
- **Minimum Bandwidth**: 256 Kbps per player
- **Recommended Bandwidth**: 1 Mbps per player
- **Maximum Packet Size**: 1400 bytes (MTU safe)

### Scalability Targets
- **Room Server**: 100,000 concurrent connections per instance
- **Relay Server**: 10,000 concurrent sessions per instance
- **Maximum Players**: 8 players per session
- **Room List Query**: < 100ms response time

## Benchmark Categories

### 1. Connection Establishment Benchmarks
**File**: `benchmark_connection_establishment.cpp`

Tests connection setup performance for different networking modes:
- Room server connection (WebSocket)
- P2P connection with relay fallback
- mDNS service discovery
- Wi-Fi Direct connection (Android)
- Reconnection after disconnect
- Performance under adverse network conditions

### 2. Latency Measurement Benchmarks  
**File**: `benchmark_latency_measurement.cpp`

Measures round-trip time and processing latency:
- Ad-hoc network latency (< 5ms target)
- P2P direct connection latency (< 20ms target)
- Relay server latency (< 50ms target)
- Packet processing pipeline latency
- 60Hz packet processing capability
- Jitter analysis and network stability
- Latency under concurrent load

### 3. Packet Processing Benchmarks
**File**: `benchmark_packet_processing.cpp`

Tests packet handling and throughput:
- Binary packet serialization/deserialization
- CRC32 validation performance (>100MB/s target)
- 60Hz frame processing (16.67ms budget)
- Sustained packet processing
- Throughput at various frequencies
- Bandwidth utilization testing
- Multi-threaded packet processing

### 4. Scalability and Memory Benchmarks
**File**: `benchmark_scalability_memory.cpp`

Validates system scalability and resource usage:
- Room server connection scalability (up to 100k connections)
- Relay server session scalability (up to 10k sessions)
- Multi-player session broadcast performance
- Room list query performance (< 100ms target)
- Memory usage under load
- Memory leak detection
- Thread safety under concurrent access

### 5. Component Performance Benchmarks
**File**: `benchmark_component_performance.cpp`

Tests individual component performance:
- HLE Interface (nn::ldn service calls)
- Configuration system (loading, validation, serialization)
- Error handling (detection and recovery)
- Platform-specific components (Windows Mobile Hotspot, Android Wi-Fi Direct)
- Cross-component integration

## Usage

### Building the Benchmarks

```bash
# From the build directory
cmake --build . --target sudachi_multiplayer_benchmarks

# Or with make
make sudachi_multiplayer_benchmarks
```

### Running Benchmarks

```bash
# Run all benchmarks
./sudachi_multiplayer_benchmarks

# Run specific benchmark category
./sudachi_multiplayer_benchmarks --benchmark_filter="ConnectionEstablishment.*"

# Run with specific iterations
./sudachi_multiplayer_benchmarks --benchmark_repetitions=5

# Generate JSON report
./sudachi_multiplayer_benchmarks --benchmark_format=json --benchmark_out=results.json

# Run with time limit
./sudachi_multiplayer_benchmarks --benchmark_max_time=300  # 5 minutes max
```

### Benchmark Filters

Filter benchmarks by category:
```bash
--benchmark_filter="ConnectionEstablishment.*"  # Connection benchmarks
--benchmark_filter="Latency.*"                  # Latency benchmarks  
--benchmark_filter="PacketProcessing.*"         # Packet processing benchmarks
--benchmark_filter="Scalability.*"              # Scalability benchmarks
--benchmark_filter="Component.*"                # Component benchmarks
```

Filter by specific tests:
```bash
--benchmark_filter=".*P2P.*"                    # All P2P related tests
--benchmark_filter=".*60Hz.*"                   # 60Hz processing tests
--benchmark_filter=".*AdHoc.*"                  # Ad-hoc networking tests
```

## Output and Reporting

### Console Output
The benchmark suite provides detailed console output including:
- Individual benchmark results with timing
- PRD target validation (PASS/FAIL)
- Statistical analysis (mean, percentiles, standard deviation)
- Performance deviation from targets
- Category summaries
- Overall performance report

### JSON Export
Results are automatically exported to JSON format with:
- Detailed timing measurements
- Target validation results
- Statistical analysis
- Performance metadata
- Compliance status

### Example Output
```
Sudachi Multiplayer Performance Benchmark Suite
Testing against PRD Section 7.1 Performance Requirements
================================================================================

Running ./sudachi_multiplayer_benchmarks
Run on (8 X 3200 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)

Benchmark                                     Time             CPU   Iterations
--------------------------------------------------------------------------------
ConnectionEstablishment/RoomServer/GoodNetwork    2.15 ms      2.12 ms         10   [PASS]
ConnectionEstablishment/P2P/WithRelayFallback     4.23 ms      4.20 ms         10   [PASS]
Latency/AdHoc/RoundTripTime                       3.2 ms       3.1 ms        1000   [PASS]
PacketProcessing/60Hz/FrameProcessing            12.4 ms      12.1 ms        1000   [PASS]

================================================================================
OVERALL PERFORMANCE SUMMARY
================================================================================
Total Performance Tests: 47
Tests Passed: 45
Tests Failed: 2
Overall Pass Rate: 95.7%

PRD Compliance Status: ❌ FAIL - 2 performance targets not met
```

## Mock Infrastructure

The benchmarks use a comprehensive mock infrastructure to provide:
- **Controlled Testing Environment**: Predictable network conditions and timing
- **Dependency Injection**: Configurable mock components for isolated testing
- **Network Simulation**: Various network conditions (good, mobile, poor, local)
- **Resource Tracking**: Memory usage and allocation monitoring
- **Thread Safety Testing**: Concurrent access simulation

### Key Mock Components
- `MockWebSocketClient`: WebSocket connection simulation
- `MockP2PNetwork`: P2P networking simulation
- `MockRelayClient`: Relay server simulation
- `MockAdHocNetwork`: Local ad-hoc networking
- `MockPacketProcessor`: Packet handling simulation
- `MockHLEInterface`: nn::ldn service simulation

## Extending the Benchmarks

### Adding New Benchmarks

1. **Create the benchmark function**:
```cpp
BENCHMARK(MyNewBenchmark) {
    for (auto _ : state) {
        // Benchmark code here
        auto start = high_resolution_clock::now();
        
        // Code to measure
        MyFunction();
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        state.SetIterationTime(duration.count() / 1000000.0);
    }
    
    // Validate against targets
    state.counters["TargetMet"] = (measured_value <= target) ? 1 : 0;
}
```

2. **Register the benchmark**:
```cpp
BENCHMARK(MyNewBenchmark)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000)
    ->Name("Category/Subcategory/TestName");
```

3. **Add target validation** in `benchmark_main.cpp`

### Adding Mock Components

1. Create mock class in `benchmark_mocks.h`
2. Implement configurable behavior
3. Add factory function in `benchmark_utilities.h`
4. Use in benchmark fixtures

## CI/CD Integration

The benchmark suite integrates with CI/CD pipelines:

```yaml
# Example CI configuration
performance_tests:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v2
    - name: Build benchmarks
      run: |
        mkdir build && cd build
        cmake .. -DENABLE_BENCHMARKS=ON
        make sudachi_multiplayer_benchmarks
    - name: Run performance tests
      run: |
        cd build
        ./sudachi_multiplayer_benchmarks --benchmark_format=json --benchmark_out=results.json
    - name: Validate performance
      run: |
        # Check if any benchmarks failed
        if grep -q '"TargetMet": 0' results.json; then
          echo "Performance regression detected!"
          exit 1
        fi
```

## Performance Regression Detection

The benchmark suite supports automated performance regression detection:
- **Baseline Comparison**: Compare results against previous runs
- **Threshold Alerts**: Alert on performance degradation beyond thresholds
- **Trend Analysis**: Track performance trends over time
- **Automated Reporting**: Generate performance reports for each build

## Troubleshooting

### Common Issues

1. **Benchmark Timeouts**: Increase timeout with `--benchmark_max_time=<seconds>`
2. **Memory Issues**: Run with fewer iterations or smaller test data
3. **Platform Differences**: Results may vary between platforms
4. **Mock Behavior**: Verify mock configuration matches test expectations

### Debug Mode
Run benchmarks in debug mode for detailed output:
```bash
./sudachi_multiplayer_benchmarks --benchmark_enable_random_interleaving=true --v=1
```

## Contributing

When adding new benchmarks:
1. Follow the existing naming convention
2. Include clear PRD target validation
3. Add comprehensive documentation
4. Test on multiple platforms
5. Update this README with new benchmark descriptions

## Performance Analysis Tools

Additional tools for performance analysis:
- **Profiling**: Use with profilers like perf, Intel VTune, or Visual Studio Profiler
- **Memory Analysis**: Use with valgrind, AddressSanitizer, or similar tools
- **Network Analysis**: Use with tcpdump, Wireshark for network testing
- **Statistical Analysis**: Export JSON results for statistical analysis in R, Python

This benchmark suite ensures the Sudachi multiplayer system meets all performance requirements and maintains high quality as the codebase evolves.