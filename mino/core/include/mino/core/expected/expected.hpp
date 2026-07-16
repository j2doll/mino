#pragma once

#include <variant>
#include <utility>
#include <stdexcept>

namespace mino::core::expected
{
    // C++17 스타일의 expected<T, E> 구현

    /* ===============================
     * unexpected<E>
     * =============================== */
    template<typename E>
    class unexpected_value {
    public:
        explicit unexpected_value(E e) : error_(std::move(e)) {}
        const E& error() const& { return error_; }
        E& error()& { return error_; }

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

        T& value()
        {
            if (!has_value())
                throw std::logic_error("expected: no value");
            return std::get<T>(storage_);
        }

        const T& value() const
        {
            if (!has_value())
                throw std::logic_error("expected: no value");
            return std::get<T>(storage_);
        }

        /* ----- 에러 접근 ----- */

        E& error()
        {
            if (has_value())
                throw std::logic_error("expected: no error");
            return std::get<E>(storage_);
        }

        const E& error() const
        {
            if (has_value())
                throw std::logic_error("expected: no error");
            return std::get<E>(storage_);
        }

    private:
        std::variant<T, E> storage_;
    };

} // namespace mino::core::expected