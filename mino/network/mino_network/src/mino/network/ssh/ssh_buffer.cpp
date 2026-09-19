#include "mino/network/ssh/ssh_buffer.hpp"
#include <stdexcept>

namespace mino::network::ssh {

    ssh_buffer::ssh_buffer() = default;

    ssh_buffer::ssh_buffer(const uint8_t* b, size_t len) : data_(b, b + len) {}

    ssh_buffer::ssh_buffer(std::vector<uint8_t> v) : data_(std::move(v)) {}

    const std::vector<uint8_t>& ssh_buffer::data() const {
        return data_;
    }

    size_t ssh_buffer::size() const {
        return data_.size();
    }

    void ssh_buffer::write_byte(uint8_t b) {
        data_.push_back(b);
    }

    void ssh_buffer::write_uint32(uint32_t val) {
        data_.push_back((val >> 24) & 0xFF);
        data_.push_back((val >> 16) & 0xFF);
        data_.push_back((val >> 8) & 0xFF);
        data_.push_back(val & 0xFF);
    }

    void ssh_buffer::write_string(const std::string& str) {
        write_uint32(static_cast<uint32_t>(str.size()));
        data_.insert(data_.end(), str.begin(), str.end());
    }

    void ssh_buffer::write_bytes(const uint8_t* b, size_t len) {
        write_uint32(static_cast<uint32_t>(len));
        data_.insert(data_.end(), b, b + len);
    }

    void ssh_buffer::write_raw(const uint8_t* b, size_t len) {
        data_.insert(data_.end(), b, b + len);
    }

    void ssh_buffer::write_mpint(const uint8_t* b, size_t len) {
        if (len == 0) {
            write_uint32(0);
            return;
        }

        size_t start = 0;
        while (start < len && b[start] == 0) {
            start++;
        }

        if (start == len) {
            write_uint32(0);
            return;
        }

        size_t actual_len = len - start;
        if (b[start] & 0x80) {
            write_uint32(static_cast<uint32_t>(actual_len + 1));
            data_.push_back(0x00);
            data_.insert(data_.end(), b + start, b + len);
        }
        else {
            write_uint32(static_cast<uint32_t>(actual_len));
            data_.insert(data_.end(), b + start, b + len);
        }
    }

    uint8_t ssh_buffer::read_byte() {
        if (rpos_ >= data_.size()) throw std::runtime_error("Buffer underflow (byte)");
        return data_[rpos_++];
    }

    uint32_t ssh_buffer::read_uint32() {
        if (rpos_ + 4 > data_.size()) throw std::runtime_error("Buffer underflow (uint32)");
        uint32_t val = (static_cast<uint32_t>(data_[rpos_]) << 24) |
            (static_cast<uint32_t>(data_[rpos_ + 1]) << 16) |
            (static_cast<uint32_t>(data_[rpos_ + 2]) << 8) |
            static_cast<uint32_t>(data_[rpos_ + 3]);
        rpos_ += 4;
        return val;
    }

    std::string ssh_buffer::read_string() {
        uint32_t len = read_uint32();
        if (rpos_ + len > data_.size()) throw std::runtime_error("Buffer underflow (string)");
        std::string s(data_.begin() + rpos_, data_.begin() + rpos_ + len);
        rpos_ += len;
        return s;
    }

    std::vector<uint8_t> ssh_buffer::read_bytes() {
        uint32_t len = read_uint32();
        if (rpos_ + len > data_.size()) throw std::runtime_error("Buffer underflow (bytes)");
        std::vector<uint8_t> v(data_.begin() + rpos_, data_.begin() + rpos_ + len);
        rpos_ += len;
        return v;
    }

} // namespace mino::network::ssh
