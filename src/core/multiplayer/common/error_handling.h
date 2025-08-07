// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "error_codes.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Core::Multiplayer {

/**
 * Error categories for classification and recovery strategies
 * Based on PRD Section 7.2
 */
enum class ErrorCategory {
  NetworkConnectivity, // Recoverable with retry
  PermissionDenied,    // Requires user action
  ConfigurationError,  // Requires settings change
  ProtocolMismatch,    // Requires update
  ResourceExhausted,   // Temporary limitation
  SecurityViolation,   // Connection terminated
  HardwareLimitation,  // Feature unavailable
  Unknown              // Unclassified error
};

/**
 * Detailed error information structure
 * Provides comprehensive error context and recovery guidance
 */
struct ErrorInfo {
  ErrorCategory category;
  ErrorCode error_code;
  std::string message;
  std::optional<uint32_t> retry_after_seconds;
  std::vector<std::string> suggested_actions;
  std::chrono::steady_clock::time_point timestamp;
  std::string component; // Which component generated the error
  std::unordered_map<std::string, std::string> context; // Additional context
};

/**
 * User notification levels for UI integration
 */
enum class NotificationLevel {
  Info,    // Transient toast (3 seconds)
  Warning, // Persistent banner
  Error,   // Modal dialog
  Critical // Blocking error screen
};

/**
 * Notification data for UI display
 */
struct NotificationData {
  NotificationLevel level;
  std::string title;
  std::string message;
  std::vector<std::string> actions;
  std::optional<uint32_t> auto_dismiss_ms;
  std::function<void(const std::string &)> action_callback;
};

/**
 * Error recovery strategy interface
 */
class IErrorRecoveryStrategy {
public:
  virtual ~IErrorRecoveryStrategy() = default;
  virtual bool CanRecover(const ErrorInfo &error) const = 0;
  virtual void AttemptRecovery(const ErrorInfo &error,
                               std::function<void(bool)> callback) = 0;
  virtual std::string GetName() const = 0;
};

/**
 * Main error handling framework
 * Centralizes error management, recovery, and user notification
 */
class ErrorHandler {
public:
  using ErrorCallback = std::function<void(const ErrorInfo &)>;
  using NotificationCallback = std::function<void(const NotificationData &)>;

  ErrorHandler();
  ~ErrorHandler();

  // Error reporting
  void ReportError(ErrorCode code, const std::string &message,
                   const std::string &component = "Unknown");
  void ReportError(const ErrorInfo &error);

  // Error enrichment
  void SetErrorContext(ErrorCode code, const std::string &key,
                       const std::string &value);
  void SetRetryDelay(ErrorCode code, uint32_t seconds);
  void AddSuggestedAction(ErrorCode code, const std::string &action);

  // Recovery strategies
  void
  RegisterRecoveryStrategy(ErrorCategory category,
                           std::unique_ptr<IErrorRecoveryStrategy> strategy);
  bool AttemptRecovery(const ErrorInfo &error);

  // Notification management
  void SetNotificationCallback(NotificationCallback callback);
  void ShowNotification(const ErrorInfo &error);
  void DismissNotification(const std::string &notification_id);

  // Error history and statistics
  std::vector<ErrorInfo> GetRecentErrors(size_t count = 10) const;
  std::unordered_map<ErrorCode, size_t> GetErrorStatistics() const;
  void ClearErrorHistory();

  // Configuration
  void SetMaxErrorHistorySize(size_t size);
  void SetAutoRecoveryEnabled(bool enabled);
  void SetNotificationLevel(ErrorCode code, NotificationLevel level);

  // Callbacks
  void SetOnError(ErrorCallback callback);
  void SetOnRecoverySuccess(ErrorCallback callback);
  void SetOnRecoveryFailure(ErrorCallback callback);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * Common error recovery strategies
 */
class NetworkRetryStrategy : public IErrorRecoveryStrategy {
public:
  NetworkRetryStrategy(uint32_t max_retries = 3,
                       uint32_t initial_delay_ms = 1000);
  ~NetworkRetryStrategy();

  bool CanRecover(const ErrorInfo &error) const override;
  void AttemptRecovery(const ErrorInfo &error,
                       std::function<void(bool)> callback) override;
  std::string GetName() const override { return "Network Retry"; }

private:
  uint32_t max_retries_;
  uint32_t initial_delay_ms_;
  std::unordered_map<ErrorCode, uint32_t> retry_counts_;
  mutable std::mutex mutex_;

  // Deterministic random engine
  std::mt19937 rng_;

  // Shared worker thread infrastructure
  std::thread worker_;
  std::mutex task_mutex_;
  std::condition_variable task_cv_;
  std::queue<std::function<void()>> tasks_;
  bool stop_{false};
};

/**
 * Permission request strategy for platform-specific permissions
 */
class PermissionRequestStrategy : public IErrorRecoveryStrategy {
public:
  using PermissionRequestFunc =
      std::function<void(const std::string &, std::function<void(bool)>)>;

  PermissionRequestStrategy(PermissionRequestFunc request_func);

  bool CanRecover(const ErrorInfo &error) const override;
  void AttemptRecovery(const ErrorInfo &error,
                       std::function<void(bool)> callback) override;
  std::string GetName() const override { return "Permission Request"; }

private:
  PermissionRequestFunc request_permission_;
};

/**
 * Fallback mode strategy for switching between multiplayer modes
 */
class FallbackModeStrategy : public IErrorRecoveryStrategy {
public:
  using ModeSwitchFunc = std::function<bool(const std::string &)>;

  FallbackModeStrategy(ModeSwitchFunc switch_func);

  bool CanRecover(const ErrorInfo &error) const override;
  void AttemptRecovery(const ErrorInfo &error,
                       std::function<void(bool)> callback) override;
  std::string GetName() const override { return "Mode Fallback"; }

private:
  ModeSwitchFunc switch_mode_;
};

/**
 * Helper functions for error creation
 */
ErrorInfo CreateNetworkError(ErrorCode code, const std::string &message);
ErrorInfo CreatePermissionError(ErrorCode code, const std::string &permission);
ErrorInfo CreateConfigurationError(ErrorCode code, const std::string &setting);
ErrorInfo CreateSecurityError(ErrorCode code, const std::string &violation);

/**
 * Global error handler instance
 */
ErrorHandler &GetGlobalErrorHandler();

} // namespace Core::Multiplayer