// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QString>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Sudachi::Multiplayer::Testing {

/**
 * Mock multiplayer backend for UI testing
 * Simulates the behavior of the actual multiplayer system
 */
class MockMultiplayerBackend : public QObject {
    Q_OBJECT

public:
    explicit MockMultiplayerBackend(QObject* parent = nullptr) : QObject(parent) {}

    // Connection Management
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Shutdown, (), ());
    MOCK_METHOD(bool, IsInitialized, (), (const));

    // Session Management
    MOCK_METHOD(bool, CreateSession, (const std::string& game_id, int max_players), ());
    MOCK_METHOD(bool, JoinSession, (const std::string& session_id), ());
    MOCK_METHOD(void, LeaveSession, (), ());
    MOCK_METHOD(bool, IsInSession, (), (const));

    // Connection Status
    MOCK_METHOD(bool, IsConnected, (), (const));
    MOCK_METHOD(std::string, GetConnectionStatus, (), (const));
    MOCK_METHOD(int, GetPing, (), (const));
    MOCK_METHOD(double, GetConnectionQuality, (), (const));

    // Player Management
    MOCK_METHOD(std::vector<std::string>, GetConnectedPlayers, (), (const));
    MOCK_METHOD(int, GetPlayerCount, (), (const));
    MOCK_METHOD(int, GetMaxPlayers, (), (const));

    // Network Configuration
    MOCK_METHOD(bool, SetNetworkMode, (const std::string& mode), ());
    MOCK_METHOD(std::string, GetNetworkMode, (), (const));
    MOCK_METHOD(bool, EnableInternetMode, (), ());
    MOCK_METHOD(bool, EnableAdHocMode, (), ());

    // Error Handling
    MOCK_METHOD(std::string, GetLastError, (), (const));
    MOCK_METHOD(bool, HasError, (), (const));
    MOCK_METHOD(void, ClearError, (), ());

    // Mock control methods for testing
    void simulateConnectionSuccess() {
        emit connectionStatusChanged("Connected");
        emit connected();
    }

    void simulateConnectionFailure(const std::string& error) {
        emit connectionStatusChanged("Disconnected");
        emit errorOccurred(QString::fromStdString(error));
    }

    void simulatePlayerJoined(const std::string& player_name) {
        emit playerJoined(QString::fromStdString(player_name));
        emit playerCountChanged(mock_player_count + 1);
        mock_player_count++;
    }

    void simulatePlayerLeft(const std::string& player_name) {
        emit playerLeft(QString::fromStdString(player_name));
        emit playerCountChanged(mock_player_count - 1);
        mock_player_count--;
    }

    void simulateNetworkModeChanged(const std::string& mode) {
        emit networkModeChanged(QString::fromStdString(mode));
    }

    void simulatePingUpdate(int ping_ms) {
        emit pingUpdated(ping_ms);
    }

    // Test state management
    void setMockConnected(bool connected) { mock_connected = connected; }
    void setMockInSession(bool in_session) { mock_in_session = in_session; }
    void setMockPlayerCount(int count) { mock_player_count = count; }
    void setMockError(const std::string& error) { mock_error = error; }

signals:
    // Connection signals
    void connected();
    void disconnected();
    void connectionStatusChanged(const QString& status);
    void connectionQualityChanged(double quality);

    // Session signals
    void sessionCreated(const QString& session_id);
    void sessionJoined(const QString& session_id);
    void sessionLeft();
    void sessionError(const QString& error);

    // Player signals
    void playerJoined(const QString& player_name);
    void playerLeft(const QString& player_name);
    void playerCountChanged(int count);

    // Network signals
    void networkModeChanged(const QString& mode);
    void pingUpdated(int ping_ms);

    // Error signals
    void errorOccurred(const QString& error);
    void warningOccurred(const QString& warning);

private:
    // Mock state
    bool mock_connected = false;
    bool mock_in_session = false;
    int mock_player_count = 0;
    std::string mock_error;
};

/**
 * Mock Network Manager for network-specific UI testing
 */
class MockNetworkManager : public QObject {
    Q_OBJECT

public:
    explicit MockNetworkManager(QObject* parent = nullptr) : QObject(parent) {}

    // Network Detection
    MOCK_METHOD(bool, IsNetworkAvailable, (), (const));
    MOCK_METHOD(bool, IsWiFiConnected, (), (const));
    MOCK_METHOD(bool, IsMobileDataConnected, (), (const));
    MOCK_METHOD(std::string, GetNetworkType, (), (const));

    // Network Quality
    MOCK_METHOD(int, GetSignalStrength, (), (const));
    MOCK_METHOD(double, GetBandwidth, (), (const));
    MOCK_METHOD(int, GetLatency, (), (const));

    // Network Configuration
    MOCK_METHOD(bool, ConfigureNetwork, (const std::string& config), ());
    MOCK_METHOD(std::string, GetNetworkConfiguration, (), (const));

    // Mock simulation methods
    void simulateNetworkChange(const std::string& network_type) {
        emit networkTypeChanged(QString::fromStdString(network_type));
    }

    void simulateSignalStrengthChange(int strength) {
        emit signalStrengthChanged(strength);
    }

    void simulateNetworkLoss() {
        emit networkLost();
    }

    void simulateNetworkRestored() {
        emit networkRestored();
    }

signals:
    void networkTypeChanged(const QString& type);
    void signalStrengthChanged(int strength);
    void networkLost();
    void networkRestored();
    void bandwidthChanged(double bandwidth);
    void latencyChanged(int latency_ms);
};

/**
 * Mock Game Session for game-specific UI testing
 */
class MockGameSession : public QObject {
    Q_OBJECT

public:
    explicit MockGameSession(QObject* parent = nullptr) : QObject(parent) {}

    // Game State
    MOCK_METHOD(bool, IsGameRunning, (), (const));
    MOCK_METHOD(std::string, GetGameTitle, (), (const));
    MOCK_METHOD(std::string, GetGameId, (), (const));
    MOCK_METHOD(bool, SupportsMultiplayer, (), (const));

    // Session State
    MOCK_METHOD(bool, IsHosting, (), (const));
    MOCK_METHOD(std::string, GetSessionId, (), (const));
    MOCK_METHOD(bool, HasPassword, (), (const));

    // Game-specific multiplayer features
    MOCK_METHOD(bool, EnableMultiplayer, (), ());
    MOCK_METHOD(void, DisableMultiplayer, (), ());
    MOCK_METHOD(bool, PauseForMultiplayer, (), ());
    MOCK_METHOD(void, ResumeFromMultiplayer, (), ());

    // Mock simulation methods
    void simulateGameStart(const std::string& game_title) {
        emit gameStarted(QString::fromStdString(game_title));
    }

    void simulateGameStop() {
        emit gameStopped();
    }

    void simulateMultiplayerEnabled() {
        emit multiplayerEnabled();
    }

    void simulateMultiplayerDisabled() {
        emit multiplayerDisabled();
    }

signals:
    void gameStarted(const QString& title);
    void gameStopped();
    void multiplayerEnabled();
    void multiplayerDisabled();
    void sessionStateChanged(const QString& state);
};

} // namespace Sudachi::Multiplayer::Testing