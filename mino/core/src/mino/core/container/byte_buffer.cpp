#include "mino/core/container/byte_buffer.hpp"

#include <algorithm>
#include <cstring>

namespace mino::core::container {

    byte_buffer::byte_buffer(std::size_t initial_capacity)
        : buffer_(initial_capacity), read_pos_(0), write_pos_(0) {
    }

    std::size_t byte_buffer::readable_bytes() const noexcept { return write_pos_ - read_pos_; }
    std::size_t byte_buffer::writable_bytes() const noexcept { return buffer_.size() - write_pos_; }
    std::size_t byte_buffer::capacity() const noexcept { return buffer_.size(); }
    bool byte_buffer::is_empty() const noexcept { return write_pos_ == read_pos_; }

    void byte_buffer::clear() noexcept {
        read_pos_ = 0;
        write_pos_ = 0;
    }

    void byte_buffer::reset(std::size_t new_capacity) {
        clear();
        buffer_.resize(new_capacity);
        buffer_.shrink_to_fit();
    }

    void byte_buffer::write(const void* src, std::size_t size) {
        if (size == 0) return;
        ensure_writable(size);
        std::memcpy(buffer_.data() + write_pos_, src, size);
        write_pos_ += size;
    }

    bool byte_buffer::read(void* dst, std::size_t size) {
        if (readable_bytes() < size) return false;
        std::memcpy(dst, buffer_.data() + read_pos_, size);
        read_pos_ += size;
        if (read_pos_ == write_pos_) {
            clear();
        }
        return true;
    }

    bool byte_buffer::peek(void* dst, std::size_t size) const {
        if (readable_bytes() < size) return false;
        std::memcpy(dst, buffer_.data() + read_pos_, size);
        return true;
    }

    void byte_buffer::compact() noexcept {
        if (read_pos_ == 0) return;
        std::size_t readable = readable_bytes();
        if (readable > 0) {
            std::memmove(buffer_.data(), buffer_.data() + read_pos_, readable);
        }
        read_pos_ = 0;
        write_pos_ = readable;
    }

    void byte_buffer::ensure_writable(std::size_t size) {
        if (writable_bytes() >= size) return;
        if (read_pos_ + writable_bytes() >= size) {
            compact();
        }
        else {
            std::size_t new_cap = std::max(buffer_.size() * 2, write_pos_ + size);
            buffer_.resize(new_cap);
        }
    }

    span<std::byte> byte_buffer::writable_span() noexcept {
        return { reinterpret_cast<std::byte*>(buffer_.data() + write_pos_), writable_bytes() };
    }

    span<const std::byte> byte_buffer::readable_span() const noexcept {
        return { reinterpret_cast<const std::byte*>(buffer_.data() + read_pos_), readable_bytes() };
    }

    void byte_buffer::advance_write(std::size_t size) noexcept {
        write_pos_ = std::min(buffer_.size(), write_pos_ + size);
    }

    void byte_buffer::advance_read(std::size_t size) noexcept {
        read_pos_ = std::min(write_pos_, read_pos_ + size);
        if (read_pos_ == write_pos_) {
            clear();
        }
    }

    // concurrent_byte_buffer 구현
    concurrent_byte_buffer::concurrent_byte_buffer(std::size_t initial_capacity)
        : buffer_(initial_capacity) {
    }

    void concurrent_byte_buffer::clear() noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.clear();
    }

    void concurrent_byte_buffer::reset(std::size_t new_capacity) {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.reset(new_capacity);
    }

    void concurrent_byte_buffer::write(const void* src, std::size_t size) {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.write(src, size);
    }

    bool concurrent_byte_buffer::read(void* dst, std::size_t size) {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.read(dst, size);
    }

    bool concurrent_byte_buffer::peek(void* dst, std::size_t size) const {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.peek(dst, size);
    }

    std::size_t concurrent_byte_buffer::readable_bytes() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.readable_bytes();
    }

    bool concurrent_byte_buffer::is_empty() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.is_empty();
    }

} // namespace mino::core::container
