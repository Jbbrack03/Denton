// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "multiplayer_backend.h"

namespace Core::Multiplayer::HLE {

/**
 * Configuration Manager - Manages multiplayer mode selection
 */
class ConfigurationManager {
public:
    enum class MultiplayerMode {
        Internet,
        AdHoc,
        Auto  // Automatically select based on platform/availability
    };
    
    virtual ~ConfigurationManager() = default;
    virtual MultiplayerMode GetPreferredMode() = 0;
    virtual void SetPreferredMode(MultiplayerMode mode) = 0;
    virtual bool IsModelAAvailable() = 0;
    virtual bool IsModelBAvailable() = 0;
    virtual std::string GetConfigFilePath() = 0;
};

/**
 * Backend Factory Interface - Selects appropriate backend based on configuration
 */
class BackendFactory {
public:
    enum class BackendType {
        ModelA_Internet,
        ModelB_AdHoc
    };
    
    virtual ~BackendFactory() = default;
    virtual std::unique_ptr<MultiplayerBackend> CreateBackend(BackendType type) = 0;
    virtual BackendType GetPreferredBackend() = 0;
};

} // namespace Core::Multiplayer::HLE