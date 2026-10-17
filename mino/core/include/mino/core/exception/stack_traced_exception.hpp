#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace mino::core::exception {

    class stack_traced_exception : public std::runtime_error {
    public:
        explicit stack_traced_exception(const std::string& message);
        virtual ~stack_traced_exception() noexcept = default;

        const std::vector<std::string>& call_stack() const noexcept;
        std::string to_string() const;

    private:
        std::vector<std::string> frames_;
        void capture_stack_trace();
    };

} // namespace mino::core::exception
