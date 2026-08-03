#pragma once

#include <string>

#include "mino/core/json/json_fwd.hpp"

namespace mino::core::json {

    class file_reader {
    public:
        static value read_file(const std::string& file_path) noexcept;
    };

} // namespace mino::core::json

