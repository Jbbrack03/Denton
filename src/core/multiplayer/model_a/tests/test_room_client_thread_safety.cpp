// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <future>
#include <random>

#include "core/multiplayer/model_a/room_client.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Thread-safe mock WebSocket connection for concurrency testing
 */
class ThreadSafeMockWebSocketConnection {
public:
    MOCK_METHOD(void, Connect, (const std::string& uri), ());
    MOCK_METHOD(void, Disconnect, (const std::string& reason), ());
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(void, SendMessage, (const std::string& message), ());
    MOCK_METHOD(void, SetOnMessageCallback, (std::function<void(const std::string&)> callback), ());
    
    // Thread-safe message tracking
    void RecordSentMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        sent_messages_.push_back(message);
        message_sent_cv_.notify_all();
    }
    
    std::vector<std::string> GetSentMessages() const {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        return sent_messages_;
    }
    
    void WaitForMessageCount(size_t count, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(messages_mutex_);
        message_sent_cv_.wait_for(lock, timeout, [this, count] {
            return sent_messages_.size() >= count;
        });
    }
    
private:
    mutable std::mutex messages_mutex_;
    std::vector<std::string> sent_messages_;
    std::condition_variable message_sent_cv_;
};

/**
 * Mock configuration for thread safety testing
 */
class MockThreadSafeConfig {
public:
    MOCK_METHOD(std::string, GetRoomServerUrl, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetMessageTimeout, (), (const));
    MOCK_METHOD(int, GetMaxConcurrentMessages, (), (const));
    MOCK_METHOD(size_t, GetMessageQueueSize, (), (const));
};

/**
 * Thread-safe test barrier for synchronizing test threads
 */
class TestBarrier {
public:
    explicit TestBarrier(int count) : count_(count), waiting_(0) {}
    
    void Wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_;
        if (waiting_ == count_) {
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this] { return waiting_ == count_; });
        }
    }
    
private:
    const int count_;
    int waiting_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // anonymous namespace

/**
 * Test fixture for RoomClient thread safety and concurrency tests
 * Tests concurrent access, message queuing, and thread synchronization
 */
class RoomClientThreadSafetyTest : public Test {
protected:
    void SetUp() override {
        mock_connection_ = std::make_shared<ThreadSafeMockWebSocketConnection>();
        mock_config_ = std::make_shared<MockThreadSafeConfig>();
        
        // Set up default configuration
        ON_CALL(*mock_config_, GetMessageTimeout())
            .WillByDefault(Return(std::chrono::milliseconds(5000)));
        ON_CALL(*mock_config_, GetMaxConcurrentMessages()).WillByDefault(Return(10));
        ON_CALL(*mock_config_, GetMessageQueueSize()).WillByDefault(Return(100));
        
        // Set up mock connection behavior
        ON_CALL(*mock_connection_, IsConnected()).WillByDefault(Return(true));
        ON_CALL(*mock_connection_, SendMessage(_))
            .WillByDefault([this](const std::string& message) {
                mock_connection_->RecordSentMessage(message);
            });
    }

    std::shared_ptr<ThreadSafeMockWebSocketConnection> mock_connection_;
    std::shared_ptr<MockThreadSafeConfig> mock_config_;
};

/**
 * Test: Concurrent message sending from multiple threads
 * Verifies that multiple threads can safely send messages simultaneously
 */
