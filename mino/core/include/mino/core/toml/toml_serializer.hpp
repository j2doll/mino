#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>
#include <iosfwd>
#include <filesystem>
#include <optional>

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
        bool is() const noexcept {
            return std::holds_alternative<T>(data);
        }

        template <typename T>
        const T* get_if() const noexcept {
            return std::get_if<T>(&data);
        }

        template <typename T>
        T* get_if() noexcept {
            return std::get_if<T>(&data);
        }

        template <typename T>
        const T& as() const noexcept {
            return *std::get_if<T>(&data);
        }

        template <typename T>
        T& as() noexcept {
            return *std::get_if<T>(&data);
        }
    };

    // ===========================================================================
    // 내부 Lexer 및 Token 선언
    // ===========================================================================

    enum class token_type {
        eof,
        error,
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
        explicit lexer(std::string_view source) noexcept;

        token next_token() noexcept;

    private:
        char peek(int offset = 0) const noexcept;
        char advance() noexcept;
        token parse_string() noexcept;
        token parse_bare_or_literal() noexcept;

        static void append_utf8(std::string& out, uint32_t cp) noexcept;
        static bool parse_rfc3339(const std::string& s, date_time& dt) noexcept;

        std::string_view src;
        size_t pos = 0;
        size_t cur_line = 1;
    };

    // ===========================================================================
    // 내부 Parser 선언
    // ===========================================================================

    class parser {
    public:
        explicit parser(std::string_view src) noexcept;

        std::optional<toml_table> parse() noexcept;

    private:
        void advance() noexcept;
        void skip_newlines() noexcept;
        bool consume(token_type type) noexcept;

        std::optional<std::vector<std::string>> parse_key_path() noexcept;
        std::optional<toml_value> parse_value() noexcept;
        std::optional<toml_value> parse_array() noexcept;
        std::optional<toml_value> parse_inline_table() noexcept;

        static void assign_value_at_path(toml_table& root, const std::vector<std::string>& path, toml_value val) noexcept;
        static toml_table& navigate_to_table(toml_table& root, const std::vector<std::string>& path) noexcept;
        static toml_table& append_array_table(toml_table& root, const std::vector<std::string>& path) noexcept;

        lexer lex;
        token cur_tok;
        bool has_error_ = false;
    };

    // ===========================================================================
    // 직렬화 보조 함수 선언
    // ===========================================================================

    void serialize_date_time(std::ostream& os, const date_time& dt) noexcept;
    void serialize_string(std::ostream& os, const std::string& str) noexcept;
    void serialize_value(std::ostream& os, const toml_value& val) noexcept;
    void dump_table_recursive(std::ostream& os, const toml_table& tbl, const std::string& prefix) noexcept;

    // ===========================================================================
    // 공개 API 선언
    // ===========================================================================

    std::optional<toml_table> parse_toml(std::string_view content) noexcept;

    // 파일 로드 (실패 시 std::nullopt 반환)
    std::optional<toml_table> load_file(const std::string& path) noexcept;
    std::optional<toml_table> load_file(const std::filesystem::path& path) noexcept;

    std::string dump_toml(const toml_table& root) noexcept;

    // 파일 저장
    bool dump_file(const std::string& path, const toml_table& root) noexcept;
    bool dump_file(const std::filesystem::path& path, const toml_table& root) noexcept;

} // namespace mino::core::toml
