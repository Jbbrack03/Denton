// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <map>

using json = nlohmann::json;

namespace Sudachi::Multiplayer::Tests {

/**
 * Mock WebSocket connection interface for testing
 */
class IWebSocketConnection {
public:
    virtual ~IWebSocketConnection() = default;
    virtual void Send(const std::string& message) = 0;
    virtual void Close(uint16_t code = 1000, const std::string& reason = "") = 0;
    virtual std::string GetRemoteAddress() const = 0;
    virtual bool IsOpen() const = 0;
};

/**
 * Mock WebSocket server interface
 */
class IWebSocketServer {
public:
    using ConnectionHandler = std::function<void(const std::string& client_id, std::shared_ptr<IWebSocketConnection>)>;
    using MessageHandler = std::function<void(const std::string& client_id, const std::string& message)>;
    using DisconnectHandler = std::function<void(const std::string& client_id)>;

    virtual ~IWebSocketServer() = default;
    virtual void Start(uint16_t port) = 0;
    virtual void Stop() = 0;
    virtual void SendMessage(const std::string& client_id, const json& message) = 0;
    virtual void BroadcastMessage(const json& message) = 0;
    virtual void DisconnectClient(const std::string& client_id) = 0;
    virtual size_t GetClientCount() const = 0;
    virtual std::vector<std::string> GetConnectedClients() const = 0;
    
    virtual void SetOnConnect(ConnectionHandler handler) = 0;
    virtual void SetOnMessage(MessageHandler handler) = 0;
    virtual void SetOnDisconnect(DisconnectHandler handler) = 0;
};

/**
 * Mock WebSocket server implementation using Google Mock
 */
class MockWebSocketServer : public IWebSocketServer {
public:
    MOCK_METHOD(void, Start, (uint16_t port), (override));
    MOCK_METHOD(void, Stop, (), (override));
    MOCK_METHOD(void, SendMessage, (const std::string& client_id, const json& message), (override));
    MOCK_METHOD(void, BroadcastMessage, (const json& message), (override));
    MOCK_METHOD(void, DisconnectClient, (const std::string& client_id), (override));
    MOCK_METHOD(size_t, GetClientCount, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, GetConnectedClients, (), (const, override));
    
    MOCK_METHOD(void, SetOnConnect, (ConnectionHandler handler), (override));
    MOCK_METHOD(void, SetOnMessage, (MessageHandler handler), (override));
    MOCK_METHOD(void, SetOnDisconnect, (DisconnectHandler handler), (override));

    // Helper methods for testing
    void SimulateClientConnect(const std::string& client_id) {
        if (on_connect_handler_) {
            auto mock_connection = std::make_shared<MockWebSocketConnection>();
            clients_[client_id] = mock_connection;
            on_connect_handler_(client_id, mock_connection);
        }
    }

    void SimulateClientMessage(const std::string& client_id, const std::string& message) {
        if (on_message_handler_) {
            on_message_handler_(client_id, message);
        }
    }

    void SimulateClientDisconnect(const std::string& client_id) {
        if (on_disconnect_handler_) {
            clients_.erase(client_id);
            on_disconnect_handler_(client_id);
        }
    }

    // Test helpers for verifying sent messages
    struct SentMessage {
        std::string client_id;
        json message;
    };

    std::vector<SentMessage> GetSentMessages() const {
        return sent_messages_;
    }

    void ClearSentMessages() {
        sent_messages_.clear();
    }

private:
    class MockWebSocketConnection : public IWebSocketConnection {
    public:
        MOCK_METHOD(void, Send, (const std::string& message), (override));
        MOCK_METHOD(void, Close, (uint16_t code, const std::string& reason), (override));
        MOCK_METHOD(std::string, GetRemoteAddress, (), (const, override));
        MOCK_METHOD(bool, IsOpen, (), (const, override));
    };

    ConnectionHandler on_connect_handler_;
    MessageHandler on_message_handler_;
    DisconnectHandler on_disconnect_handler_;
    std::map<std::string, std::shared_ptr<IWebSocketConnection>> clients_;
    mutable std::vector<SentMessage> sent_messages_;

    // Override implementations to track behavior
    void SendMessageImpl(const std::string& client_id, const json& message) {
        sent_messages_.push_back({client_id, message});
        if (auto it = clients_.find(client_id); it != clients_.end()) {
            it->second->Send(message.dump());
        }
    }
};

/**
 * Test fixture for WebSocket-based tests
 */
class WebSocketTestFixture {
public:
    WebSocketTestFixture() : mock_server_(std::make_unique<MockWebSocketServer>()) {
        SetupDefaultBehavior();
    }

    void SetupDefaultBehavior() {
        using ::testing::_;
        using ::testing::Invoke;

        // Default behavior for tracking sent messages
        ON_CALL(*mock_server_, SendMessage(_, _))
            .WillByDefault(Invoke([this](const std::string& client_id, const json& message) {
                sent_messages_.push_back({client_id, message});
            }));

        ON_CALL(*mock_server_, BroadcastMessage(_))
            .WillByDefault(Invoke([this](const json& message) {
                for (const auto& client_id : connected_clients_) {
                    sent_messages_.push_back({client_id, message});
                }
            }));

        ON_CALL(*mock_server_, GetClientCount())
            .WillByDefault(Invoke([this]() { return connected_clients_.size(); }));

        ON_CALL(*mock_server_, GetConnectedClients())
            .WillByDefault(Invoke([this]() { return connected_clients_; }));
    }

    // Helper to simulate Room Server responses
    void SimulateRoomServerResponse(const std::string& client_id, const std::string& message_type, const json& data) {
        json response;
        response["type"] = message_type;
        response["data"] = data;
        mock_server_->SimulateClientMessage(client_id, response.dump());
    }

    // Helper to verify sent messages
    bool VerifyMessageSent(const std::string& client_id, const std::string& message_type) {
        return std::any_of(sent_messages_.begin(), sent_messages_.end(),
            [&](const MockWebSocketServer::SentMessage& msg) {
                return msg.client_id == client_id && 
                       msg.message.contains("type") && 
                       msg.message["type"] == message_type;
            });
    }

    json GetLastSentMessage(const std::string& client_id) {
        for (auto it = sent_messages_.rbegin(); it != sent_messages_.rend(); ++it) {
            if (it->client_id == client_id) {
                return it->message;
            }
        }
        return {};
    }

protected:
    std::unique_ptr<MockWebSocketServer> mock_server_;
    std::vector<std::string> connected_clients_;
    std::vector<MockWebSocketServer::SentMessage> sent_messages_;
};

} // namespace Sudachi::Multiplayer::Tests