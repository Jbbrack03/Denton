// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTimer>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mocks/mock_qt_widgets.h"
#include "mocks/mock_multiplayer_backend.h"

namespace Sudachi::Multiplayer::Testing {

/**
 * Base test fixture for Qt UI components
 * Provides common setup and teardown for UI tests
 */
class QtTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application if not already initialized
        if (!QApplication::instance()) {
            static int argc = 1;
            static char* argv[] = {const_cast<char*>("test"), nullptr};
            app = std::make_unique<QApplication>(argc, argv);
            app->setAttribute(Qt::AA_DisableWindowContextHelpButton);
        }

        // Reset mock state
        QtTestUtilities::resetTestState();
        
        // Set up test window
        test_window = std::make_unique<QMainWindow>();
        test_window->resize(800, 600);
        test_window->setWindowTitle("Sudachi Multiplayer UI Test");
        
        // Enable offscreen rendering for headless testing
        test_window->setAttribute(Qt::WA_DontShowOnScreen, true);
    }

    void TearDown() override {
        // Clean up test widgets
        test_widgets.clear();
        
        // Reset test window
        if (test_window) {
            test_window->close();
            test_window.reset();
        }
        
        // Process pending events
        if (QApplication::instance()) {
            QApplication::processEvents();
        }
    }

    /**
     * Create a test widget and add it to the test window
     */
    template<typename T, typename... Args>
    T* createTestWidget(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        T* widget_ptr = widget.get();
        
        test_widgets.push_back(std::move(widget));
        test_window->setCentralWidget(widget_ptr);
        
        return widget_ptr;
    }

    /**
     * Process Qt events to allow signals/slots to execute
     */
    void processEvents(int timeout_ms = 100) {
        QTimer timer;
        timer.setSingleShot(true);
        timer.start(timeout_ms);
        
        while (timer.isActive()) {
            QApplication::processEvents();
        }
    }

    /**
     * Wait for a condition to become true
     */
    bool waitForCondition(std::function<bool()> condition, int timeout_ms = 5000) {
        QTimer timeout_timer;
        timeout_timer.setSingleShot(true);
        timeout_timer.start(timeout_ms);
        
        while (!condition() && timeout_timer.isActive()) {
            QApplication::processEvents();
            QThread::msleep(10);
        }
        
        return condition();
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<QMainWindow> test_window;
    std::vector<std::unique_ptr<QWidget>> test_widgets;
};

/**
 * Test fixture specifically for multiplayer UI components
 */
class MultiplayerUITestFixture : public QtTestFixture {
protected:
    void SetUp() override {
        QtTestFixture::SetUp();
        
        // Create mock multiplayer backend
        mock_backend = std::make_unique<MockMultiplayerBackend>();
        mock_network_manager = std::make_unique<MockNetworkManager>();
        mock_game_session = std::make_unique<MockGameSession>();
        
        // Set up default mock behavior
        setupDefaultMockBehavior();
    }

    void TearDown() override {
        // Clean up mocks
        mock_backend.reset();
        mock_network_manager.reset();
        mock_game_session.reset();
        
        QtTestFixture::TearDown();
    }

    void setupDefaultMockBehavior() {
        // Default backend behavior
        ON_CALL(*mock_backend, IsInitialized())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*mock_backend, IsConnected())
            .WillByDefault(::testing::Return(false));
        ON_CALL(*mock_backend, IsInSession())
            .WillByDefault(::testing::Return(false));
        ON_CALL(*mock_backend, GetConnectionStatus())
            .WillByDefault(::testing::Return("Disconnected"));
        ON_CALL(*mock_backend, GetPlayerCount())
            .WillByDefault(::testing::Return(0));
        ON_CALL(*mock_backend, GetMaxPlayers())
            .WillByDefault(::testing::Return(8));
        ON_CALL(*mock_backend, GetNetworkMode())
            .WillByDefault(::testing::Return("Internet"));
        ON_CALL(*mock_backend, HasError())
            .WillByDefault(::testing::Return(false));
        
