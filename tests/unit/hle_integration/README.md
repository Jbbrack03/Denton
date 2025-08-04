# HLE Integration Test Suite - Phase 6.1 Critical Bridge

## Overview

This test suite implements **Phase 6.1 of the Implementation Plan** - the most critical missing piece that bridges the Nintendo Switch nn::ldn HLE interface to our new dual-mode multiplayer system.

## Current Situation (Critical Gap)

- ✅ **Legacy LDN service exists** in `sudachi/src/core/hle/service/ldn/`
- ✅ **New multiplayer system exists** in `src/core/multiplayer/`
- ❌ **NO bridge exists between them** (critical gap)
- ❌ **Games cannot access multiplayer** without this bridge

## Test Files Created (TDD Red Phase)

### 1. `test_multiplayer_backend_interface.cpp`
**Purpose**: Tests the unified `MultiplayerBackend` interface that both Model A and B must implement.

**Key Components**:
- `MultiplayerBackend` abstract interface with all required methods
- `MockMultiplayerBackend` for testing
- Tests covering initialization, network management, discovery, data transmission
- **WILL FAIL**: Backend implementations don't exist yet

**Critical Methods**:
```cpp
virtual ErrorCode Initialize() = 0;
virtual ErrorCode CreateNetwork(const Service::LDN::CreateNetworkConfig& config) = 0;
virtual ErrorCode Scan(std::vector<Service::LDN::NetworkInfo>& out_networks, 
                      const Service::LDN::ScanFilter& filter) = 0;
virtual ErrorCode Connect(const Service::LDN::ConnectNetworkData& connect_data,
                         const Service::LDN::NetworkInfo& network_info) = 0;
```

### 2. `test_backend_factory.cpp`
**Purpose**: Tests the factory that selects appropriate backend (Model A vs B) based on configuration.

**Key Components**:
- `ConfigurationManager` interface for multiplayer mode selection
- `ConcreteBackendFactory` that creates backend instances
- Platform-specific backend selection logic
- **WILL FAIL**: Backend classes don't exist yet

**Selection Logic**:
- Internet Mode → Model A Backend
- Ad-Hoc Mode → Model B Backend  
- Auto Mode → Prefer Model B, fallback to Model A

### 3. `test_ldn_service_bridge.cpp`
**Purpose**: Tests the critical bridge that connects LDN HLE service to multiplayer backends.

**Key Components**:
- `LdnServiceBridge` interface that replaces LANDiscovery usage
- `ConcreteLdnServiceBridge` implementation with state machine
- Proper state transitions (None → Initialized → AccessPoint/Station states)
- **WILL FAIL**: Backend implementations don't exist yet

**Critical Integration Point**:
```cpp
// CURRENT (in user_local_communication_service.cpp):
LANDiscovery lan_discovery;

// MUST BECOME:
std::unique_ptr<LdnServiceBridge> ldn_bridge;

// All calls like lan_discovery.CreateNetwork() must become:
// ldn_bridge->CreateNetwork()
```

### 4. `test_error_code_mapper.cpp`
**Purpose**: Tests mapping between multiplayer error codes and LDN result codes.

**Key Components**:
- `ErrorCodeMapper` interface for bidirectional error translation
- Comprehensive mapping tables (40+ error mappings)
- Recoverability checking and retry delay calculation
- Human-readable error descriptions

**Example Mappings**:
- `ErrorCode::ConnectionFailed` → `Service::LDN::ResultConnectionFailed`
- `ErrorCode::RoomFull` → `Service::LDN::ResultMaximumNodeCount`
- `ErrorCode::MessageTooLarge` → `Service::LDN::ResultAdvertiseDataTooLarge`

### 5. `test_type_translator.cpp`
**Purpose**: Tests conversion between LDN types and internal multiplayer types.

**Key Components**:
- `TypeTranslator` interface for bidirectional type conversion
- Internal multiplayer type definitions (InternalNetworkInfo, etc.)
- Validation methods for both type systems
- Data size and format handling

**Critical Conversions**:
- `Service::LDN::NetworkInfo` ↔ `InternalNetworkInfo`
- `Service::LDN::NodeInfo` ↔ `InternalNodeInfo`
- `Service::LDN::CreateNetworkConfig` ↔ `InternalSessionInfo`

