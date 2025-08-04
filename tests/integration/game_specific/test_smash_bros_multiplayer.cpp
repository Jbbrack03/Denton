// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * Super Smash Bros. Ultimate Multiplayer Integration Tests
 * Tests high-frequency packet synchronization, frame-perfect inputs,
 * and competitive multiplayer scenarios.
 */

#include "utilities/game_test_base.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Sudachi::GameTesting;
using namespace testing;

class SmashBrosMultiplayerTest : public GameTestBase {
protected:
    void SetUp() override {
        GameTestBase::SetUp();
        
        // Initialize Smash Bros game info
        smash_bros_info_ = {
            .game_id = GameConstants::SMASH_BROS_ID,
            .title = "Super Smash Bros. Ultimate",
            .max_players = GameConstants::SMASH_BROS_MAX_PLAYERS,
            .supported_modes = {"local_wireless", "online_smash"},
            .expected_packet_rate_hz = GameConstants::SMASH_BROS_PACKET_RATE,
            .max_latency_ms = GameConstants::SMASH_BROS_MAX_LATENCY
        };
        
        InitializeGame(smash_bros_info_);
    }

    void SetupBattleArena(uint8_t fighter_count, const std::string& stage = "battlefield") {
        // Load battle arena save state
        LoadGameSaveState("test_data/smash_bros/battle_arena_" + stage + ".dat");
        
        SessionConfig config = {
            .player_count = fighter_count,
            .multiplayer_mode = "adhoc",
            .session_duration_ms = 180000, // 3 minutes (typical match)
            .enable_network_simulation = false,
            .artificial_latency_ms = 0,
            .packet_loss_rate = 0.0f
        };
        
        SetupMultiplayerSession(config);
    }

    void SetupOnlineMatch(uint8_t player_count, uint32_t target_latency_ms = 50) {
        LoadGameSaveState("test_data/smash_bros/online_match_setup.dat");
        
        SessionConfig config = {
            .player_count = player_count,
            .multiplayer_mode = "internet",
            .session_duration_ms = 420000, // 7 minutes (with countdowns)
            .enable_network_simulation = true,
            .artificial_latency_ms = target_latency_ms,
            .packet_loss_rate = 0.005f // 0.5% packet loss
        };
        
        SetupMultiplayerSession(config);
    }

    void SimulateFightingInputs(uint8_t player_id, uint32_t duration_ms) {
        const std::vector<std::string> fighting_moves = {
            "attack_a", "attack_b", "smash_up", "smash_down", "smash_side",
            "special_neutral", "special_up", "special_down", "special_side",
            "grab", "shield", "dodge", "jump", "dash", "walk"
        };
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count() < duration_ms) {
            
            // Random fighting input every 50ms (20Hz input rate)
            int move_index = rand() % fighting_moves.size();
            SendPlayerAction(player_id, fighting_moves[move_index]);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void SimulateIntenseBattle() {
        // All players perform intensive fighting inputs simultaneously
        std::vector<std::thread> input_threads;
        
        for (uint8_t i = 0; i < session_config_.player_count; ++i) {
            input_threads.emplace_back([this, i]() {
                SimulateFightingInputs(i, 30000); // 30 seconds of intense fighting
            });
        }
        
        // Wait for all input threads to complete
        for (auto& thread : input_threads) {
            thread.join();
        }
    }

    void VerifyFrameSync() {
        // Capture game states from all players
        for (uint8_t i = 0; i < session_config_.player_count; ++i) {
            CaptureGameState(i);
        }
        
        // Verify all players are on the same frame
        ASSERT_TRUE(VerifyGameStateSynchronization());
        
        // Verify frame numbers match within tolerance
        auto& states = captured_states_;
        for (size_t i = 1; i < states.size(); ++i) {
            uint64_t frame_diff = std::abs(static_cast<int64_t>(states[i].frame_number) - 
                                         static_cast<int64_t>(states[0].frame_number));
            EXPECT_LE(frame_diff, 2) << "Frame desync detected between players";
        }
    }

    GameInfo smash_bros_info_;
};

// High-frequency packet synchronization tests
TEST_F(SmashBrosMultiplayerTest, HighFrequencyPacketSync) {
    SCOPED_TRACE("Testing high-frequency packet synchronization (60Hz)");
    
    SetupBattleArena(2); // 1v1 match
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(2, 5000));
    
    // Simulate 10 seconds of gameplay
    SimulateGameplay(10000);
    
    // Verify 60Hz packet rate is maintained
    AssertPacketRate(GameConstants::SMASH_BROS_PACKET_RATE, 5); // 5% tolerance
    
    // Verify low latency is maintained
    AssertLatency(GameConstants::SMASH_BROS_MAX_LATENCY);
    