        // Default network manager behavior
        ON_CALL(*mock_network_manager, IsNetworkAvailable())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*mock_network_manager, IsWiFiConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*mock_network_manager, GetNetworkType())
            .WillByDefault(::testing::Return("WiFi"));
        ON_CALL(*mock_network_manager, GetSignalStrength())
            .WillByDefault(::testing::Return(75));
        
        // Default game session behavior
        ON_CALL(*mock_game_session, IsGameRunning())
            .WillByDefault(::testing::Return(false));
        ON_CALL(*mock_game_session, SupportsMultiplayer())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*mock_game_session, IsHosting())
            .WillByDefault(::testing::Return(false));
    }

    std::unique_ptr<MockMultiplayerBackend> mock_backend;
    std::unique_ptr<MockNetworkManager> mock_network_manager;
    std::unique_ptr<MockGameSession> mock_game_session;
};

/**
 * Test fixture for error dialog testing
 */
class ErrorDialogTestFixture : public MultiplayerUITestFixture {
protected:
    void SetUp() override {
        MultiplayerUITestFixture::SetUp();
        
        // Create mock error scenarios
        error_scenarios = {
            {"Connection Timeout", "Failed to connect to multiplayer server within 30 seconds."},
            {"Network Error", "Network connection lost. Please check your internet connection."},
            {"Session Full", "Cannot join session: maximum number of players reached."},
            {"Game Mismatch", "Cannot join session: different game version detected."},
            {"Authentication Failed", "Failed to authenticate with multiplayer service."},
            {"Invalid Session", "The requested session no longer exists."},
            {"Host Disconnected", "The host has disconnected from the session."},
            {"Peer Connection Failed", "Failed to establish direct connection with peers."}
        };
    }

    struct ErrorScenario {
        std::string title;
        std::string message;
    };

    std::vector<ErrorScenario> error_scenarios;
};

/**
 * Test fixture for connection status testing
 */
class ConnectionStatusTestFixture : public MultiplayerUITestFixture {
protected:
    void SetUp() override {
        MultiplayerUITestFixture::SetUp();
        
        // Define connection states for testing
        connection_states = {
            {"Disconnected", 0, "Not connected to multiplayer"},
            {"Connecting", 25, "Establishing connection..."},
            {"Connected", 100, "Connected to multiplayer server"},
            {"In Session", 100, "In multiplayer session"},
            {"Host", 100, "Hosting multiplayer session"},
            {"Error", 0, "Connection error occurred"}
        };
    }

    struct ConnectionState {
        std::string status;
        int progress;
        std::string description;
    };

    std::vector<ConnectionState> connection_states;
};

/**
 * Performance test fixture for UI responsiveness
 */
class UIPerformanceTestFixture : public MultiplayerUITestFixture {
protected:
    void SetUp() override {
        MultiplayerUITestFixture::SetUp();
        
        // Set up performance monitoring
        performance_timer = std::make_unique<QTimer>();
        performance_timer->setSingleShot(true);
    }

    void TearDown() override {
        performance_timer.reset();
        MultiplayerUITestFixture::TearDown();
    }

    /**
     * Measure time taken for a UI operation
     */
    template<typename Func>
    int measureOperationTime(Func&& operation) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        processEvents(); // Allow UI to update
        auto end = std::chrono::high_resolution_clock::now();
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    std::unique_ptr<QTimer> performance_timer;
};

/**
 * Cross-platform test fixture for platform-specific UI behavior
 */
class CrossPlatformUITestFixture : public MultiplayerUITestFixture {
protected:
    void SetUp() override {
        MultiplayerUITestFixture::SetUp();
        
        // Detect platform for platform-specific tests
#ifdef _WIN32
        current_platform = Platform::Windows;
#elif defined(__ANDROID__)
        current_platform = Platform::Android;
#elif defined(__linux__)
        current_platform = Platform::Linux;
#elif defined(__APPLE__)
        current_platform = Platform::macOS;
#else
        current_platform = Platform::Unknown;
#endif
    }

    enum class Platform {
        Windows,
        Android,
        Linux,
        macOS,
        Unknown
    };

    Platform current_platform = Platform::Unknown;
    
    bool isPlatform(Platform platform) const {
        return current_platform == platform;
    }
    
    void skipIfNotPlatform(Platform platform) const {
        if (current_platform != platform) {
            GTEST_SKIP() << "Test not applicable for current platform";
        }
    }
};

} // namespace Sudachi::Multiplayer::Testing
