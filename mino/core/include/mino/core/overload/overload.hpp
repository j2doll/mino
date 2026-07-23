#pragma once

#include <utility>

namespace mino::core::overload
{
    template <typename... Ts>
    struct overload : Ts... {
        using Ts::operator()...;
    };

    template <typename... Ts>
    overload(Ts...) -> overload<Ts...>;

}
