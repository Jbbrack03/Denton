# Legacy Multiplayer Code Removal Plan

## Overview

This document identifies all legacy multiplayer code from Yuzu/Citra that must be removed before implementing the new LDN-based multiplayer system.

## Files to Remove

### 1. Network Infrastructure (`src/network/`)

**Complete Directory Removal:**
```
src/network/
├── CMakeLists.txt
├── announce_multiplayer_session.cpp
├── announce_multiplayer_session.h
├── network.cpp
├── network.h
├── packet.cpp
├── packet.h
├── precompiled_headers.h
├── room.cpp
├── room.h
├── room_member.cpp
├── room_member.h
├── verify_user.cpp
└── verify_user.h
```

**Rationale:** This entire module implements a room-based multiplayer system designed for Citra (3DS emulation). It's incompatible with the Switch's LDN protocol and our dual-mode architecture.

### 2. Qt Multiplayer UI (`src/sudachi/multiplayer/`)

**Complete Directory Removal:**
```
src/sudachi/multiplayer/
├── chat_room.cpp
├── chat_room.h
├── chat_room.ui
├── client_room.cpp
├── client_room.h
├── client_room.ui
├── direct_connect.cpp
├── direct_connect.h
├── direct_connect.ui
├── host_room.cpp
├── host_room.h
├── host_room.ui
├── lobby.cpp
├── lobby.h
├── lobby.ui
├── lobby_p.h
├── message.cpp
├── message.h
├── moderation_dialog.cpp
├── moderation_dialog.h
├── moderation_dialog.ui
├── state.cpp
├── state.h
└── validation.h
```

**Rationale:** These UI components are designed for room-based multiplayer with chat, moderation, and lobby features. Our LDN implementation will be transparent to games and won't require these UI elements.

### 3. LDN Service Implementation (`src/core/hle/service/ldn/`)

**Files to Rewrite (not remove):**
```
src/core/hle/service/ldn/
├── lan_discovery.cpp
├── lan_discovery.h
├── ldn.cpp
├── ldn.h
├── ldn_results.h
├── ldn_types.h
├── monitor_service.cpp
├── monitor_service.h
├── sf_monitor_service.cpp
├── sf_monitor_service.h
├── sf_service.cpp
├── sf_service.h
├── sf_service_monitor.cpp
├── sf_service_monitor.h
├── system_local_communication_service.cpp
├── system_local_communication_service.h
├── user_local_communication_service.cpp
└── user_local_communication_service.h
```

**Rationale:** These files contain the HLE implementation of nn::ldn service. While the interface definitions (ldn_types.h, ldn_results.h) can be preserved, the implementation must be completely rewritten to support our dual-mode architecture.

### 4. Related Components to Modify

#### 4.1 Common Headers (`src/common/`)
- `announce_multiplayer_room.h` - Remove completely

#### 4.2 Core Components
- Check for any references to `Network::RoomNetwork` in:
  - `src/core/core.cpp`
  - `src/core/core.h`

#### 4.3 CMake Build Files
- Remove network module from `src/CMakeLists.txt`
- Remove multiplayer UI from `src/sudachi/CMakeLists.txt`
- Update any other CMake files that reference these modules

#### 4.4 Qt Main Window
- Remove multiplayer menu items and actions from:
  - `src/sudachi/main.cpp`
  - `src/sudachi/main.h`
  - `src/sudachi/main.ui`

## Removal Strategy

### Phase 1: UI Removal
1. Remove `src/sudachi/multiplayer/` directory
2. Remove multiplayer menu items from main window
3. Remove any multiplayer-related settings from configuration

### Phase 2: Network Module Removal
1. Remove `src/network/` directory
2. Remove `announce_multiplayer_room.h` from common
3. Update CMake files to exclude network module

### Phase 3: LDN Service Preparation
1. Back up existing LDN type definitions
2. Clear implementation files while preserving interfaces
3. Prepare for new dual-mode implementation

## Verification Steps

After removal:
1. Ensure project builds without errors
2. Verify no orphaned includes remain
3. Check for any remaining references to removed components
4. Run existing single-player functionality tests

## Git Commands for Removal

```bash
# Create a new branch for the removal
git checkout -b remove-legacy-multiplayer

# Remove directories
git rm -r src/network/
git rm -r src/sudachi/multiplayer/
git rm src/common/announce_multiplayer_room.h

# Commit the removal
git commit -m "Remove legacy Yuzu/Citra multiplayer implementation

- Remove room-based network infrastructure
- Remove multiplayer UI components
- Prepare for new LDN-based implementation"
```

## Next Steps

After removing legacy code:
1. Set up new project structure for dual-mode multiplayer
2. Integrate required dependencies (cpp-libp2p, mjansson/mdns)
3. Begin implementing Model A (Internet) and Model B (Ad-hoc) backends
4. Create new HLE module for nn::ldn service
