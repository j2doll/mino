#pragma once

#include <string>

#include "mino/core/json/json_fwd.hpp"

namespace mino::core::json {

    class serializer {
    public:
        // indent <= 0 이면 compact(기존 동작), indent > 0 이면 pretty-print
        static std::string serialize(const value& val, int indent = -1) noexcept;
    };

} // namespace mino::core::json

