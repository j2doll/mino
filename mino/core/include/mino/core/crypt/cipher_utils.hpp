#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mino::core::crypt {

    class  cipher_utils {
    public:

        // 헥스 문자열과 바이트 배열 간 변환 유틸리티
        static std::string to_hex(const std::vector<uint8_t>& data);

        // 헥스 문자열을 바이트 배열로 변환 (입력 검증 포함)
        static std::vector<uint8_t> from_hex(const std::string& hex);

        // PKCS#7 패딩 추가 및 제거 유틸리티
        static std::vector<uint8_t> add_padding(const std::string& input, size_t blockSize = 16);

        // PKCS#7 패딩 제거 (입력 검증 포함) 
        static std::string remove_padding(const std::vector<uint8_t>& input);
    };

} // namespace mino::core::crypt
