// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "core/multiplayer/common/error_codes.h"
#include "i_websocket_connection.h"
#include "room_messages.h"
#include "room_types.h"

namespace nlohmann {
class json;
}

/**
 * Utility class for calculating exponential backoff delays
 * Provides configurable exponential backoff with jitter and maximum delay
 * limits
 */
class ExponentialBackoff {
public:
  ExponentialBackoff(std::chrono::milliseconds base_delay, double multiplier,
                     std::chrono::milliseconds max_delay);

  std::chrono::milliseconds CalculateDelay(int attempt) const;
  std::chrono::milliseconds CalculateDelayWithJitter(int attempt) const;
  void Reset();

private:
  std::chrono::milliseconds base_delay_;
  double multiplier_;
  std::chrono::milliseconds max_delay_;
};

namespace Core::Multiplayer::ModelA {

// Forward declarations for config interfaces
class IConfigProvider {
public:
  virtual ~IConfigProvider() = default;
  virtual std::string GetRoomServerUrl() const = 0;
  virtual std::string GetAuthToken() const = 0;
  virtual std::chrono::milliseconds GetConnectionTimeout() const = 0;
  virtual std::chrono::milliseconds GetHeartbeatInterval() const = 0;
  virtual std::chrono::milliseconds GetMessageTimeout() const = 0;
  virtual bool IsAutoReconnectEnabled() const = 0;
  virtual int GetMaxReconnectAttempts() const = 0;
  virtual std::chrono::milliseconds GetReconnectBaseDelay() const = 0;
  virtual double GetReconnectBackoffMultiplier() const = 0;
  virtual std::chrono::milliseconds GetMaxReconnectDelay() const = 0;
  virtual bool ShouldReconnectOnError(const std::string &error_type) const = 0;
  virtual int GetMaxConcurrentMessages() const = 0;
  virtual size_t GetMessageQueueSize() const = 0;
};

// Message handler interface
class IMessageHandler {
public:
  virtual ~IMessageHandler() = default;
  virtual void OnRoomCreated(const RoomCreatedResponse &response) = 0;
  virtual void OnRoomListUpdate(const RoomListResponse &response) = 0;
  virtual void OnJoinedRoom(const JoinRoomResponse &response) = 0;
  virtual void OnP2PInfoReceived(const P2PInfoMessage &message) = 0;
  virtual void OnUseProxyMessage(const UseProxyMessage &message) = 0;
  virtual void OnErrorReceived(const ErrorMessage &error) = 0;
  virtual void OnPlayerJoined(const PlayerJoinedMessage &message) = 0;
  virtual void OnPlayerLeft(const PlayerLeftMessage &message) = 0;
};

// Reconnection listener interface
class IReconnectionListener {
public:
  virtual ~IReconnectionListener() = default;
  virtual void OnReconnectionStarted(int attempt) = 0;
  virtual void OnReconnectionFailed(int attempt, const std::string &error) = 0;
  virtual void OnReconnectionSucceeded(int attempt) = 0;
  virtual void OnReconnectionGivenUp(int total_attempts) = 0;
  virtual void OnConnectionLost(const std::string &reason) = 0;
};

/**
 * WebSocket-based Room Client for Model A (Internet Multiplayer)
 * Handles connection to room server, message serialization, and reconnection
 * logic
 */
class RoomClient {
public:
  RoomClient(std::shared_ptr<IWebSocketConnection> connection,
             std::shared_ptr<IConfigProvider> config);
  ~RoomClient();

  // Connection management
  ErrorCode Connect();
  void Disconnect();
  ErrorCode Reconnect();
  void Shutdown();
  bool IsConnected() const;
  ConnectionState GetConnectionState() const;
  ConnectionInfo GetConnectionInfo() const;

  // Authentication and identification
  std::string GetClientId() const;
  void SetCurrentRoomId(const std::string &room_id);
  std::string GetCurrentRoomId() const;
  void SetPlayerSlot(int slot);
  int GetPlayerSlot() const;
  bool IsInRoom() const;

