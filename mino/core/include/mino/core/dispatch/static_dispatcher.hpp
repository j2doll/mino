#pragma once

#include <utility>
#include <variant>
#include <vector>

namespace mino::core::dispatch {

    template <typename... callables_t>
    struct overloaded : callables_t... {
        using callables_t::operator()...;
    };

    template <typename... callables_t>
    overloaded(callables_t...) -> overloaded<callables_t...>;

    template <typename... events_t>
    class static_dispatcher {
    public:
        using variant_type = std::variant<events_t...>;

        template <typename visitor_t>
        static constexpr decltype(auto) dispatch(const variant_type& event, visitor_t&& visitor) {
            return std::visit(std::forward<visitor_t>(visitor), event);
        }

        template <typename visitor_t>
        static constexpr decltype(auto) dispatch(variant_type&& event, visitor_t&& visitor) {
            return std::visit(std::forward<visitor_t>(visitor), std::move(event));
        }

        template <typename event_t, typename visitor_t>
        static constexpr decltype(auto) dispatch_value(event_t&& event, visitor_t&& visitor) {
            return std::visit(
                std::forward<visitor_t>(visitor),
                variant_type{ std::forward<event_t>(event) }
            );
        }

        template <typename visitor_t>
        static void dispatch_queue(const std::vector<variant_type>& queue, visitor_t&& visitor) {
            for (const auto& event : queue) {
                std::visit(visitor, event);
            }
        }
    };

} // namespace mino::core::dispatch
