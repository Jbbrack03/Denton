// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "p2p_network.h"
#include "libp2p_p2p_network.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>

namespace Core::Multiplayer::ModelA {

/**
 * Simple thread pool executor used by P2PNetwork to run asynchronous tasks
 * without spawning a new thread for each operation.
 *
 * Thread-safe: tasks can be submitted from multiple threads. The executor
 * manages its worker threads and joins them on destruction, ensuring no tasks
 * are left running when the owning P2PNetwork is destroyed.
 */
class AsyncExecutor {
public:
    explicit AsyncExecutor(size_t thread_count = std::thread::hardware_concurrency()) {
        if (thread_count == 0) {
            thread_count = 1;
        }
        for (size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~AsyncExecutor() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename Func>
    auto Submit(Func&& f) -> std::future<typename std::invoke_result_t<Func>> {
        using ReturnT = typename std::invoke_result_t<Func>;
        auto task = std::make_shared<std::packaged_task<ReturnT()>>(std::forward<Func>(f));
        auto fut = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

P2PNetwork::P2PNetwork(const P2PNetworkConfig& config)
    : impl_(std::make_unique<Libp2pP2PNetwork>(config)),
      executor_(std::make_shared<AsyncExecutor>()) {}

P2PNetwork::~P2PNetwork() = default;

std::future<MultiplayerResult> P2PNetwork::Start() {
    return executor_->Submit([this] { return impl_->Start(); });
}

MultiplayerResult P2PNetwork::Stop() { return impl_->Stop(); }

MultiplayerResult P2PNetwork::Shutdown() { return impl_->Shutdown(); }

bool P2PNetwork::IsStarted() const { return impl_->IsStarted(); }

std::string P2PNetwork::GetPeerId() const { return impl_->GetPeerId(); }

std::future<MultiplayerResult> P2PNetwork::ConnectToPeer(const std::string& peer_id, const std::string& multiaddr) {
    return executor_->Submit([this, peer_id, multiaddr] {
        return impl_->ConnectToPeer(peer_id, multiaddr);
    });
}

MultiplayerResult P2PNetwork::DisconnectFromPeer(const std::string& peer_id) {
    return impl_->DisconnectFromPeer(peer_id);
}

bool P2PNetwork::IsConnectedToPeer(const std::string& peer_id) const {
    return impl_->IsConnectedToPeer(peer_id);
}

bool P2PNetwork::IsConnectedViaaRelay(const std::string& peer_id) const {
    return impl_->IsConnectedViaaRelay(peer_id);
}

size_t P2PNetwork::GetConnectionCount() const { return impl_->GetConnectionCount(); }

std::vector<std::string> P2PNetwork::GetConnectedPeers() const { return impl_->GetConnectedPeers(); }

MultiplayerResult P2PNetwork::SendMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    return impl_->SendMessage(peer_id, protocol, data);
}

MultiplayerResult P2PNetwork::BroadcastMessage(const std::string& protocol, const std::vector<uint8_t>& data) {
    return impl_->BroadcastMessage(protocol, data);
}

void P2PNetwork::RegisterProtocolHandler(const std::string& protocol) {
    impl_->RegisterProtocolHandler(protocol);
}

void P2PNetwork::HandleIncomingMessage(const std::string& peer_id, const std::string& protocol, const std::vector<uint8_t>& data) {
    impl_->HandleIncomingMessage(peer_id, protocol, data);
}

std::future<MultiplayerResult> P2PNetwork::DetectNATType() {
    return executor_->Submit([this] { return impl_->DetectNATType(); });
}

bool P2PNetwork::CanTraverseNAT(NATType local_nat, NATType remote_nat) const {
    return impl_->CanTraverseNAT(local_nat, remote_nat);
}

std::vector<std::string> P2PNetwork::GetTraversalStrategies(NATType local_nat, NATType remote_nat) const {
    return impl_->GetTraversalStrategies(local_nat, remote_nat);
}

std::vector<std::string> P2PNetwork::GetConfiguredRelayServers() const {
    return impl_->GetConfiguredRelayServers();
}

void P2PNetwork::SetOnPeerConnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnPeerConnectedCallback(callback);
}

void P2PNetwork::SetOnPeerDisconnectedCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnPeerDisconnectedCallback(callback);
}

void P2PNetwork::SetOnConnectionFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnConnectionFailedCallback(callback);
}

void P2PNetwork::SetOnMessageReceivedCallback(std::function<void(const std::string&, const std::string&, const std::vector<uint8_t>&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnMessageReceivedCallback(callback);
}

void P2PNetwork::SetOnNATDetectedCallback(std::function<void(NATType, bool)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnNATDetectedCallback(callback);
}

void P2PNetwork::SetOnRelayConnectedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnRelayConnectedCallback(callback);
}

void P2PNetwork::SetOnRelayFailedCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    impl_->SetOnRelayFailedCallback(callback);
}

} // namespace Core::Multiplayer::ModelA
