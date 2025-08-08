// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/core.h"
#include <gtest/gtest.h>

#ifdef ENABLE_MULTIPLAYER
#include "core/multiplayer/backend_factory.h"
#include "core/multiplayer/multiplayer_backend.h"
#endif

namespace Core {

class MultiplayerIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override { system = std::make_unique<System>(); }

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
  auto *backend = system->GetMultiplayerBackend();

  // Backend should be created when multiplayer support is enabled
  ASSERT_NE(backend, nullptr);

  // Newly created systems should have an initialized backend
  EXPECT_TRUE(backend->IsInitialized());
}

TEST_F(MultiplayerIntegrationTest, SystemHasConstMultiplayerBackendGetter) {
  const System *const_system = system.get();

  // Const version should also exist
  const auto *backend = const_system->GetMultiplayerBackend();

  // Backend should exist and be initialized in const contexts as well
  ASSERT_NE(backend, nullptr);
  EXPECT_TRUE(backend->IsInitialized());
}

TEST_F(MultiplayerIntegrationTest,
       MultiplayerBackendInitializedDuringSystemInit) {
  // Initialize a fresh system
  System test_system;

  // After initialization, multiplayer backend should be available
  auto *backend = test_system.GetMultiplayerBackend();

  ASSERT_NE(backend, nullptr);
  EXPECT_TRUE(backend->IsInitialized());
}

TEST_F(MultiplayerIntegrationTest, MultiplayerBackendCleanedUpDuringShutdown) {
  // Create and initialize system
  {
    System test_system;
    auto *backend_before = test_system.GetMultiplayerBackend();
    ASSERT_NE(backend_before, nullptr);
    EXPECT_TRUE(backend_before->IsInitialized());

    // Shutdown should clean up multiplayer backend
    test_system.Shutdown();

    // After shutdown, backend should still be accessible but should no longer
    // be initialized
    auto *backend_after = test_system.GetMultiplayerBackend();
    ASSERT_NE(backend_after, nullptr);
    EXPECT_FALSE(backend_after->IsInitialized());
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
