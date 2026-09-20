#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mino::network::ssh {

    class ssh_buffer {
    private:
        std::vector<uint8_t> data_;
        size_t rpos_{ 0 };

    public:
        ssh_buffer();
        ssh_buffer(const uint8_t* b, size_t len);
        explicit ssh_buffer(std::vector<uint8_t> v);

        const std::vector<uint8_t>& data() const;
        size_t size() const;

        void write_byte(uint8_t b);
        void write_uint32(uint32_t val);
        void write_uint64(uint64_t val);
        void write_string(const std::string& str);
        void write_bytes(const uint8_t* b, size_t len);
        void write_raw(const uint8_t* b, size_t len);
        void write_mpint(const uint8_t* b, size_t len);

        uint8_t read_byte();
        uint32_t read_uint32();
        uint64_t read_uint64();
        std::string read_string();
        std::vector<uint8_t> read_bytes();
    };

} // namespace mino::network::ssh
