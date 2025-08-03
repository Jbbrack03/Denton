# Sudachi Multiplayer UI/UX Specifications

## Overview

This document provides detailed UI/UX specifications for the Sudachi multiplayer implementation. Since the system operates transparently through game interfaces, the UI elements are minimal but critical for user understanding and control.

## Settings Interface

### Multiplayer Mode Toggle

**Location**: Emulation → Configure → System → Network

**Visual Design**:
```
┌─────────────────────────────────────────────┐
│ Network Settings                            │
├─────────────────────────────────────────────┤
│                                             │
│ Multiplayer Mode:                           │
│ ┌───────────────────────────────────────┐   │
│ │ ● Internet Multiplayer (Default)      │   │
│ │   Play with others online             │   │
│ └───────────────────────────────────────┘   │
│ ┌───────────────────────────────────────┐   │
│ │ ○ Offline Ad-Hoc Multiplayer         │   │
│ │   Local wireless play (no internet)   │   │
│ └───────────────────────────────────────┘   │
│                                             │
│ [Advanced Settings...]                      │
│                                             │
└─────────────────────────────────────────────┘
```

**Behavior**:
- Radio button selection
- Immediate effect (no restart required)
- Tooltip on hover explaining each mode
- Warning dialog when switching modes during active session

### Advanced Settings

**Expandable Section Content**:
```
Username: [________________] 
Room Server: [wss://rooms.sudachi.org     ] [Test Connection]
Enable Diagnostics: □
Connection Quality Display: ☑
Auto-reconnect: ☑ (Attempts: [5])
Preferred Region: [Auto-detect ▼]
```

## In-Game Status Indicators

### Connection Status Overlay

**Position**: Top-right corner (configurable)
**Size**: 200x50 pixels
**Opacity**: 80% (user adjustable)

**States**:
```
┌─────────────────────┐
│ 🟢 Connected (P2P)  │  <- Green, direct connection
│ Players: 3/4        │
└─────────────────────┘

┌─────────────────────┐
│ 🟡 Connected (Relay)│  <- Yellow, using relay
│ Players: 2/2        │
└─────────────────────┘

┌─────────────────────┐
│ 🔴 Disconnected     │  <- Red, connection lost
│ Reconnecting...     │
└─────────────────────┘

┌─────────────────────┐
│ 🟠 Ad-Hoc Mode      │  <- Orange, local wireless
│ Range: Good         │
└─────────────────────┘
```

### Network Quality Indicator

**Visual Design**:
```
Latency: ▁▂▃▄ 25ms    <- Bar graph showing latency
Packet Loss: 0.0%
```

**Color Coding**:
- Green: < 50ms latency, 0% loss
- Yellow: 50-150ms latency, < 1% loss  
- Red: > 150ms latency, > 1% loss

## Permission Dialogs

### Android Wi-Fi Permission

**Initial Request**:
```
┌────────────────────────────────────────┐
│ Enable Local Multiplayer?              │
├────────────────────────────────────────┤
│ Sudachi needs permission to:           │
│                                        │
│ • Access nearby devices                │
│ • Create Wi-Fi Direct connections      │
│ • View your location (required by      │
│   Android for Wi-Fi Direct)            │
│                                        │
│ Your location is only used to enable   │
│ local wireless features and is never   │
│ shared or stored.                      │
│                                        │
│ [Not Now]              [Grant Access]  │
└────────────────────────────────────────┘
```

### Windows Admin Elevation

**UAC Prompt Preparation**:
```
┌────────────────────────────────────────┐
│ Administrator Access Required          │
├────────────────────────────────────────┤
│ To use offline multiplayer, Sudachi    │
│ needs to:                              │
│                                        │
│ • Create a mobile hotspot              │
│ • Modify network settings              │
│                                        │
│ Windows will ask for administrator     │
│ permission in the next dialog.         │
│                                        │
│ [Cancel]           [Continue]          │
└────────────────────────────────────────┘
```

## Error Notifications

### Toast Notifications (3 seconds)
- Position: Bottom-center
- Animation: Slide up, fade out
- Examples:
  - "Connecting to room server..."
  - "Player joined the session"
  - "Switched to relay connection"

### Banner Warnings (Persistent until dismissed)
- Position: Top of game window
- Color: Yellow background, dark text
- Examples:
  - "Poor network connection detected"
  - "Host is using an older version"
  - "NAT type may affect connectivity"

### Error Dialogs (Modal)
```
┌────────────────────────────────────────┐
│ ⚠️ Connection Failed                   │
├────────────────────────────────────────┤
│ Unable to join the game session.       │
│                                        │
│ Possible reasons:                      │
│ • The room is full                     │
│ • Version mismatch                     │
│ • Network configuration issue          │
│                                        │
│ Error code: NET_002                    │
│                                        │
│ [Help]    [Try Again]    [Cancel]     │
└────────────────────────────────────────┘
```

## Platform-Specific UI Elements

### Android Connection Accept Dialog

**System-Level Prompt Override**:
```
┌────────────────────────────────────────┐
│ Incoming Player Connection             │
├────────────────────────────────────────┤
│ "Player123" wants to join your game    │
│                                        │
│ ⚠️ You'll see an Android system       │
│ prompt next. Please tap "Accept" to    │
│ allow them to connect.                 │
│                                        │
│ This is required by Android for        │
│ security reasons.                      │
│                                        │
│                    [Got it]            │
└────────────────────────────────────────┘
```

### Progress Indicators

**Connection Progress**:
```
┌────────────────────────────────────────┐
│ Connecting to Game Session...          │
├────────────────────────────────────────┤
│                                        │
│ [████████████░░░░░░░] 60%             │
│                                        │
│ Finding host...                        │
│ ✓ Room server connected                │
│ ✓ Host found                          │
│ • Establishing P2P connection...       │
│                                        │
│                    [Cancel]            │
└────────────────────────────────────────┘
```

## Accessibility Features

### Screen Reader Support
- All UI elements have descriptive labels
- Connection status announced on change
- Error messages read automatically

### Keyboard Navigation
- Tab order follows logical flow
- Enter/Space activate buttons
- Escape cancels dialogs

### High Contrast Mode
- Respects system high contrast settings
- Ensures 4.5:1 contrast ratio minimum
- Clear focus indicators

## Animation Guidelines

### Transitions
- Duration: 200-300ms
- Easing: ease-in-out
- No animation option available

### Loading Indicators
- Spinning circle for indeterminate progress
- Progress bar for determinate operations
- Pulse animation for "searching" states

## Responsive Design

### Window Scaling
- UI scales with window DPI
- Minimum supported: 1280x720
- Maximum tested: 4K displays

### Text Scaling
- Respects system font size
- Minimum: 12pt
- Maximum: 24pt
- Line height: 1.5x font size

## Platform Integration

### Windows
- Follows Windows 11 Fluent Design
- Uses system accent color
- Native context menus

### Android
- Material Design 3 compliance
- Bottom sheets for options
- Gesture navigation support

### Linux
- GTK theme integration
- Respects desktop environment
- Native file dialogs

## Testing Checklist

- [ ] All states reachable via keyboard
- [ ] Screen reader announces all changes
- [ ] Error messages are clear and actionable
- [ ] Animations can be disabled
- [ ] Works at 100%, 150%, 200% scaling
- [ ] Platform-specific guidelines followed
- [ ] Color blind friendly (no red/green only)
- [ ] All text is localizable