TEST_F(RoomClientThreadSafetyTest, ConcurrentMessageSendingFromMultipleThreads) {
    // This test will fail because thread-safe message sending doesn't exist yet
    
    // ARRANGE
    const int num_threads = 10;
    const int messages_per_thread = 20;
    const int total_messages = num_threads * messages_per_thread;
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    auto barrier = std::make_shared<TestBarrier>(num_threads);
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_sends{0};
    std::atomic<int> failed_sends{0};
    
    // ACT - Launch multiple threads sending messages concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            barrier->Wait(); // Synchronize thread start
            
            for (int j = 0; j < messages_per_thread; ++j) {
                std::string message = "Thread_" + std::to_string(thread_id) + "_Msg_" + std::to_string(j);
                
                auto result = client->SendMessage(message);
                if (result == ErrorCode::Success) {
                    successful_sends++;
                } else {
                    failed_sends++;
                }
                
                // Small random delay to increase concurrency stress
                std::this_thread::sleep_for(std::chrono::microseconds(rand() % 10));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for all messages to be processed
    mock_connection_->WaitForMessageCount(total_messages, std::chrono::seconds(5));
    
    // ASSERT
    EXPECT_EQ(successful_sends.load(), total_messages);
    EXPECT_EQ(failed_sends.load(), 0);
    
    auto sent_messages = mock_connection_->GetSentMessages();
    EXPECT_EQ(sent_messages.size(), total_messages);
    
    // Verify no message corruption or duplication
    std::set<std::string> unique_messages(sent_messages.begin(), sent_messages.end());
    EXPECT_EQ(unique_messages.size(), total_messages);
}

/**
 * Test: Thread-safe connection state management
 * Verifies that connection state changes are handled safely across threads
 */
TEST_F(RoomClientThreadSafetyTest, ThreadSafeConnectionStateManagement) {
    // This test will fail because thread-safe state management doesn't exist yet
    
    // ARRANGE
    const int num_threads = 5;
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> connect_attempts{0};
    std::atomic<int> disconnect_attempts{0};
    std::atomic<int> state_queries{0};
    
    // ACT - Multiple threads performing connection operations
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < 50; ++j) {
                switch (j % 3) {
                    case 0:
                        client->Connect();
                        connect_attempts++;
                        break;
                    case 1:
                        client->Disconnect();
                        disconnect_attempts++;
                        break;
                    case 2:
                        auto state = client->GetConnectionState();
                        auto connected = client->IsConnected();
                        state_queries++;
                        // Verify consistency between state and connected flag
                        if (state == ConnectionState::Connected) {
                            EXPECT_TRUE(connected);
                        } else {
                            EXPECT_FALSE(connected);
                        }
                        break;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // ASSERT - No crashes and operations completed
    EXPECT_GT(connect_attempts.load(), 0);
    EXPECT_GT(disconnect_attempts.load(), 0);
    EXPECT_GT(state_queries.load(), 0);
    
    // Final state should be consistent
    auto final_state = client->GetConnectionState();
    auto final_connected = client->IsConnected();
    if (final_state == ConnectionState::Connected) {
        EXPECT_TRUE(final_connected);
    } else {
        EXPECT_FALSE(final_connected);
    }
}

/**
 * Test: Message queue thread safety under high load
 * Verifies that the internal message queue handles concurrent access properly
 */
TEST_F(RoomClientThreadSafetyTest, MessageQueueThreadSafetyUnderHighLoad) {
    // This test will fail because thread-safe message queue doesn't exist yet
    
    // ARRANGE
    const int producer_threads = 5;
    const int consumer_threads = 3;
    const int messages_per_producer = 100;
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    std::atomic<bool> stop_consumers{false};
    std::atomic<int> messages_produced{0};
    std::atomic<int> messages_consumed{0};
    
    std::vector<std::thread> threads;
    
    // Launch consumer threads (message processing)
    for (int i = 0; i < consumer_threads; ++i) {
        threads.emplace_back([&]() {
            while (!stop_consumers.load()) {
                if (client->ProcessPendingMessages()) {
                    messages_consumed++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Launch producer threads (message queueing)
    for (int i = 0; i < producer_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < messages_per_producer; ++j) {
                std::string message = "Producer_" + std::to_string(thread_id) + "_" + std::to_string(j);
                
                if (client->QueueMessage(message) == ErrorCode::Success) {
                    messages_produced++;
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(rand() % 5));
            }
        });
    }
    
    // Wait for producers to finish
    for (int i = consumer_threads; i < threads.size(); ++i) {
        threads[i].join();
    }
    
    // Allow consumers to process remaining messages
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_consumers = true;
    
    // Wait for consumers to finish
    for (int i = 0; i < consumer_threads; ++i) {
        threads[i].join();
    }
    
    // ASSERT
    const int expected_messages = producer_threads * messages_per_producer;
    EXPECT_EQ(messages_produced.load(), expected_messages);
    EXPECT_EQ(messages_consumed.load(), expected_messages);
    
    // Queue should be empty
    EXPECT_EQ(client->GetPendingMessageCount(), 0);
}

/**
 * Test: Callback thread safety
 * Verifies that callbacks are invoked safely from multiple threads
 */
TEST_F(RoomClientThreadSafetyTest, CallbackThreadSafety) {
    // This test will fail because thread-safe callbacks don't exist yet
    
    // ARRANGE
    const int num_callback_threads = 8;
    std::atomic<int> callback_invocations{0};
    std::atomic<int> concurrent_callbacks{0};
    std::atomic<int> max_concurrent{0};
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    client->SetOnMessageCallback([&](const std::string& message) {
        int current_concurrent = ++concurrent_callbacks;
        max_concurrent = std::max(max_concurrent.load(), current_concurrent);
        
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        callback_invocations++;
        concurrent_callbacks--;
    });
    
    std::vector<std::thread> threads;
    
    // ACT - Multiple threads triggering callbacks simultaneously
    for (int i = 0; i < num_callback_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < 20; ++j) {
                std::string message = "Callback_Thread_" + std::to_string(thread_id) + "_" + std::to_string(j);
                client->SimulateIncomingMessage(message);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for all callbacks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // ASSERT
    const int expected_callbacks = num_callback_threads * 20;
    EXPECT_EQ(callback_invocations.load(), expected_callbacks);
    EXPECT_EQ(concurrent_callbacks.load(), 0); // All callbacks should have completed
    EXPECT_GT(max_concurrent.load(), 1); // Should have had concurrent execution
}

/**
 * Test: Room state synchronization across threads
 * Verifies that room membership state is consistent across concurrent operations
 */
TEST_F(RoomClientThreadSafetyTest, RoomStateSynchronizationAcrossThreads) {
    // This test will fail because thread-safe room state doesn't exist yet
    
    // ARRANGE
    const int num_threads = 6;
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_joins{0};
    std::atomic<int> successful_leaves{0};
    std::atomic<int> state_inconsistencies{0};
    
    // ACT - Concurrent room operations
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < 30; ++j) {
                if (j % 2 == 0) {
                    // Attempt to join room
                    std::string room_id = "room_" + std::to_string(thread_id % 3);
                    if (client->JoinRoom(room_id) == ErrorCode::Success) {
                        successful_joins++;
                    }
                } else {
                    // Attempt to leave room
                    if (client->LeaveRoom() == ErrorCode::Success) {
                        successful_leaves++;
                    }
                }
                
                // Verify state consistency
                auto current_room = client->GetCurrentRoomId();
                auto is_in_room = client->IsInRoom();
                
                if (current_room.empty() && is_in_room) {
                    state_inconsistencies++;
                } else if (!current_room.empty() && !is_in_room) {
                    state_inconsistencies++;
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(rand() % 20));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // ASSERT
    EXPECT_GT(successful_joins.load(), 0);
    EXPECT_GT(successful_leaves.load(), 0);
    EXPECT_EQ(state_inconsistencies.load(), 0); // No state inconsistencies
    
    // Final state should be consistent
    auto final_room = client->GetCurrentRoomId();
    auto final_in_room = client->IsInRoom();
    
    if (final_room.empty()) {
        EXPECT_FALSE(final_in_room);
    } else {
        EXPECT_TRUE(final_in_room);
    }
}

/**
 * Test: Memory safety during concurrent destruction
 * Verifies that RoomClient can be safely destroyed while operations are ongoing
 */
TEST_F(RoomClientThreadSafetyTest, MemorySafetyDuringConcurrentDestruction) {
    // This test will fail because safe concurrent destruction doesn't exist yet
    
    // ARRANGE
    const int num_threads = 4;
    std::atomic<bool> keep_running{true};
    std::vector<std::thread> threads;
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Launch threads that continuously use the client
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            while (keep_running.load()) {
                try {
                    // Various operations that might be in progress during destruction
                    client->IsConnected();
                    client->SendMessage("Test message " + std::to_string(thread_id));
                    client->GetConnectionState();
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                } catch (...) {
                    // Expected during destruction - should not crash
                    break;
                }
            }
        });
    }
    
    // Let threads run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // ACT - Destroy client while threads are still running
    client.reset();
    keep_running = false;
    
    // Wait for threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    
    // ASSERT - Test passes if no crashes occurred
    SUCCEED();
}

/**
 * Test: Lock-free performance under contention
 * Verifies that the client performs adequately under high contention
 */
TEST_F(RoomClientThreadSafetyTest, LockFreePerformanceUnderContention) {
    // This test will fail because lock-free implementation doesn't exist yet
    
    // ARRANGE
    const int num_threads = std::thread::hardware_concurrency();
    const int operations_per_thread = 1000;
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> completed_operations{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // ACT - High contention scenario
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                // Mix of different operations to test various code paths
                switch (j % 4) {
                    case 0:
                        client->IsConnected();
                        break;
                    case 1:
                        client->GetConnectionState();
                        break;
                    case 2:
                        client->SendMessage("Perf test " + std::to_string(j));
                        break;
                    case 3:
                        client->GetCurrentRoomId();
                        break;
                }
                completed_operations++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // ASSERT
    const int total_operations = num_threads * operations_per_thread;
    EXPECT_EQ(completed_operations.load(), total_operations);
    
    // Performance requirement: Should complete within reasonable time
    // (This is somewhat arbitrary but ensures no excessive blocking)
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    double ops_per_second = total_operations / (duration.count() / 1000.0);
    EXPECT_GT(ops_per_second, 1000); // At least 1000 operations per second
}

/**
 * Test: Deadlock detection and prevention
 * Verifies that the client doesn't deadlock under complex scenarios
 */
TEST_F(RoomClientThreadSafetyTest, DeadlockDetectionAndPrevention) {
    // This test will fail because deadlock prevention doesn't exist yet
    
    // ARRANGE
    const int num_threads = 8;
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    std::vector<std::thread> threads;
    std::atomic<bool> deadlock_detected{false};
    std::atomic<int> completed_sequences{0};
    
    // Complex scenario that could cause deadlocks:
    // - Thread A: Connect -> Join Room -> Send Message -> Disconnect
    // - Thread B: Join Room -> Send Message -> Leave Room -> Connect
    // - Thread C: Send Message -> Get State -> Connect -> Join Room
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            
            try {
                switch (thread_id % 3) {
                    case 0: {
                        // Sequence A
                        client->Connect();
                        client->JoinRoom("test_room");
                        client->SendMessage("Message from sequence A");
                        client->Disconnect();
                        break;
                    }
                    case 1: {
                        // Sequence B
                        client->JoinRoom("test_room");
                        client->SendMessage("Message from sequence B");
                        client->LeaveRoom();
                        client->Connect();
                        break;
                    }
                    case 2: {
                        // Sequence C
                        client->SendMessage("Message from sequence C");
                        client->GetConnectionState();
                        client->Connect();
                        client->JoinRoom("test_room");
                        break;
                    }
                }
                
                if (std::chrono::steady_clock::now() > timeout) {
                    deadlock_detected = true;
                    return;
                }
                
                completed_sequences++;
                
            } catch (...) {
                // Exception is acceptable, deadlock is not
            }
        });
    }
    
    // Wait for completion with timeout
    auto start = std::chrono::steady_clock::now();
    for (auto& thread : threads) {
        thread.join();
    }
    auto duration = std::chrono::steady_clock::now() - start;
    
    // ASSERT
    EXPECT_FALSE(deadlock_detected.load());
    EXPECT_LT(duration, std::chrono::seconds(15)); // Should complete within 15 seconds
    EXPECT_GT(completed_sequences.load(), 0); // At least some operations should complete
}

