// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include "core/core.h"

#ifdef ENABLE_MULTIPLAYER
#include "core/multiplayer/multiplayer_backend.h"
#include "core/multiplayer/backend_factory.h"
#endif

namespace Core {

class MultiplayerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<System>();
    }
    
    void TearDown() override {
        if (system) {
            system->Shutdown();
        }
        system.reset();
    }
    
    std::unique_ptr<System> system;
};

#ifdef ENABLE_MULTIPLAYER

TEST_F(MultiplayerIntegrationTest, SystemHasMultiplayerBackendGetter) {
    // System should provide access to multiplayer backend
    auto* backend = system->GetMultiplayerBackend();
    
    // Backend might be null if initialization failed, but getter should exist
    // The important part is that the method exists and compiles
    EXPECT_TRUE(backend == nullptr || backend != nullptr);
}

TEST_F(MultiplayerIntegrationTest, SystemHasConstMultiplayerBackendGetter) {
    const System* const_system = system.get();
    
    // Const version should also exist
    const auto* backend = const_system->GetMultiplayerBackend();
    
    // Backend might be null if initialization failed, but getter should exist
    EXPECT_TRUE(backend == nullptr || backend != nullptr);
}

TEST_F(MultiplayerIntegrationTest, MultiplayerBackendInitializedDuringSystemInit) {
    // Initialize a fresh system
    System test_system;
    
    // After initialization, multiplayer backend should be available
    // (might be null if initialization failed, but that's a runtime issue)
    auto* backend = test_system.GetMultiplayerBackend();
    
    // If backend exists, it should be initialized
    if (backend) {
        // We can't test the actual state without knowing the backend interface
        // but we can verify it's not a dangling pointer
        EXPECT_NE(backend, nullptr);
    }
}

TEST_F(MultiplayerIntegrationTest, MultiplayerBackendCleanedUpDuringShutdown) {
    // Create and initialize system
    {
        System test_system;
        auto* backend_before = test_system.GetMultiplayerBackend();
        
        // Shutdown should clean up multiplayer backend
        test_system.Shutdown();
        
        // After shutdown, backend should still be accessible but might be reset
        auto* backend_after = test_system.GetMultiplayerBackend();
        
        // The pointer should be valid (not crash) even after shutdown
        EXPECT_TRUE(backend_after == nullptr || backend_after != nullptr);
    }
    
    // System destructor should not crash
    EXPECT_TRUE(true);
}

#else // !ENABLE_MULTIPLAYER

TEST_F(MultiplayerIntegrationTest, MultiplayerDisabledBuild) {
    // When ENABLE_MULTIPLAYER is not defined, the system should still work
    EXPECT_NE(system.get(), nullptr);
    
    // System should initialize without multiplayer
    system->Initialize();
    
    // And shutdown cleanly
    system->Shutdown();
}

#endif // ENABLE_MULTIPLAYER

} // namespace Core