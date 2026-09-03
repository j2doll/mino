#pragma once

#include <cstddef>
#include <functional>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mino::core::dispatch {

    class dynamic_event_dispatcher {
    public:
        using handler_id = std::size_t;

        template <typename event_t, typename callable_t>
        handler_id subscribe(callable_t&& handler) {
            auto type = std::type_index(typeid(event_t));
            auto wrapper = [fn = std::forward<callable_t>(handler)](const void* event_ptr) {
                fn(*static_cast<const event_t*>(event_ptr));
                };
            return subscribe_raw(type, std::move(wrapper));
        }

        template <typename event_t>
        bool unsubscribe(handler_id id) {
            return unsubscribe_raw(std::type_index(typeid(event_t)), id);
        }

        template <typename event_t>
        void dispatch(const event_t& event) const {
            dispatch_raw(std::type_index(typeid(event_t)), &event);
        }

        template <typename... events_t>
        void dispatch_all(const events_t&... events) const {
            (dispatch(events), ...);
        }

    private:
        struct subscription_entry {
            handler_id id;
            std::function<void(const void*)> callback;
        };

        handler_id subscribe_raw(std::type_index type, std::function<void(const void*)> callback);
        bool unsubscribe_raw(std::type_index type, handler_id id);
        void dispatch_raw(std::type_index type, const void* event_ptr) const;

        mutable std::shared_mutex mutex_;
        handler_id next_id_ = 1;
        std::unordered_map<std::type_index, std::vector<subscription_entry>> subscribers_;
    };

} // namespace mino::core::dispatch
