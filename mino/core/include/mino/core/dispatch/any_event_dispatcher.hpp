#pragma once

#include <any>
#include <cstddef>
#include <functional>
#include <shared_mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mino::core::dispatch {

    class any_event_dispatcher {
    public:
        using handler_id = std::size_t;

        template <typename event_t, typename callable_t>
        handler_id subscribe(callable_t&& handler) {
            auto type = std::type_index(typeid(event_t));
            auto wrapper = [fn = std::forward<callable_t>(handler)](const std::any& any_event) {
                fn(std::any_cast<const event_t&>(any_event));
                };
            return subscribe_raw(type, std::move(wrapper));
        }

        template <typename event_t>
        bool unsubscribe(handler_id id) {
            return unsubscribe_raw(std::type_index(typeid(event_t)), id);
        }

        bool dispatch_any(const std::any& event) const;

        template <typename event_t>
        bool dispatch(event_t&& event) const {
            return dispatch_any(std::make_any<std::decay_t<event_t>>(std::forward<event_t>(event)));
        }

    private:
        struct subscription_entry {
            handler_id id;
            std::function<void(const std::any&)> callback;
        };

        handler_id subscribe_raw(std::type_index type, std::function<void(const std::any&)> callback);
        bool unsubscribe_raw(std::type_index type, handler_id id);

        mutable std::shared_mutex mutex_;
        handler_id next_id_ = 1;
        std::unordered_map<std::type_index, std::vector<subscription_entry>> subscribers_;
    };

} // namespace mino::core::dispatch
