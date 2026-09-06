#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>
#include <iosfwd>
#include <filesystem>

namespace mino::core::toml {

    // ===========================================================================
    // 공용 데이터 구조 선언
    // ===========================================================================

    // RFC 3339 날짜 및 시간
    struct date_time {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int microsecond = 0;
        bool has_date = false;
        bool has_time = false;
        bool has_offset = false;
        int offset_minutes = 0; // UTC 오프셋 (분 단위)
    };

    struct toml_value;
    using toml_table = std::map<std::string, toml_value>;
    using toml_array = std::vector<toml_value>;

    // TOML 값 래퍼
    struct toml_value {
        using value_type = std::variant<
            std::monostate,
            bool,
            int64_t,
            double,
            std::string,
            date_time,
            toml_array,
            toml_table
        >;

        value_type data;

        template <typename T>
        bool is() const {
            return std::holds_alternative<T>(data);
        }

        template <typename T>
        const T& as() const {
            return std::get<T>(data);
        }

        template <typename T>
        T& as() {
            return std::get<T>(data);
        }
    };

    // ===========================================================================
    // 내부 Lexer 및 Token 선언
    // ===========================================================================

    enum class token_type {
        eof,
        newline,
        equals,
        dot,
        comma,
        lbracket,
        rbracket,
        double_lbracket,
        double_rbracket,
        lbrace,
        rbrace,
        identifier,
        string_literal,
        integer_literal,
        float_literal,
        boolean_literal,
        datetime_literal
    };

    struct token {
        token_type type = token_type::eof;
        std::string text;
        int64_t int_val = 0;
        double float_val = 0.0;
        bool bool_val = false;
        date_time dt_val;
        size_t line = 1;
    };

    class lexer {
    public:
        explicit lexer(std::string_view source);

        token next_token();

    private:
        char peek(int offset = 0) const;
        char advance();
        token parse_string();
        token parse_bare_or_literal();

        static void append_utf8(std::string& out, uint32_t cp);
        static bool parse_rfc3339(const std::string& s, date_time& dt);

        std::string_view src;
        size_t pos = 0;
        size_t cur_line = 1;
    };

    // ===========================================================================
    // 내부 Parser 선언
    // ===========================================================================

    class parser {
    public:
        explicit parser(std::string_view src);

        toml_table parse();

    private:
        void advance();
        void skip_newlines();
        void consume(token_type type, const std::string& err);

        std::vector<std::string> parse_key_path();
        toml_value parse_value();
        toml_value parse_array();
        toml_value parse_inline_table();

        static void assign_value_at_path(toml_table& root, const std::vector<std::string>& path, toml_value val);
        static toml_table& navigate_to_table(toml_table& root, const std::vector<std::string>& path);
        static toml_table& append_array_table(toml_table& root, const std::vector<std::string>& path);

        lexer lex;
        token cur_tok;
    };

    // ===========================================================================
    // 직렬화 보조 함수 선언
    // ===========================================================================

    void serialize_date_time(std::ostream& os, const date_time& dt);
    void serialize_string(std::ostream& os, const std::string& str);
    void serialize_value(std::ostream& os, const toml_value& val);
    void dump_table_recursive(std::ostream& os, const toml_table& tbl, const std::string& prefix);

    // ===========================================================================
    // 공개 API 선언
    // ===========================================================================

    toml_table parse_toml(std::string_view content);

    // 파일 로드 (문자열 경로 및 std::filesystem::path 지원)
    toml_table load_file(const std::string& path);
    toml_table load_file(const std::filesystem::path& path);

    std::string dump_toml(const toml_table& root);

    // 파일 저장 (문자열 경로 및 std::filesystem::path 지원)
    bool dump_file(const std::string& path, const toml_table& root);
    bool dump_file(const std::filesystem::path& path, const toml_table& root);

} // namespace mino::core::toml
