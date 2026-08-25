#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <variant>
#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <filesystem>

namespace mino::core::yaml {

    struct node;
    // 순서 보존을 위해 std::vector<std::pair> 기반으로 변경
    using sequence = std::vector<node>;
    using mapping = std::vector<std::pair<std::string, node>>;
    using null_type = std::monostate;

    // -------------------------------------------------------------
    // 1. node 구조체
    // -------------------------------------------------------------
    struct node {
        std::variant<null_type, bool, int64_t, double, std::string, sequence, mapping> data;

        enum class node_type {
            null_val,
            boolean,
            integer,
            floating,
            string,
            sequence,
            mapping
        };

        // node 구조체 내부 메서드
        node_type type() const {
            return static_cast<node_type>(data.index());
        }

        std::string_view type_name() const {
            switch (type()) {
                case node_type::null_val:  return "null";
                case node_type::boolean:   return "bool";
                case node_type::integer:   return "int";
                case node_type::floating:  return "double";
                case node_type::string:    return "string";
                case node_type::sequence:  return "sequence";
                case node_type::mapping:   return "mapping";
            }
            return "unknown";
        }

        // 생성자
        node();
        node(null_type v);
        node(bool v);
        node(int64_t v);
        node(int v);
        node(double v);
        node(std::string v);
        node(const char* v);
        node(sequence v);
        node(mapping v);

        // 타입 확인 메서드
        bool is_null()   const;
        bool is_bool()   const;
        bool is_int()    const;
        bool is_double() const;
        bool is_string() const;
        bool is_scalar() const;
        bool is_seq()    const;
        bool is_map()    const;

        // 상태 및 키 검사
        bool has_key(const std::string& key) const;
        size_t size() const;

        // 데이터 접근 연산자
        node& operator[](const std::string& key);
        const node& operator[](const std::string& key) const;
        node& operator[](size_t index);
        const node& operator[](size_t index) const;

        void push_back(node item);

        // 템플릿 변환 함수
        template <typename T>
        const T& as() const {
            return std::get<T>(data);
        }

        template <typename T>
        T& as() {
            return std::get<T>(data);
        }
    };

    // -------------------------------------------------------------
    // 2. parser 클래스
    // -------------------------------------------------------------
    class parser {
    public:
        static node parse(std::string_view yaml_str);
        static node parse_file(const std::filesystem::path& file_path);

    private:
        struct line {
            size_t indent = 0;
            std::string_view text;
        };

        static std::string_view trim(std::string_view s);
        static node parse_scalar(std::string_view sv);
        static std::vector<line> tokenize_lines(std::string_view src);
        static node parse_block(const std::vector<line>& lines, size_t& idx, size_t base_indent);
    };

    // -------------------------------------------------------------
    // 3. emitter 클래스
    // -------------------------------------------------------------
    class emitter {
    public:
        static std::string dump(const node& root);
        static void dump(const node& root, std::ostream& out);
        static void dump_file(const node& root, const std::filesystem::path& file_path);

    private:
        static void emit_node(const node& n, std::ostream& out, int indent_level);
    };

    // -------------------------------------------------------------
    // 4. 공개 편의 API 함수
    // -------------------------------------------------------------
    inline node parse_yaml(std::string_view yaml_str) {
        return parser::parse(yaml_str);
    }

    inline node parse_yaml_file(const std::filesystem::path& path) {
        return parser::parse_file(path);
    }

    inline std::string dump_yaml(const node& root) {
        return emitter::dump(root);
    }

    inline void dump_yaml_file(const node& root, const std::filesystem::path& path) {
        emitter::dump_file(root, path);
    }

} // namespace mino::core::yaml
