
#include <vector>

#include "mino/core/crypt/keyless_cipher.hpp"

namespace mino::core::crypt {

    // 익명 네임스페이스를 사용하여 .cpp 파일 내부에서만 접근 가능하도록 은닉
    namespace {
        constexpr std::string_view base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
    }

    std::string keyless_cipher::encrypt(std::string_view plain_text) {
        if (plain_text.empty()) return "";

        // 1단계: 고정 비트 연산을 통한 난독화
        std::string obfuscated;
        obfuscated.reserve(plain_text.size());
        for (char c : plain_text) {
            obfuscated.push_back(static_cast<char>(~c ^ 0xAA));
        }

        // 2단계: Base64 인코딩
        return base64_encode(obfuscated);
    }

    std::string keyless_cipher::decrypt(std::string_view cipher_text) {
        if (cipher_text.empty()) return "";

        // 1단계: Base64 디코딩
        std::string decoded = base64_decode(cipher_text);

        // 2단계: 난독화 역연산
        std::string plain_text;
        plain_text.reserve(decoded.size());
        for (char c : decoded) {
            plain_text.push_back(static_cast<char>(~c ^ 0xAA));
        }

        return plain_text;
    }

    std::string keyless_cipher::base64_encode(std::string_view input) {
        std::string output;
        output.reserve(((input.size() + 2) / 3) * 4);

        int val = 0;
        int val_b = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            val_b += 8;
            while (val_b >= 0) {
                output.push_back(base64_chars[(val >> val_b) & 0x3F]);
                val_b -= 6;
            }
        }
        if (val_b > -6) {
            output.push_back(base64_chars[((val << 8) >> (val_b + 8)) & 0x3F]);
        }
        while (output.size() % 4 != 0) {
            output.push_back('=');
        }
        return output;
    }

    std::string keyless_cipher::base64_decode(std::string_view input) {
        std::vector<int> t(256, -1);
        for (int i = 0; i < 64; i++) t[base64_chars[i]] = i;

        std::string output;
        int val = 0;
        int val_b = -8;
        for (unsigned char c : input) {
            if (t[c] == -1) continue;
            val = (val << 6) + t[c];
            val_b += 6;
            if (val_b >= 0) {
                output.push_back(char((val >> val_b) & 0xFF));
                val_b -= 8;
            }
        }
        return output;
    }

} // namespace mino::core::crypt
