#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mino::core::dispatch {

    template <typename return_t>
    class collecting_event_dispatcher {
    public:
        using handler_id = std::size_t;

        template <typename event_t, typename callable_t>
        handler_id subscribe(callable_t&& handler) {
            static_assert(std::is_invocable_r_v<return_t, callable_t, const event_t&>,
                "Handler return type must be convertible to return_t");

            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto type = std::type_index(typeid(event_t));
            handler_id id = next_id_++;

            auto wrapper = [fn = std::forward<callable_t>(handler)](const void* event_ptr) -> return_t {
                return fn(*static_cast<const event_t*>(event_ptr));
                };

            subscribers_[type].push_back({ id, std::move(wrapper) });
            return id;
        }

        template <typename event_t>
        bool unsubscribe(handler_id id) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto type = std::type_index(typeid(event_t));
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

        template <typename event_t>
        std::vector<return_t> dispatch(const event_t& event) const {
            std::vector<return_t> results;

            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto type = std::type_index(typeid(event_t));
            auto it = subscribers_.find(type);
            if (it == subscribers_.end()) {
                return results;
            }

            results.reserve(it->second.size());
            for (const auto& entry : it->second) {
                results.push_back(entry.callback(&event));
            }

            return results;
        }

        template <typename event_t>
        std::size_t subscriber_count() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto type = std::type_index(typeid(event_t));
            auto it = subscribers_.find(type);
            return (it != subscribers_.end()) ? it->second.size() : 0;
        }

        void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            subscribers_.clear();
        }

    private:
        struct subscription_entry {
            handler_id id;
            std::function<return_t(const void*)> callback;
        };

        mutable std::shared_mutex mutex_;
        handler_id next_id_ = 1;
        std::unordered_map<std::type_index, std::vector<subscription_entry>> subscribers_;
    };

} // namespace mino::core::dispatch
