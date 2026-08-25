#include <cstring>

#include "mino/core/reflect/reflect.hpp"

namespace mino::core::reflect {

    binary_writer::binary_writer() = default;
    binary_writer::~binary_writer() = default;

    void binary_writer::write_bytes(const void* src, size_t size) {
        if (src == nullptr || size == 0) return;
        const auto* ptr = static_cast<const uint8_t*>(src);
        buffer_.insert(buffer_.end(), ptr, ptr + size);
    }

    void binary_writer::write_string(std::string_view str) {
        const auto len = static_cast<uint32_t>(str.size());
        write_bytes(&len, sizeof(len));
        if (len > 0) {
            write_bytes(str.data(), len);
        }
    }

    const std::vector<uint8_t>& binary_writer::get_buffer() const noexcept {
        return buffer_;
    }

    std::vector<uint8_t>& binary_writer::get_buffer() noexcept {
        return buffer_;
    }

    size_t binary_writer::size() const noexcept {
        return buffer_.size();
    }

    void binary_writer::clear() noexcept {
        buffer_.clear();
    }

    binary_reader::binary_reader(const uint8_t* data, size_t size) noexcept
        : data_(data), size_(size), offset_(0), error_(false) {
    }

    binary_reader::~binary_reader() = default;

    bool binary_reader::read_bytes(void* dest, size_t size) {
        if (error_ || offset_ + size > size_) {
            error_ = true;
            return false;
        }
        if (size > 0 && dest != nullptr) {
            std::memcpy(dest, data_ + offset_, size);
        }
        offset_ += size;
        return true;
    }

    bool binary_reader::read_string(std::string& str) {
        uint32_t len = 0;
        if (!read_bytes(&len, sizeof(len))) {
            return false;
        }
        if (offset_ + len > size_) {
            error_ = true;
            return false;
        }
        str.assign(reinterpret_cast<const char*>(data_ + offset_), len);
        offset_ += len;
        return true;
    }

    bool binary_reader::has_error() const noexcept {
        return error_;
    }

    size_t binary_reader::bytes_read() const noexcept {
        return offset_;
    }

} // namespace mino::core::reflect
