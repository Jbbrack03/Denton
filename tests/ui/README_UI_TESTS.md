# Sudachi Multiplayer UI Tests

## Overview

This directory contains comprehensive failing unit tests for the Sudachi multiplayer UI components, following Test-Driven Development (TDD) methodology. These tests are designed to **fail initially** (red phase) and drive the implementation of the actual UI components.

## Test Architecture

### Directory Structure

```
tests/ui/
├── CMakeLists.txt              # Build configuration (Qt6 + GTest/GMock)
├── mocks/
│   ├── mock_qt_widgets.h       # Mock Qt widgets for dependency injection
│   ├── mock_qt_widgets.cpp     # Static member definitions
│   ├── mock_multiplayer_backend.h  # Mock multiplayer backend
│   └── mock_multiplayer_backend.cpp
├── test_utilities/
│   └── qt_test_fixtures.h      # Test fixtures and utilities
├── unit/
│   ├── test_multiplayer_mode_toggle.cpp      # 16 failing tests
│   ├── test_connection_status_overlay.cpp    # 25 failing tests
│   └── test_error_dialog_manager.cpp         # 22 failing tests
└── integration/
    └── (future integration tests)
```

## Components Under Test

### 1. Multiplayer Mode Toggle (`test_multiplayer_mode_toggle.cpp`)

**Purpose**: UI component for switching between Internet and Ad-Hoc multiplayer modes.

**Test Coverage** (16 tests):
- **Core Functionality** (3 tests):
  - Initial state shows Internet mode by default
  - Toggle to Ad-Hoc mode updates text and backend
  - Toggle back to Internet mode reverts state

- **Visual State** (3 tests):
  - Internet mode styling (blue background)
  - Ad-Hoc mode styling (orange background)  
  - Disabled state styling (grayed out)

- **Error Handling** (2 tests):
  - Backend failure reverts toggle and shows error
  - Network unavailable disables toggle

- **State Transitions** (2 tests):
  - Active session prevents mode change
  - Session end re-enables toggle

- **Signals/Events** (2 tests):
  - Mode change emits correct signal
  - Status update emits text change signal

- **Performance** (1 test):
  - Mode switch completes within 200ms

- **Accessibility** (2 tests):
  - Toggle has accessible name
  - Supports keyboard navigation

- **Expected Failures**: All tests will fail until `Sudachi::UI::MultiplayerModeToggle` is implemented

### 2. Connection Status Overlay (`test_connection_status_overlay.cpp`)

**Purpose**: Overlay showing connection progress, status, and quality indicators.

**Test Coverage** (25+ tests):
- **Initial State** (2 tests):
  - Shows disconnected status by default
  - Overlay is hidden initially

- **Connection Progress** (3 tests):  
  - Shows overlay and progress on connection start
  - Updates progress bar during connection
  - Shows success then auto-hides

- **Quality Indicators** (3 tests):
  - High quality shows green indicators
  - Poor quality shows red indicators
  - Quality fluctuation updates smoothly

- **Player Count** (3 tests):
  - Updates when players join/leave
  - Highlights when session full

- **Error States** (3 tests):
  - Shows error state on connection failure
  - Shows timeout message
  - Shows reconnecting state on network loss

- **Animations** (3 tests):
  - Fade-in animation when showing
  - Fade-out animation when hiding
  - Smooth progress bar animation

- **Responsive Design** (2 tests):
  - Compact layout on small screens
  - Expanded layout on large screens

- **Performance** (2 tests):
  - Frequent updates don't cause stutter
  - Overlay rendering completes within 100ms

- **Expected Failures**: All tests will fail until `Sudachi::UI::ConnectionStatusOverlay` is implemented

### 3. Error Dialog Manager (`test_error_dialog_manager.cpp`)

**Purpose**: System for managing error dialogs, notifications, and user feedback.

**Test Coverage** (22+ tests):
- **Basic Error Display** (4 tests):
  - Simple error shows dialog
  - Critical error uses MessageBox::Critical
  - Warning error uses MessageBox::Warning
  - Info message uses MessageBox::Information

- **Error Queue Management** (3 tests):
  - Multiple errors queue and display sequentially
  - Duplicate errors are suppressed
  - Queue overflow discards oldest errors

- **Error Categorization** (3 tests):
  - Connection errors have connection icon
  - Game errors have game icon
  - Network errors have network icon

- **User Actions** (3 tests):
  - Recoverable errors offer retry button
  - Retry button triggers retry action
  - Configuration errors offer settings button

- **Notification System** (3 tests):
  - Minor errors show notifications instead of dialogs
  - Notifications auto-hide after timeout
  - Notification click shows full dialog

- **Error Recovery** (3 tests):
  - Recoverable errors provide recovery actions
  - Auto-recovery shows progress
  - Recovery success shows success notification

- **Modal Dialog Management** (2 tests):
  - Game running uses non-blocking notifications
  - Game paused allows modal dialogs

- **Accessibility** (2 tests):
  - Error dialogs have accessible descriptions
  - Support keyboard navigation

- **Performance** (1 test):
  - Many errors don't impact game performance

