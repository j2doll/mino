#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <mutex>
#include <cstdint>

#include <libssh2.h>
#include "mino/network/ethernet.hpp"

namespace mino::network::ssh2 {

    enum class fingerprint_type {
        sha256,
        sha1,
        md5
    };

    // 전역 네트워크 및 libssh2 라이브러리 초기화 관리 (RAII)
    class platform_network_initializer {
    public:
        static void ensure_initialized() {
            static platform_network_initializer instance;
        }

    private:
        platform_network_initializer() {
#ifdef _WIN32
            WSADATA wsa_data;
            if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                throw std::runtime_error("WSAStartup failed");
            }
#endif
            if (::libssh2_init(0) != 0) {
                throw std::runtime_error("libssh2_init failed");
            }
        }

        ~platform_network_initializer() {
            ::libssh2_exit();
#ifdef _WIN32
            ::WSACleanup();
#endif
        }

        platform_network_initializer(const platform_network_initializer&) = delete;
        platform_network_initializer& operator=(const platform_network_initializer&) = delete;
    };

    inline std::string bytes_to_hex(const unsigned char* data, size_t len, char delimiter = ':') {
        if (!data || len == 0) return {};
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            oss << std::setw(2) << static_cast<int>(data[i]);
            if (delimiter != '\0' && i + 1 < len) {
                oss << delimiter;
            }
        }
        return oss.str();
    }

    inline std::string bytes_to_base64(const unsigned char* data, size_t len) {
        static constexpr char b64_table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t val = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < len) val |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < len) val |= static_cast<uint32_t>(data[i + 2]);

            out.push_back(b64_table[(val >> 18) & 0x3F]);
            out.push_back(b64_table[(val >> 12) & 0x3F]);
            out.push_back((i + 1 < len) ? b64_table[(val >> 6) & 0x3F] : '=');
            out.push_back((i + 2 < len) ? b64_table[val & 0x3F] : '=');
        }
        return out;
    }

} // namespace mino::network::ssh
