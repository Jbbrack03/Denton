# WebSocket Room Client Tests - TDD Red Phase

This directory contains comprehensive failing tests for the WebSocket Room Client implementation, following Test-Driven Development (TDD) red phase principles.

## Test Structure

The test suite is organized into four main test files, each focusing on specific aspects of the RoomClient functionality:

### 1. test_room_client_connection.cpp
**Connection Lifecycle Management Tests**
- WebSocket connection establishment and teardown
- Authentication token handling
- Connection timeout management
- Connection state tracking and callbacks
- Heartbeat mechanism testing
- Multiple connection attempt handling

**Key Test Cases:**
- `ConstructionInitializesCorrectly` - Verifies proper initialization
- `ConnectEstablishesConnectionSuccessfully` - Tests basic connection flow
- `ConnectHandlesAuthenticationFailure` - Error handling for invalid auth
- `ConnectHandlesConnectionTimeout` - Timeout behavior verification
- `HeartbeatMaintainsConnection` - Heartbeat mechanism testing

### 2. test_room_client_messages.cpp
**JSON Message Parsing and Serialization Tests**
- All WebSocket message types from the server architecture
- JSON serialization and deserialization
- Message validation and error handling
- Message handler dispatch system

**Supported Message Types:**
- **RegisterRequest/Response** - Client registration with server
- **CreateRoomRequest/Response** - Room creation flow
- **RoomListRequest/Response** - Room discovery and listing
- **JoinRoomRequest/Response** - Room joining with player info
- **P2PInfoMessage** - P2P connection negotiation data
- **UseProxyMessage** - Relay server allocation
- **ErrorMessage** - Server error responses
- **PlayerJoined/Left** - Room participant updates

**Key Test Cases:**
- Message serialization for all request types
- Message deserialization for all response types
- Invalid JSON handling
- Unknown message type handling
- Message size limits and timestamp validation

### 3. test_room_client_reconnection.cpp
**Automatic Reconnection with Exponential Backoff Tests**
- Automatic reconnection trigger on connection loss
- Exponential backoff delay calculation
- Maximum reconnection attempts handling
- Manual reconnection when auto-reconnect disabled
- Session state preservation across reconnections

**Key Test Cases:**
- `AutoReconnectionTriggersOnConnectionLoss` - Auto-reconnect triggering
- `ExponentialBackoffCalculation` - Backoff algorithm verification
- `SuccessfulReconnectionAfterNetworkFailure` - Recovery testing
- `ReconnectionGivesUpAfterMaxAttempts` - Max attempts handling
- `ReconnectionPreservesSessionState` - State preservation

### 4. test_room_client_thread_safety.cpp
**Concurrency and Thread Safety Tests**
- Concurrent message sending from multiple threads
- Thread-safe connection state management
- Message queue thread safety under high load
- Callback thread safety
- Memory safety during concurrent destruction

**Key Test Cases:**
- `ConcurrentMessageSendingFromMultipleThreads` - Multi-thread message sending
- `ThreadSafeConnectionStateManagement` - State consistency across threads
- `MessageQueueThreadSafetyUnderHighLoad` - Queue stress testing
- `CallbackThreadSafety` - Safe callback invocation
- `DeadlockDetectionAndPrevention` - Deadlock avoidance

## Test Framework and Dependencies

### Testing Framework
- **Google Test (GTest)** - Unit testing framework
- **Google Mock (GMock)** - Mocking framework for dependencies
- **nlohmann/json** - JSON parsing and serialization

### Mock Interfaces
The tests use comprehensive mock interfaces defined in `mock_interfaces.h`:
- `MockWebSocketInterface` - WebSocket connection abstraction
- `MockConfigInterface` - Configuration provider abstraction

## Expected Test Results (Red Phase)

All tests are designed to **FAIL** initially since the RoomClient implementation doesn't exist yet. This follows TDD red phase principles:

### Expected Compilation Errors:
```cpp
// These types don't exist yet:
RoomClient client(...);                    // Type 'RoomClient' not found
ConnectionState::Connected                 // Enum 'ConnectionState' not found
RegisterRequest request;                   // Type 'RegisterRequest' not found
MessageSerializer::Serialize(...)         // Class 'MessageSerializer' not found
```

### Expected Runtime Failures:
- All test assertions will fail due to missing implementation
- Mock expectations will not be met
- Method calls will result in compilation errors

## Test Quality Assurance

### FIRST Principles Compliance:
- **Fast**: Tests use mocks and avoid real network I/O
- **Independent**: Each test can run in isolation
- **Repeatable**: Deterministic results with no external dependencies
- **Self-validating**: Clear pass/fail criteria with descriptive assertions
- **Timely**: Written before implementation (red phase)

### Test Coverage:
- **Unit Tests**: Individual method and function testing
- **Integration Tests**: Component interaction testing
- **Concurrency Tests**: Thread safety and race condition testing
- **Error Tests**: Exception and error condition handling
- **Performance Tests**: Basic performance characteristics

## Building and Running Tests

### Prerequisites:
```bash
# Install dependencies (example for Ubuntu/Debian)
sudo apt-get install libgtest-dev libgmock-dev nlohmann-json3-dev

# Or using vcpkg (as configured in the project)
vcpkg install gtest nlohmann-json
```

### Build Commands:
```bash
mkdir build && cd build
cmake .. -DSUDACHI_USE_BUNDLED_VCPKG=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target room_client_connection_tests
cmake --build . --target room_client_messages_tests  
cmake --build . --target room_client_reconnection_tests
cmake --build . --target room_client_thread_safety_tests
```

### Run Tests:
```bash
# Individual test suites
./room_client_connection_tests
./room_client_messages_tests
./room_client_reconnection_tests  
./room_client_thread_safety_tests

# All tests combined
./room_client_all_tests

# Using CTest
ctest --output-on-failure
```

## Implementation Guidance

When implementing the RoomClient, these tests provide clear specifications for:

1. **Public Interface**: Method signatures and expected behavior
2. **Error Handling**: Required error codes and exception types
3. **Thread Safety**: Synchronization requirements
4. **Performance**: Basic performance expectations
5. **Configuration**: Required configuration parameters

## Test Maintenance

As the implementation progresses:
1. Tests should gradually change from failing to passing
2. Mock expectations may need adjustment based on actual implementation
3. Performance tests may need tuning based on real performance characteristics
4. Additional edge cases may be discovered and should be added

The goal is to achieve 100% test pass rate when the RoomClient implementation is complete, ensuring all specified behavior is correctly implemented.