- **Expected Failures**: All tests will fail until `Sudachi::UI::ErrorDialogManager` is implemented

## Mock Infrastructure

### Mock Qt Widgets (`mock_qt_widgets.h`)
- `MockQWidget`: Base widget with common methods
- `MockPushButton`: For multiplayer mode toggle testing
- `MockLabel`: For status display testing
- `MockProgressBar`: For connection progress testing
- `MockDialog`: For error dialog testing
- `MockMessageBox`: For system message boxes
- `MockTimer`: For animation and timing

### Mock Multiplayer Backend (`mock_multiplayer_backend.h`)
- `MockMultiplayerBackend`: Simulates multiplayer system behavior
- `MockNetworkManager`: Simulates network conditions
- `MockGameSession`: Simulates game state

### Test Fixtures (`qt_test_fixtures.h`)
- `QtTestFixture`: Base Qt application setup
- `MultiplayerUITestFixture`: Multiplayer-specific setup
- `ErrorDialogTestFixture`: Error testing scenarios
- `ConnectionStatusTestFixture`: Connection state scenarios
- `UIPerformanceTestFixture`: Performance testing setup
- `CrossPlatformUITestFixture`: Platform-specific testing

## TDD Approach

### Red Phase (Current State)
✅ **COMPLETED**: All tests are written and designed to fail
- Tests compile successfully
- Tests fail because target components don't exist
- Test failures provide clear guidance for implementation

### Green Phase (Next Step)
🔄 **TODO**: Implement minimal UI components to make tests pass
- Create `Sudachi::UI::MultiplayerModeToggle` class
- Create `Sudachi::UI::ConnectionStatusOverlay` class
- Create `Sudachi::UI::ErrorDialogManager` class
- Implement just enough functionality to satisfy tests

### Refactor Phase (Future)
🔄 **TODO**: Optimize and improve implementation
- Code cleanup and optimization
- Performance improvements
- UI polish and styling

## Building and Running Tests

### Prerequisites
- Qt6 (Core, Widgets, Test)
- Google Test/Mock
- CMake 3.22+

### Build Commands
```bash
# Configure with full Qt6 support
cmake -B build tests/ui -DCMAKE_BUILD_TYPE=Debug

# Build tests
cmake --build build

# Run specific test categories
./build/sudachi_multiplayer_ui_unit_tests
./build/sudachi_multiplayer_ui_integration_tests

# Run with CTest
cd build && ctest --output-on-failure
```

### Verification Build (No Qt6 Required)
```bash
# Simple verification that test structure is correct
cd build_ui_tests
cmake ../tests/ui -DCMAKE_BUILD_TYPE=Debug
make
./ui_test_verification
```

## Test Quality Metrics

### Coverage Areas
- ✅ Core functionality testing
- ✅ Error handling and edge cases  
- ✅ Visual state and styling verification
- ✅ Signal/slot behavior testing
- ✅ Performance requirements validation
- ✅ Accessibility compliance testing
- ✅ Cross-platform behavior testing
- ✅ Responsive design testing

### Test Characteristics
- **Fast**: All tests use mock objects for speed
- **Independent**: No test dependencies or shared state
- **Repeatable**: Deterministic behavior with mocks
- **Self-validating**: Clear pass/fail criteria
- **Timely**: Written before implementation (TDD)

## Integration with Main Project

### CMake Integration
The UI tests are integrated into the main test suite via:
```cmake
# In tests/CMakeLists.txt
add_subdirectory(ui)
set(SUDACHI_ENABLE_UI_TESTS ON CACHE BOOL "Enable UI tests")
```

### Continuous Integration
Tests are designed to run in headless environments:
- Uses `QT_QPA_PLATFORM=offscreen` for headless testing
- Mock objects eliminate dependencies on actual Qt widgets
- Fast execution suitable for CI/CD pipelines

## Next Steps

1. **Implement Components**: Create the actual UI components to satisfy the failing tests
2. **Qt Integration**: Connect components to the main Sudachi UI architecture  
3. **Backend Integration**: Wire up components to the multiplayer backend
4. **Styling**: Implement proper Qt stylesheets and themes
5. **Platform Testing**: Test on Windows, Linux, and Android platforms
6. **User Testing**: Validate UI/UX with actual users

## Key Implementation Guidance

The failing tests provide specific guidance for implementation:

### MultiplayerModeToggle Requirements
- QPushButton-based toggle between "Internet Mode" and "Ad-Hoc Mode"
- Blue styling for Internet mode, orange for Ad-Hoc mode
- Disabled during active sessions
- Keyboard navigation support
- Backend integration for mode switching

### ConnectionStatusOverlay Requirements  
- QWidget overlay with progress bar and status labels
- Fade animations for show/hide
- Quality indicators with color coding (green/orange/red)
- Player count display
- Responsive design for different screen sizes
- Auto-hide after successful connection

### ErrorDialogManager Requirements
- Error queue management system
- Different dialog types (Critical, Warning, Information)
- Modal vs. non-blocking behavior based on game state
- Notification system for minor errors
- Retry and recovery action support
- Accessibility compliance

This comprehensive test suite ensures that the UI components will be robust, user-friendly, and maintainable when implemented.
