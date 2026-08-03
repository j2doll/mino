#pragma once

#include <string>

#include "mino/core/json/json_fwd.hpp"

namespace mino::core::json {

    class serializer {
    public:
        static std::string serialize(const value& val) noexcept;
    };

} // namespace mino::core::json