  // Room operations
  ErrorCode JoinRoom(const std::string &room_id);
  ErrorCode LeaveRoom();

  // Message operations
  ErrorCode SendMessage(const std::string &message);
  ErrorCode QueueMessage(const std::string &message);
  bool ProcessPendingMessages();
  size_t GetPendingMessageCount() const;

  // Callbacks and handlers
  void SetOnConnectedCallback(std::function<void()> callback);
  void
  SetOnDisconnectedCallback(std::function<void(const std::string &)> callback);
  void SetOnMessageCallback(std::function<void(const std::string &)> callback);
  void SetMessageHandler(std::shared_ptr<IMessageHandler> handler);

  // Reconnection management
  void SetReconnectionListener(std::shared_ptr<IReconnectionListener> listener);
  bool IsReconnecting() const;
  int GetReconnectionAttempts() const;
  std::chrono::milliseconds CalculateReconnectDelay(int attempt) const;
  ReconnectionStatistics GetReconnectionStatistics() const;

  // Heartbeat management
  void StartHeartbeat();
  void StopHeartbeat();

  // WebSocket event handlers (internal)
  void OnWebSocketConnected();
  void OnWebSocketDisconnected(const std::string &reason);
  void OnWebSocketError(const std::string &error);
  void OnWebSocketMessage(const std::string &message);

  // Test utilities
  void SimulateIncomingMessage(const std::string &message);

private:
  // Internal state
  std::shared_ptr<IWebSocketConnection> connection_;
  std::shared_ptr<IConfigProvider> config_;
  std::shared_ptr<IMessageHandler> message_handler_;
  std::shared_ptr<IReconnectionListener> reconnection_listener_;

  // Connection state
  mutable std::mutex state_mutex_;
  std::condition_variable connection_cv_;
  std::atomic<ConnectionState> connection_state_{ConnectionState::Disconnected};
  std::string client_id_;
  uint64_t created_at_;

  // Room state
  mutable std::mutex room_mutex_;
  std::string current_room_id_;
  int player_slot_ = -1;

  // Message handling
  mutable std::mutex message_mutex_;
  std::queue<std::string> message_queue_;
  std::condition_variable message_cv_;
  std::atomic<bool> processing_messages_{false};

  // Reconnection state
  mutable std::mutex reconnection_mutex_;
  std::atomic<bool> is_reconnecting_{false};
  std::atomic<int> reconnection_attempts_{0};
  std::thread reconnection_thread_;
  ReconnectionStatistics reconnection_stats_;
  std::unique_ptr<ExponentialBackoff> reconnection_backoff_;

  // Heartbeat
  std::atomic<bool> heartbeat_active_{false};
  std::thread heartbeat_thread_;

  // Callbacks
  mutable std::mutex callback_mutex_;
  std::function<void()> on_connected_;
  std::function<void(const std::string &)> on_disconnected_;
  std::function<void(const std::string &)> on_message_;

  // Shutdown flag
  std::atomic<bool> shutdown_requested_{false};

  // Internal methods
  void InitializeWebSocketCallbacks();
  void StartReconnectionProcess(const std::string &reason);
  void ReconnectionWorker();
  void HeartbeatWorker();
  void ProcessMessage(const std::string &message);
  void ProcessRoomCreatedMessage(const nlohmann::json &j);
  void ProcessErrorMessage(const nlohmann::json &j);
  void ProcessRoomListMessage(const nlohmann::json &j);
  void ProcessJoinRoomMessage(const nlohmann::json &j);
  void ProcessP2PInfoMessage(const nlohmann::json &j);
  void ProcessUseProxyMessage(const nlohmann::json &j);
  void ProcessPlayerJoinedMessage(const nlohmann::json &j);
  void ProcessPlayerLeftMessage(const nlohmann::json &j);
  void DispatchMessage(const BaseMessage &base_message,
                       const std::string &raw_message);
  std::string GenerateClientId();
  uint64_t GetCurrentTimestamp();
};

} // namespace Core::Multiplayer::ModelA
