#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace mino::core::exception {

    class traceable_exception : public std::runtime_error {
    public:
        traceable_exception(
            const std::string& msg,
            const char* file,
            int line,
            std::exception_ptr orig = nullptr
        );

        [[nodiscard]] const char* file_name() const noexcept;
        [[nodiscard]] int line_number() const noexcept;
        [[nodiscard]] std::exception_ptr original_exception() const noexcept;

    private:
        const char* file_name_;
        int line_number_;
        std::exception_ptr original_exception_;
    };

    // 템플릿 함수는 헤더에 위치해야 컴파일 타임 인스턴스화가 가능합니다.
    template <typename Fn>
    decltype(auto) invoke_with_trace(const char* file, int line, Fn&& fn) {
        try {
            return fn();
        }
        catch (const std::exception& e) {
            throw traceable_exception(e.what(), file, line, std::current_exception());
        }
        catch (...) {
            throw traceable_exception("unknown non-std exception", file, line, std::current_exception());
        }
    }

} // namespace mino::core::exception

// 네임스페이스 경로를 완전히 포함한 단축 매크로
#define MINO_X(...) \
    ::mino::core::exception::invoke_with_trace( \
        __FILE__, __LINE__, [&]() -> decltype(auto) { return (__VA_ARGS__); } \
    )
