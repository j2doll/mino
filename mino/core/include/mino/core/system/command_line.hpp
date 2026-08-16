#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace mino::core::system {

        /**
         * @brief 간단한 명령행 인자 파서
         *
         * 지원:
         *  - 장문 옵션: --name 또는 --name=value
         *  - 단문 옵션: -n 또는 -n value
         *  - 플래그 (requires_value == false)
         *  - 위치 인자
         */
        class  command_line {
        public:
            struct option_def {
                std::string long_name;
                char short_name{ '\0' };
                bool requires_value{ false };
                std::string description;
            };

            command_line() = default;

            // 옵션 등록
            void add_option(const std::string& long_name,
                            char short_name = '\0',
                            bool requires_value = false,
                            const std::string& description = "");

            // 프로그램 버전 설정 (있으면 --version 처리)
            void set_version(const std::string& ver);

            // 파싱 수행
            bool parse(int argc, char* argv[]);

            // 옵션 존재/값 조회
            bool has(const std::string& name) const;
            std::string get(const std::string& name, const std::string& default_val = "") const;

            // 위치 인자 반환
            std::vector<std::string> positional() const;

            // 사용법 문자열 생성
            std::string usage() const;

            // 등록된 옵션 정의를 열거하기 위한 접근자 (읽기 전용)
            const std::vector<option_def>& options() const;

        private:
            std::vector<option_def> m_defs;
            std::unordered_map<std::string, std::string> m_values; // long_name -> value (빈 값은 플래그)
            std::unordered_map<char, std::string> m_short_to_long;
            std::vector<std::string> m_positionals;
            std::string m_program_name;
            std::string m_version;
        };

} 
