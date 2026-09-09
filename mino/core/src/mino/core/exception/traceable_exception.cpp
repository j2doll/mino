#include "mino/core/exception/traceable_exception.hpp"

namespace mino::core::exception {

    traceable_exception::traceable_exception(
        const std::string& msg,
        const char* file,
        int line,
        std::exception_ptr orig
    )
        : std::runtime_error(msg + " [" + file + ":" + std::to_string(line) + "]"),
        file_name_(file),
        line_number_(line),
        original_exception_(orig) {
    }

    const char* traceable_exception::file_name() const noexcept {
        return file_name_;
    }

    int traceable_exception::line_number() const noexcept {
        return line_number_;
    }

    std::exception_ptr traceable_exception::original_exception() const noexcept {
        return original_exception_;
    }

} // namespace mino::core::exception
