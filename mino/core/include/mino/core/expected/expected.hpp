#pragma once

#include <variant>
#include <utility>
#include <cassert>

namespace mino::core::expected
{
    // C++17 스타일의 expected<T, E> 구현 (Non-throwing 버전을 적용)

    /* ===============================
     * unexpected<E>
     * =============================== */
    template<typename E>
    class unexpected_value {
    public:
        explicit unexpected_value(E e) : error_(std::move(e)) {}
        const E& error() const& noexcept { return error_; }
        E& error() & noexcept { return error_; }

    private:
        E error_;
    };

    /* ===============================
     * expected<T, E>
     * =============================== */
    template <typename T, typename E>
    class expected
    {
    public:
        /* ----- 생성자 ----- */

        expected(const T& value)
            : storage_(value)
        {
        }

        expected(T&& value)
            : storage_(std::move(value))
        {
        }

        expected(const unexpected_value<E>& unexp)
            : storage_(unexp.error())
        {
        }

        expected(unexpected_value<E>&& unexp)
            : storage_(std::move(unexp.error()))
        {
        }

        /* ----- 상태 확인 ----- */

        bool has_value() const noexcept
        {
            return std::holds_alternative<T>(storage_);
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        /* ----- 값 접근 ----- */

        T& value() noexcept
        {
            assert(has_value() && "expected: no value");
            return std::get<T>(storage_);
        }

        const T& value() const noexcept
        {
            assert(has_value() && "expected: no value");
            return std::get<T>(storage_);
        }

        /* ----- 기본값 기반 안전 접근 ----- */

        template <typename U>
        T value_or(U&& default_value) const&
        {
            return has_value() ? std::get<T>(storage_) : static_cast<T>(std::forward<U>(default_value));
        }

        template <typename U>
        T value_or(U&& default_value)&&
        {
            return has_value() ? std::move(std::get<T>(storage_)) : static_cast<T>(std::forward<U>(default_value));
        }

        /* ----- 에러 접근 ----- */

        E& error() noexcept
        {
            assert(!has_value() && "expected: no error");
            return std::get<E>(storage_);
        }

        const E& error() const noexcept
        {
            assert(!has_value() && "expected: no error");
            return std::get<E>(storage_);
        }

    private:
        std::variant<T, E> storage_;
    };

} // namespace mino::core::expected