    auto metrics = GetTestMetrics();
    EXPECT_GT(metrics.packets_sent, 500); // ~60 packets/second * 10 seconds
    EXPECT_LT(metrics.average_latency.count(), 50); // Under 50ms average
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, FramePerfectInputSync) {
    SCOPED_TRACE("Testing frame-perfect input synchronization");
    
    SetupBattleArena(4); // 4-player free-for-all
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4, 10000));
    
    // Start with synchronized frame check
    VerifyFrameSync();
    
    // All players perform same input on same frame
    for (uint8_t i = 0; i < 4; ++i) {
        SendPlayerAction(i, "smash_attack_frame_perfect");
    }
    
    // Verify frame synchronization is maintained
    WaitForCondition([this]() {
        return metrics_.packets_sent > 10;
    }, 1000);
    
    VerifyFrameSync();
    AssertNoDesyncEvents();
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, IntenseCombatSynchronization) {
    SCOPED_TRACE("Testing synchronization during intense combat");
    
    SetupBattleArena(8); // Maximum players
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(8, 15000));
    
    // Simulate intense battle with all players
    SimulateIntenseBattle();
    
    // Verify synchronization held up under intense load
    AssertGameStateSync();
    AssertNoDesyncEvents();
    
    auto metrics = GetTestMetrics();
    EXPECT_GT(metrics.packets_sent, 3000); // High packet count from intense fighting
    EXPECT_LT(metrics.desync_events, 1); // No desyncs allowed
    
    StopMultiplayerSession();
}

// Network condition stress tests
TEST_F(SmashBrosMultiplayerTest, LowLatencyRequirement) {
    SCOPED_TRACE("Testing strict low-latency requirement for competitive play");
    
    SetupOnlineMatch(2, 30); // 30ms target latency
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(2, 8000));
    
    SimulateIntenseBattle();
    
    // Strict latency requirements for competitive play
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.average_latency.count(), 50);
    EXPECT_LT(metrics.max_latency.count(), 100);
    
    // Verify jitter is minimal
    EXPECT_LT(metrics.jitter_ms, 10);
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, PacketLossImpact) {
    SCOPED_TRACE("Testing impact of packet loss on gameplay");
    
    SetupBattleArena(4);
    
    // Simulate problematic network conditions
    SimulatePacketLoss(0.02f); // 2% packet loss
    SimulateNetworkJitter(15); // 15ms jitter
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4, 12000));
    
    SimulateIntenseBattle();
    
    // Verify system handles packet loss gracefully
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.desync_events, 2); // Allow minimal desyncs
    EXPECT_GT(metrics.packet_loss_rate, 0.015f); // Verify packet loss was simulated
    
    // Input responsiveness should still be acceptable
    EXPECT_LT(metrics.average_latency.count(), 150);
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, ConnectionStabilityUnderLoad) {
    SCOPED_TRACE("Testing connection stability under high load");
    
    SetupBattleArena(6);
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(6, 12000));
    
    // Extended intense battle session
    for (int round = 0; round < 3; ++round) {
        SCOPED_TRACE("Round " + std::to_string(round + 1));
        
        SimulateIntenseBattle();
        
        // Brief pause between rounds
        SimulateGameplay(2000);
        
        // Verify connection stability throughout
        AssertPlayerCount(6);
        AssertPacketRate(GameConstants::SMASH_BROS_PACKET_RATE, 10);
    }
    
    auto metrics = GetTestMetrics();
    EXPECT_EQ(metrics.connection_drops, 0);
    EXPECT_LT(metrics.desync_events, 3); // Minimal desyncs over extended play
    
    StopMultiplayerSession();
}

// Game-specific functionality tests
TEST_F(SmashBrosMultiplayerTest, FinalSmashSynchronization) {
    SCOPED_TRACE("Testing Final Smash attack synchronization");
    
    SetupBattleArena(4);
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4));
    
    // Simulate Final Smash conditions
    SendPlayerAction(0, "break_smash_ball");
    SendPlayerAction(0, "final_smash_mario");
    
    // Verify Final Smash is synchronized across all players
    WaitForCondition([this]() {
        return VerifyPlayerActions({"final_smash_mario"});
    }, 3000);
    
    // Continue battle during Final Smash
    SimulateFightingInputs(1, 5000);
    SimulateFightingInputs(2, 5000);
    SimulateFightingInputs(3, 5000);
    
    AssertGameStateSync();
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, StageHazardSynchronization) {
    SCOPED_TRACE("Testing stage hazard synchronization");
    
    SetupBattleArena(4, "kalos_pokemon_league"); // Stage with hazards
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4));
    
    // Simulate gameplay until stage hazard activates
    SimulateGameplay(30000); // 30 seconds
    
    // Verify stage hazards are synchronized
    AssertGameStateSync();
    
    // Continue fighting during hazard
    SimulateIntenseBattle();
    
    // Verify no desyncs during hazard events
    AssertNoDesyncEvents();
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, ItemSpawnSynchronization) {
    SCOPED_TRACE("Testing item spawn synchronization");
    
    SetupBattleArena(4);
    
    // Enable items for this test
    SendPlayerAction(0, "enable_items:all");
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4));
    
    // Play until items spawn
    SimulateGameplay(20000);
    
    // Players grab items
    SendPlayerAction(0, "grab_item:home_run_bat");
    SendPlayerAction(1, "grab_item:pokeball");
    
    // Verify item states are synchronized
    WaitForCondition([this]() {
        return VerifyGameStateSynchronization();
    }, 3000);
    
    AssertGameStateSync();
    
    StopMultiplayerSession();
}

