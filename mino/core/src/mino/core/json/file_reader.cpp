#include <fstream>
#include <sstream>
#include <filesystem>

#include "mino/core/json/file_reader.hpp"
#include "mino/core/json/value.hpp"
#include "mino/core/json/parser.hpp"

namespace mino::core::json {

    value file_reader::read_file(const std::string& file_path) noexcept {
        std::error_code ec;
        std::filesystem::path target_path(file_path);

        if (!std::filesystem::exists(target_path, ec) || !std::filesystem::is_regular_file(target_path, ec)) {
            return {};
        }

        std::ifstream file(target_path);
        if (!file.is_open()) {
            return {};
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return parser::parse(buffer.str());
    }

} // namespace mino::core::json
