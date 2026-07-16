#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

#include "mino/core/string/u8.hpp"

// C++ 표준 문자열 타입
// | 타입           | type     | 문자크기 | 인코딩                          |
// | :------------- | :------- | :------- | : ----------------------------- |
// | std::string    | char     | 1 Byte   | ASCII, UTF-8, 멀티바이트 문자열 |
// | std::wstring   | wchar_t  | 2/4 Byte | Windows 2B, Linux/macOS 4B      |
// | std::u16string | char16_t | 2 Byte   | UTF-16 인코딩(플랫폼 무관 고정) |
// | std::u32string | char32_t | 4 Byte   | UTF-32 인코딩(모든 유니코드)    |

// UTF-8 관련 기능을 j2::string::u8 네임스페이스에 모아서 사용
namespace mino::core::string::u8
{
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    // C++20 char8_t 지원 환경 
    using char8 = char8_t;
    using u8string = std::u8string; // C++20부터 std::u8string과 std::u8string_view가 공식 지원됨
    using u8string_view = std::u8string_view;
#else
    // C++17 char8_t 미지원 환경
    using char8 = unsigned char;
    using u8string = std::basic_string<char8>;
    using u8string_view = std::basic_string_view<char8>;
#endif

    // UTF-8 <-> std::string 계열
     u8string     to_u8_string(const std::string& s);
     u8string     to_u8_string(const char* s);
     std::string  to_std_string(const u8string& s);

    // UTF-8 <-> std::wstring
     std::wstring to_wstring(const u8string& s);
     u8string     to_u8_string(const std::wstring& ws);

    // UTF-8 <-> std::u16string
     std::u16string to_u16string(const u8string& s);
     u8string       to_u8_string(const std::u16string& s16);

    // UTF-8 <-> std::u32string
     std::u32string to_u32string(const u8string& s);
     u8string       to_u8_string(const std::u32string& s32);

    // --------------------------------------------------------
    // UTF-8 래퍼 클래스: u8str
    // --------------------------------------------------------
    class   u8str
    {
    public:
        using storage_type = u8string;

        u8str();
        explicit u8str(const char* s);
        explicit u8str(const std::string& s);
        explicit u8str(const u8string& s);
        explicit u8str(u8string&& s);
        explicit u8str(const std::wstring& ws);
        explicit u8str(const std::u16string& s16);
        explicit u8str(const std::u32string& s32);

        const u8string& value() const noexcept;
        u8string& value()       noexcept;

        const char8* data() const noexcept;
        std::size_t  size() const noexcept;
        bool         empty() const noexcept;
        void         clear() noexcept;

        // 변환 함수들
        std::string   to_std_string() const;
        std::wstring  to_wstring() const;
        std::u16string to_u16string() const;
        std::u32string to_u32string() const;

        // from_* 세터
        u8str& from_std_string(const std::string& s);
        u8str& from_cstr(const char* s);
        u8str& from_wstring(const std::wstring& ws);
        u8str& from_u16string(const std::u16string& s16);
        u8str& from_u32string(const std::u32string& s32);

        // 추가된 파싱/세터
        bool to_int64(int64_t& out) const;
        bool to_double(double& out) const;
        bool to_bool(bool& out) const;

        u8str& from_int64(int64_t v);
        u8str& from_double(double v);
        u8str& from_bool(bool v);

        // 검색 계열
        bool starts_with(const u8str& prefix) const;
        bool starts_with(const char* prefix_utf8) const;

        bool ends_with(const u8str& suffix) const;
        bool ends_with(const char* suffix_utf8) const;

        bool contains(const u8str& needle) const;
        bool contains(const char* needle_utf8) const;

        long index_of(const u8str& needle, std::size_t start = 0) const;
        long index_of(const char* needle_utf8, std::size_t start = 0) const;

        long last_index_of(const u8str& needle) const;
        long last_index_of(const char* needle_utf8) const;

        // 공백 제거
        u8str& ltrim();
        u8str& rtrim();
        u8str& trim();

        // 분할
        std::vector<u8str> split(const char* delim_utf8,
            bool skip_empty = false) const;

        // 치환
        u8str& replace(const u8str& from, const u8str& to);
        u8str& replace(const char* from_utf8, const char* to_utf8);

        u8str& replace_all(const u8str& from, const u8str& to);
        u8str& replace_all(const char* from_utf8, const char* to_utf8);

        // UTF-8 코드포인트 기준 부분 문자열
        u8str left(std::size_t n) const;
        u8str right(std::size_t n) const;
        u8str mid(std::size_t pos, std::size_t count) const;
        u8str substr_utf8(std::size_t pos, std::size_t count) const;

        // UTF-8 코드포인트 역순
        u8str& reverse_utf8();

        // 코드포인트 개수 기준 패딩 (fill 은 ASCII 가정)
        u8str& pad_left(std::size_t total_width, char fill = ' ');
        u8str& pad_right(std::size_t total_width, char fill = ' ');

        // prefix / suffix 제거
        bool strip_prefix(const u8str& prefix);
        bool strip_prefix(const char* prefix_utf8);

        bool strip_suffix(const u8str& suffix);
        bool strip_suffix(const char* suffix_utf8);

        // ASCII 대소문자 무시 비교
        bool equals_ignore_case_ascii(const u8str& other) const;
        bool equals_ignore_case_ascii(const char* other_utf8) const;

        // ASCII 기반 대소문자 변환
        u8str& to_lower();
        u8str& to_upper();
        u8str  to_lower_copy() const;
        u8str  to_upper_copy() const;

        // 연산자
        u8str& operator+=(const u8str& o);
        u8str& operator+=(const char* o);

        friend bool operator==(const u8str& a, const u8str& b) noexcept;
        friend bool operator!=(const u8str& a, const u8str& b) noexcept;

    private:
        // UTF-8 코드포인트 인덱스 맵 (code point → {byte_offset, byte_len})
        static std::vector<std::pair<std::size_t, std::size_t>>
            utf8_index_map(const u8string& s);

        u8string value_;
    };

    // 전역 + 연산자
     u8str operator+(const u8str& a, const u8str& b);
     u8str operator+(const u8str& a, const char* b);
     u8str operator+(const char* a, const u8str& b);

}  
