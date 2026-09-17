#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cstdint>

namespace mino::core::cli {

    enum class string_encoding {
        utf8,
        cp949,
        iso2022kr,
        johab,
        mackorean,
        console
    };

    struct option_def {
        std::string short_opt;
        std::string long_opt;
        std::string description;
        bool has_value = false;
        std::string default_val;

        // 문자열 관련 옵션
        bool is_case_sensitive = true;
        string_encoding encoding = string_encoding::utf8;

        // 실수 관련 옵션
        std::optional<int> precision = std::nullopt;
        bool allow_inf = true;
        bool allow_nan = true;
    };

    class option_builder {
    public:
        explicit option_builder(option_def& opt);

        option_builder& case_sensitive(bool enable);
        option_builder& encoding(string_encoding enc);
        option_builder& precision(int decimal_places);
        option_builder& allow_inf(bool allow);
        option_builder& allow_nan(bool allow);

    private:
        option_def& opt_;
    };

    class arg_parser {
    public:
        explicit arg_parser(std::string program_desc = "");

        option_builder add_option( // 옵션 추가
            const std::string& short_opt, // 단축 옵션 (예: "-h")
            const std::string& long_opt, // 전체 옵션 (예: "--help")
            const std::string& description, // 옵션 설명
            bool has_value = false, // 옵션이 값을 가지는지 여부 (예: --port=8080 같은 경우, 값이 있음)
            const std::string& default_val = ""); // 옵션의 기본값 (has_value가 true일 때만 의미 있음)

        bool parse(int argc, char* argv[]); // 명령행 인자 파싱

        bool has_flag(const std::string& long_opt) const; // 플래그 옵션 존재 여부 확인 (long_opt: "--help" 같은 전체 옵션 이름)

        const std::vector<std::string>& get_positional() const; // 위치 인자(옵션이 아닌 인자) 반환

        void print_help() const; // 도움말 출력

        const std::string& get_error() const; // 마지막 파싱 에러 메시지 반환

        // 타입별 명시적 getter 함수. (long_opt: "--port" 같은 전체 옵션 이름)
        std::optional<std::string> get_string(const std::string& long_opt) const;
        std::optional<bool>        get_bool(const std::string& long_opt) const;
        std::optional<int>         get_int(const std::string& long_opt) const;
        std::optional<int64_t>     get_int64(const std::string& long_opt) const;
        std::optional<float>       get_float(const std::string& long_opt) const;
        std::optional<double>      get_double(const std::string& long_opt) const;

    private:
        std::optional<float>  parse_float(const std::string& str, const option_def& def) const;
        std::optional<double> parse_double(const std::string& str, const option_def& def) const;

        static bool convert_to_utf8(const std::string& input, string_encoding enc, std::string& out);

        std::string program_name_;
        std::string program_description_;
        std::string last_error_;

        std::vector<option_def> options_;
        std::unordered_map<std::string, size_t> short_map_;
        std::unordered_map<std::string, size_t> long_map_;
        std::unordered_map<std::string, std::string> parsed_values_;
        std::unordered_set<std::string> flags_; // 플래그 옵션 저장
        std::vector<std::string> positional_args_;
    };

} // namespace mino::core::cli