/**
 * Test: Thread pool integration safety
 * Verifies safe integration with external thread pools and executors
 */
TEST_F(RoomClientThreadSafetyTest, ThreadPoolIntegrationSafety) {
    // This test will fail because thread pool integration doesn't exist yet
    
    // ARRANGE
    const int pool_size = 4;
    const int task_count = 100;
    
    auto client = std::make_unique<RoomClient>(mock_connection_, mock_config_);
    
    // Simulate thread pool with various task types
    std::vector<std::future<ErrorCode>> futures;
    std::atomic<int> successful_tasks{0};
    
    // ACT - Submit various tasks to simulated thread pool
    for (int i = 0; i < task_count; ++i) {
        auto future = std::async(std::launch::async, [&client, i]() -> ErrorCode {
            try {
                switch (i % 5) {
                    case 0:
                        return client->Connect();
                    case 1:
                        return client->SendMessage("Pool task " + std::to_string(i));
                    case 2:
                        client->GetConnectionState();
                        return ErrorCode::Success;
                    case 3:
                        return client->JoinRoom("pool_room");
                    case 4:
                        return client->LeaveRoom();
                }
                return ErrorCode::Success;
            } catch (...) {
                return ErrorCode::InternalError;
            }
        });
        
        futures.push_back(std::move(future));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        try {
            auto result = future.get();
            if (result == ErrorCode::Success) {
                successful_tasks++;
            }
        } catch (...) {
            // Task execution error - acceptable for this test
        }
    }
    
    // ASSERT
    EXPECT_GT(successful_tasks.load(), task_count / 2); // At least half should succeed
    
    // Client should still be in a valid state
    auto final_state = client->GetConnectionState();
    EXPECT_NE(final_state, ConnectionState::Failed);
}
