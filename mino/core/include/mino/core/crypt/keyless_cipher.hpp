#pragma once

#include <string>
#include <string_view>

namespace mino::core::crypt {

    class  keyless_cipher {
    public:
        // 암호화 (인코딩 + 난독화)
        static std::string encrypt(std::string_view plain_text);

        // 복호화 (디코딩 + 역난독화)
        static std::string decrypt(std::string_view cipher_text);

    private:
        static std::string base64_encode(std::string_view input);
        static std::string base64_decode(std::string_view input);
    };

} // namespace mino::core::crypt

