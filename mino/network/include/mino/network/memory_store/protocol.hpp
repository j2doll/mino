#pragma once

#include <string>
#include <vector>
#include <cctype>

namespace mino::network::memory_store {

    inline std::vector<std::string> parse_command(const std::string& raw_cmd) {
        std::vector<std::string> tokens;
        std::string token;
        bool in_quotes = false;

        for (size_t i = 0; i < raw_cmd.size(); ++i) {
            char ch = raw_cmd[i];
            if (ch == '"') {
                in_quotes = !in_quotes;
                continue;
            }
            if (std::isspace(static_cast<unsigned char>(ch)) && !in_quotes) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            }
            else {
                token += ch;
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // 간단한 키 유효성 검사: 비어있지 않고 공백, 큰따옴표, 제어문자가 없어야 함
    inline bool is_valid_key(const std::string& key) {
        if (key.empty()) return false;
        for (unsigned char ch : key) {
            if (std::isspace(ch)) return false; // 스페이스, 탭, 개행 등 모두 금지
            if (ch == '"') return false;       // 따옴표는 프로토콜에서 특별 처리
            if (ch == '\0') return false;
            if (ch < 0x20) return false;      // 기타 제어문자 금지
        }
        return true;
    }

} 
