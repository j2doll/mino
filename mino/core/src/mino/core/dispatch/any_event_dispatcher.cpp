#include <mutex>

#include "mino/core/dispatch/any_event_dispatcher.hpp"

namespace mino::core::dispatch {

    any_event_dispatcher::handler_id any_event_dispatcher::subscribe_raw(
        std::type_index type,
        std::function<void(const std::any&)> callback
    ) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handler_id id = next_id_++;
        subscribers_[type].push_back({ id, std::move(callback) });
        return id;
    }

    bool any_event_dispatcher::unsubscribe_raw(std::type_index type, handler_id id) {
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

    bool any_event_dispatcher::dispatch_any(const std::any& event) const {
        if (!event.has_value()) {
            return false;
        }

        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto type = std::type_index(event.type());
        auto it = subscribers_.find(type);
        if (it == subscribers_.end()) {
            return false;
        }

        for (const auto& entry : it->second) {
            entry.callback(event);
        }
        return true;
    }

} // namespace mino::core::dispatch