// Performance and scalability tests
TEST_F(SmashBrosMultiplayerTest, MaxPlayersPerformance) {
    SCOPED_TRACE("Testing performance with maximum players (8)");
    
    SetupBattleArena(8);
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(8, 20000));
    
    // Measure baseline performance
    auto start_time = std::chrono::steady_clock::now();
    
    SimulateIntenseBattle();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify performance doesn't degrade significantly with max players
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.average_latency.count(), 100);
    EXPECT_GT(metrics.packets_sent / (duration.count() / 1000.0), 50); // At least 50 packets/sec
    
    AssertGameStateSync();
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, ExtendedTournamentMatch) {
    SCOPED_TRACE("Testing extended tournament-style match");
    
    SetupBattleArena(2); // Tournament 1v1
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(2));
    
    // Simulate best-of-5 tournament match (5 rounds)
    for (int game = 0; game < 5; ++game) {
        SCOPED_TRACE("Game " + std::to_string(game + 1));
        
        // Reset to neutral positions
        SendPlayerAction(0, "reset_position");
        SendPlayerAction(1, "reset_position");
        
        // Intense fighting for each game
        SimulateIntenseBattle();
        
        // Brief pause between games
        SimulateGameplay(3000);
        
        // Verify consistency throughout tournament
        AssertPacketRate(GameConstants::SMASH_BROS_PACKET_RATE, 8);
        AssertLatency(GameConstants::SMASH_BROS_MAX_LATENCY);
    }
    
    auto metrics = GetTestMetrics();
    EXPECT_EQ(metrics.connection_drops, 0);
    EXPECT_LT(metrics.desync_events, 2);
    
    StopMultiplayerSession();
}

// Mode-specific tests
TEST_F(SmashBrosMultiplayerTest, ModelAOnlineSmash) {
    SCOPED_TRACE("Testing Model A (Internet) online smash mode");
    
    SetupOnlineMatch(4, 80); // Realistic internet latency
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4, 15000));
    
    SimulateIntenseBattle();
    
    // Verify online mode handles internet latency appropriately
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.average_latency.count(), 150); // Allow higher latency for internet
    EXPECT_EQ(metrics.desync_events, 0);
    
    // Verify rollback/prediction systems work
    EXPECT_TRUE(VerifyPlayerActions({"rollback_prediction", "input_delay_compensation"}));
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, ModelBLocalWireless) {
    SCOPED_TRACE("Testing Model B (Ad-Hoc) local wireless mode");
    
    SetupBattleArena(8); // Local wireless with max players
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(8, 12000));
    
    SimulateIntenseBattle();
    
    // Verify ad-hoc provides optimal performance
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.average_latency.count(), 30); // Very low latency for local
    EXPECT_EQ(metrics.desync_events, 0);
    
    // Verify direct peer-to-peer communication
    EXPECT_GT(metrics.direct_connections, 0);
    
    StopMultiplayerSession();
}

// Error recovery tests
TEST_F(SmashBrosMultiplayerTest, MidMatchRecovery) {
    SCOPED_TRACE("Testing recovery from mid-match errors");
    
    SetupBattleArena(4);
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(4));
    
    // Start intense match
    std::thread battle_thread([this]() {
        SimulateIntenseBattle();
    });
    
    // Simulate network interruption mid-match
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    SimulateConnectionDrop(2, 2000); // Drop player 2 for 2 seconds
    
    battle_thread.join();
    
    // Verify match continued and recovered
    EXPECT_TRUE(WaitForPlayerCount(4, 8000));
    AssertGameStateSync();
    
    // Continue match after recovery
    SimulateGameplay(5000);
    AssertNoDesyncEvents();
    
    StopMultiplayerSession();
}

TEST_F(SmashBrosMultiplayerTest, InputLagCompensation) {
    SCOPED_TRACE("Testing input lag compensation mechanisms");
    
    SetupOnlineMatch(2, 120); // Higher latency scenario
    
    StartMultiplayerSession();
    ASSERT_TRUE(WaitForPlayerCount(2));
    
    // Send rapid inputs to test compensation
    for (int i = 0; i < 20; ++i) {
        SendPlayerAction(0, "rapid_attack_" + std::to_string(i));
        SendPlayerAction(1, "rapid_counter_" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps timing
    }
    
    // Verify inputs are properly compensated
    WaitForCondition([this]() {
        return metrics_.packets_sent > 40;
    }, 5000);
    
    AssertGameStateSync();
    
    auto metrics = GetTestMetrics();
    EXPECT_LT(metrics.input_lag_compensation_ms, 100);
    
    StopMultiplayerSession();
}