#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "mino/core/container/span.hpp"

namespace mino::core::container {

    class byte_buffer {
    public:
        explicit byte_buffer(std::size_t initial_capacity = 4096);

        std::size_t readable_bytes() const noexcept;
        std::size_t writable_bytes() const noexcept;
        std::size_t capacity() const noexcept;
        bool is_empty() const noexcept;

        void clear() noexcept;
        void reset(std::size_t new_capacity = 4096);

        void write(const void* src, std::size_t size);
        bool read(void* dst, std::size_t size);
        bool peek(void* dst, std::size_t size) const;

        void compact() noexcept;
        void ensure_writable(std::size_t size);

        span<std::byte> writable_span() noexcept;
        span<const std::byte> readable_span() const noexcept;

        void advance_write(std::size_t size) noexcept;
        void advance_read(std::size_t size) noexcept;

    private:
        std::vector<std::uint8_t> buffer_;
        std::size_t read_pos_{ 0 };
        std::size_t write_pos_{ 0 };
    };

    class concurrent_byte_buffer {
    public:
        explicit concurrent_byte_buffer(std::size_t initial_capacity = 4096);

        void clear() noexcept;
        void reset(std::size_t new_capacity = 4096);

        void write(const void* src, std::size_t size);
        bool read(void* dst, std::size_t size);
        bool peek(void* dst, std::size_t size) const;

        std::size_t readable_bytes() const noexcept;
        bool is_empty() const noexcept;

        template <typename Func>
        auto with_lock(Func&& fn) -> decltype(fn(std::declval<byte_buffer&>())) {
            std::lock_guard<std::mutex> lock(mtx_);
            return fn(buffer_);
        }

    private:
        mutable std::mutex mtx_;
        byte_buffer buffer_;
    };

} // namespace mino::core::container
