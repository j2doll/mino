#include <mutex>

#include "mino/core/dispatch/dynamic_event_dispatcher.hpp"

namespace mino::core::dispatch {

    dynamic_event_dispatcher::handler_id dynamic_event_dispatcher::subscribe_raw(
        std::type_index type,
        std::function<void(const void*)> callback
    ) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handler_id id = next_id_++;
        subscribers_[type].push_back({ id, std::move(callback) });
        return id;
    }

    bool dynamic_event_dispatcher::unsubscribe_raw(std::type_index type, handler_id id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = subscribers_.find(type);
        if (it == subscribers_.end()) {
            return false;
        }

        auto& list = it->second;
        for (auto entry_it = list.begin(); entry_it != list.end(); ++entry_it) {
            if (entry_it->id == id) {
                list.erase(entry_it);
                return true;
            }
        }
        return false;
    }

    void dynamic_event_dispatcher::dispatch_raw(std::type_index type, const void* event_ptr) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = subscribers_.find(type);
        if (it == subscribers_.end()) {
            return;
        }

        for (const auto& entry : it->second) {
            entry.callback(event_ptr);
        }
    }

} // namespace mino::core::dispatch
