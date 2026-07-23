#pragma once

#include <functional>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <cstdint>
#include <memory>

namespace mino::core::notifications {

    // Generic thread-safe event / observable
    template <typename... Args>
    class event {
    public:
        using callback_t = std::function<void(Args...)>;
        using id_t = std::uint64_t;

        event() noexcept = default;
        ~event() noexcept = default;

        // non-copyable, non-movable to avoid dangling connections
        event(const event&) = delete;
        event& operator=(const event&) = delete;
        event(event&&) = delete;
        event& operator=(event&&) = delete;

        // Subscribe a callback. Returns an id that can be used to unsubscribe.
        id_t subscribe(callback_t cb) {
            std::lock_guard<std::mutex> lock(mutex_);
            const id_t id = next_id_++;
            listeners_.emplace(id, std::move(cb));
            return id;
        }

        // Unsubscribe by id. Returns true if removed.
        bool unsubscribe(id_t id) noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return listeners_.erase(id) > 0;
        }

        // Notify all listeners. Safe to call from multiple threads.
        void notify(Args... args) {
            // Copy listeners to avoid holding lock while invoking callbacks.
            std::vector<callback_t> callbacks;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                callbacks.reserve(listeners_.size());
                for (auto const & kv : listeners_) {
                    callbacks.push_back(kv.second);
                }
            }
            for (auto & cb : callbacks) {
                if (cb) {
                    try {
                        cb(args...);
                    }
                    catch (...) {
                        // Swallow exceptions to avoid breaking other listeners.
                        // If desired, extend to log or rethrow.
                    }
                }
            }
        }

        // RAII handle for automatic unsubscription
        class  scoped_connection {
        public:
            scoped_connection() noexcept = default;

            scoped_connection(event* ev, id_t id) noexcept
                : event_(ev), id_(id) {}

            scoped_connection(scoped_connection&& other) noexcept
                : event_(other.event_), id_(other.id_) {
                other.event_ = nullptr;
                other.id_ = 0;
            }

            scoped_connection& operator=(scoped_connection&& other) noexcept {
                if (this != &other) {
                    disconnect();
                    event_ = other.event_;
                    id_ = other.id_;
                    other.event_ = nullptr;
                    other.id_ = 0;
                }
                return *this;
            }

            ~scoped_connection() noexcept {
                disconnect();
            }

            // Explicit disconnect before destruction
            void disconnect() noexcept {
                if (event_ && id_ != 0) {
                    event_->unsubscribe(id_);
                    event_ = nullptr;
                    id_ = 0;
                }
            }

            bool connected() const noexcept {
                return event_ != nullptr && id_ != 0;
            }

            // non-copyable
            scoped_connection(const scoped_connection&) = delete;
            scoped_connection& operator=(const scoped_connection&) = delete;

        private:
            event* event_ = nullptr;
            id_t id_ = 0;
        };

        // Helper: subscribe and return a scoped_connection for RAII
         scoped_connection subscribe_scoped(callback_t cb) {
            id_t id = subscribe(std::move(cb));
            return scoped_connection(this, id);
        }

    private:
        std::unordered_map<id_t, callback_t> listeners_;
        std::mutex mutex_;
        id_t next_id_ = 1;
    };

} 