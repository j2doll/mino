#pragma once

namespace mino::core::json {

    enum class value_type {
        null_type,
        boolean_type,
        number_type,
        string_type,
        array_type,
        object_type
    };

} // namespace mino::core::json
