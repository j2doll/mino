#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace mino::core::json {

    enum class value_type;

    class value;
    class parser;
    class serializer;
    class file_reader;

    using array_t = std::vector<value>;
    using object_t = std::unordered_map<std::string, value>;

} // namespace mino::core::json