## Test Strategy (TDD Red Phase)

### Why These Tests MUST FAIL Initially

1. **True Red Phase**: Tests are written before implementation exists
2. **Implementation Guidance**: Test failures show exactly what needs to be built
3. **Interface Definition**: Tests define the exact interfaces that must be implemented
4. **Integration Points**: Tests demonstrate how components connect together

### Expected Failures

```bash
# These commands will FAIL until implementation exists:
cd build && make hle_integration_tests
./hle_integration_tests

# Expected output:
# [  FAILED  ] MultiplayerBackendInterfaceTest.InitializeSuccess
# [  FAILED  ] BackendFactoryTest.CreateModelABackend  
# [  FAILED  ] LdnServiceBridgeTest.InitializeSuccess
# [  FAILED  ] ErrorCodeMapperTest.MapSuccessCode
# [  FAILED  ] TypeTranslatorTest.ConvertInternalToLdnNetworkInfo
```

### When Tests Will Pass

Tests will pass once these implementations are created:

1. **Model A Backend**: `src/core/multiplayer/model_a/model_a_backend.cpp`
2. **Model B Backend**: `src/core/multiplayer/model_b/model_b_backend.cpp`
3. **LDN Service Bridge**: `src/core/multiplayer/hle_integration/ldn_service_bridge.cpp`
4. **Error Code Mapper**: `src/core/multiplayer/hle_integration/error_code_mapper.cpp`
5. **Type Translator**: `src/core/multiplayer/hle_integration/type_translator.cpp`

## Integration Requirements

### File Updates Required

1. **`user_local_communication_service.cpp`**:
   - Replace `LANDiscovery lan_discovery;` with `std::unique_ptr<LdnServiceBridge> ldn_bridge;`
   - Update all method calls to use bridge instead of LANDiscovery

2. **New Implementation Files** (must be created):
   ```
   src/core/multiplayer/hle_integration/
   ├── multiplayer_backend.h
   ├── backend_factory.h
   ├── backend_factory.cpp
   ├── ldn_service_bridge.h
   ├── ldn_service_bridge.cpp
   ├── error_code_mapper.h
   ├── error_code_mapper.cpp
   ├── type_translator.h
   └── type_translator.cpp
   ```

3. **Backend Implementations**:
   - `src/core/multiplayer/model_a/model_a_backend.cpp`
   - `src/core/multiplayer/model_b/model_b_backend.cpp`

## Test Execution

### Building Tests
```bash
cd sudachi/build
cmake .. -DSUDACHI_TESTS=ON
make hle_integration_tests
```

### Running Specific Test Categories
```bash
# Test backend interface
make test_backend_interface

# Test backend factory
make test_backend_factory

# Test LDN bridge
make test_ldn_bridge

# Test error mapping
make test_error_mapping

# Test type translation
make test_type_translation

# Run all HLE integration tests
make test_hle_integration
```

### Demonstrating Expected Failures
```bash
# Show that tests fail as expected (red phase)
make demonstrate_test_failures
```

## Next Steps

1. **Implement Backend Interfaces**: Create the abstract `MultiplayerBackend` interface
2. **Implement Backend Factory**: Create configuration management and backend selection
3. **Implement LDN Service Bridge**: Create the critical bridge component
4. **Implement Error Mapping**: Create comprehensive error translation
5. **Implement Type Translation**: Create bidirectional type conversion
6. **Update LDN Service**: Replace LANDiscovery with LdnServiceBridge
7. **Run Tests**: Verify all tests pass (green phase)

## Critical Success Criteria

- ✅ **Tests fail initially** (demonstrates true TDD red phase)
- ✅ **Interfaces are well-defined** (tests specify exact method signatures)
- ✅ **Integration points are clear** (tests show how components connect)
- ✅ **Error handling is comprehensive** (tests cover all error scenarios)
- ❌ **Implementations don't exist yet** (this is the gap to fill)

This test suite provides the roadmap for implementing the most critical missing piece of the multiplayer system - the bridge that allows Nintendo Switch games to use our new dual-mode multiplayer backends.