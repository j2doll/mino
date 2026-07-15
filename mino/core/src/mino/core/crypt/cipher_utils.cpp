#include <iomanip>
#include <sstream>

#include "mino/core/crypt/cipher_utils.hpp"

namespace mino::core::crypt {

        std::string cipher_utils::to_hex(const std::vector<uint8_t>& data) {
            std::stringstream ss;
            ss << std::hex << std::setfill('0');
            for (auto b : data) ss << std::setw(2) << static_cast<int>(b);
            return ss.str();
        }

        std::vector<uint8_t> cipher_utils::from_hex(const std::string& hex) {
            std::vector<uint8_t> data;
            if (hex.length() % 2 != 0) return data;
            for (size_t i = 0; i < hex.length(); i += 2) {
                data.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
            }
            return data;
        }

        std::vector<uint8_t> cipher_utils::add_padding(const std::string& input, size_t blockSize) {
            size_t paddingLen = blockSize - (input.length() % blockSize);
            std::vector<uint8_t> data(input.begin(), input.end());
            for (size_t i = 0; i < paddingLen; ++i) data.push_back(static_cast<uint8_t>(paddingLen));
            return data;
        }

        std::string cipher_utils::remove_padding(const std::vector<uint8_t>& input) {
            if (input.empty()) return "";
            uint8_t paddingLen = input.back();
            if (paddingLen == 0 || paddingLen > 16 || paddingLen > input.size()) return "";
            for (size_t i = 0; i < paddingLen; ++i) {
                if (input[input.size() - 1 - i] != paddingLen) return "";
            }
            return std::string(input.begin(), input.end() - paddingLen);
        }

} // namespace mino::core::crypt
