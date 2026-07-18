#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#   include <windows.h>
#else
#   include <locale>
#endif

#include "mino/core/string/u8string.hpp"
#include "mino/core/string/string_basic.hpp"
#include "mino/core/string/to_string.hpp"

// UTF-8 내부 처리용 detail 네임스페이스
namespace mino::core::string::u8
{
    namespace detail
    {
        // UTF-8 코드포인트 디코더
        bool next_code_point(
            const unsigned char* data,
            std::size_t size,
            std::size_t& index,
            char32_t& out_cp)
        {
            if (index >= size)
                return false;

            unsigned char b0 = data[index];

            // 1바이트 (ASCII)
            if ((b0 & 0b10000000u) == 0)
            {
                out_cp = static_cast<char32_t>(b0);
                ++index;
                return true;
            }

            // 2바이트
            if ((b0 & 0b11100000u) == 0b11000000u)
            {
                if (index + 1 >= size)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }
                unsigned char b1 = data[index + 1];
                if ((b1 & 0b11000000u) != 0b10000000u)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }
                char32_t cp = ((b0 & 0b00011111u) << 6) |
                    (b1 & 0b00111111u);
                out_cp = (cp < 0x80) ? 0xFFFD : cp;
                index += 2;
                return true;
            }

            // 3바이트
            if ((b0 & 0b11110000u) == 0b11100000u)
            {
                if (index + 2 >= size)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }
                unsigned char b1 = data[index + 1];
                unsigned char b2 = data[index + 2];

                if ((b1 & 0b11000000u) != 0b10000000u ||
                    (b2 & 0b11000000u) != 0b10000000u)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }

                char32_t cp = ((b0 & 0b00001111u) << 12) |
                    ((b1 & 0b00111111u) << 6) |
                    (b2 & 0b00111111u);

                if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))
                    out_cp = 0xFFFD;
                else
                    out_cp = cp;

                index += 3;
                return true;
            }

            // 4바이트
            if ((b0 & 0b11111000u) == 0b11110000u)
            {
                if (index + 3 >= size)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }
                unsigned char b1 = data[index + 1];
                unsigned char b2 = data[index + 2];
                unsigned char b3 = data[index + 3];

                if ((b1 & 0b11000000u) != 0b10000000u ||
                    (b2 & 0b11000000u) != 0b10000000u ||
                    (b3 & 0b11000000u) != 0b10000000u)
                {
                    out_cp = 0xFFFD;
                    ++index;
                    return true;
                }

                char32_t cp = ((b0 & 0b00000111u) << 18) |
                    ((b1 & 0b00111111u) << 12) |
                    ((b2 & 0b00111111u) << 6) |
                    (b3 & 0b00111111u);

                if (cp < 0x10000 || cp > 0x10FFFF)
                    out_cp = 0xFFFD;
                else
                    out_cp = cp;

                index += 4;
                return true;
            }

            // 잘못된 리드 바이트
            out_cp = 0xFFFD;
            ++index;
            return true;
        }

        // 코드포인트를 UTF-8로 인코딩
        void append_utf8(char32_t cp, u8string& out)
        {
            auto push = [&](unsigned char b)
                {
                    out.push_back(static_cast<char8>(b));
                };

            if (cp <= 0x7F)
            {
                push(static_cast<unsigned char>(cp));
            }
            else if (cp <= 0x7FF)
            {
                push(static_cast<unsigned char>(0b11000000u | (cp >> 6)));
                push(static_cast<unsigned char>(0b10000000u | (cp & 0b00111111u)));
            }
            else if (cp <= 0xFFFF)
            {
                if (cp >= 0xD800 && cp <= 0xDFFF)
                    cp = 0xFFFD;

                push(static_cast<unsigned char>(0b11100000u | (cp >> 12)));
                push(static_cast<unsigned char>(0b10000000u | ((cp >> 6) & 0b00111111u)));
                push(static_cast<unsigned char>(0b10000000u | (cp & 0b00111111u)));
            }
            else if (cp <= 0x10FFFF)
            {
                push(static_cast<unsigned char>(0b11110000u | (cp >> 18)));
                push(static_cast<unsigned char>(0b10000000u | ((cp >> 12) & 0b00111111u)));
                push(static_cast<unsigned char>(0b10000000u | ((cp >> 6) & 0b00111111u)));
                push(static_cast<unsigned char>(0b10000000u | (cp & 0b00111111u)));
            }
            else
            {
                append_utf8(0xFFFD, out);
            }
        }

        // ASCII 공백 체크
        bool is_ascii_space(unsigned char c)
        {
            return c == ' ' || c == '\t' ||
                c == '\n' || c == '\r' ||
                c == '\f' || c == '\v';
        }

        // ASCII 소문자로 변환
        unsigned char to_lower_ascii(unsigned char c)
        {
            if (c >= 'A' && c <= 'Z')
                return static_cast<unsigned char>(c + ('a' - 'A'));
            return c;
        }

    } // namespace detail

    // --------------------------------------------------------
    // UTF-8 <-> std::string
    // --------------------------------------------------------
    u8string to_u8_string(const std::string& s)
    {
        return u8string(
            reinterpret_cast<const char8*>(s.data()),
            reinterpret_cast<const char8*>(s.data() + s.size()));
    }

    u8string to_u8_string(const char* s)
    {
        return to_u8_string(std::string(s));
    }

    std::string to_std_string(const u8string& s)
    {
        return std::string(
            reinterpret_cast<const char*>(s.data()),
            reinterpret_cast<const char*>(s.data() + s.size()));
    }

    // --------------------------------------------------------
    // UTF-8 <-> std::wstring
    // --------------------------------------------------------
    std::wstring to_wstring(const u8string& s)
    {
        if (s.empty())
            return std::wstring();

#if defined(_WIN32)
        const char* p = reinterpret_cast<const char*>(s.data());
        int len = static_cast<int>(s.size());

        int req = MultiByteToWideChar(
            CP_UTF8,
            0,
            p,
            len,
            nullptr,
            0);
        if (req <= 0)
            return std::wstring();

        std::wstring w(req, L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            0,
            p,
            len,
            &w[0],
            req);
        return w;
#else
        // Portable conversion without deprecated codecvt:
        // If wchar_t is 2 bytes, convert via UTF-16; if 4 bytes, via UTF-32.
        if constexpr (sizeof(wchar_t) == 2)
        {
            std::u16string s16 = to_u16string(s);
            std::wstring w;
            w.reserve(s16.size());
            for (char16_t c : s16)
                w.push_back(static_cast<wchar_t>(c));
            return w;
        }
        else
        {
            std::u32string s32 = to_u32string(s);
            std::wstring w;
            w.reserve(s32.size());
            for (char32_t c : s32)
                w.push_back(static_cast<wchar_t>(c));
            return w;
        }
#endif
    }


    u8string to_u8_string(const std::wstring& ws)
    {
        if (ws.empty())
            return u8string();

#if defined(_WIN32)
        int req = WideCharToMultiByte(
            CP_UTF8,
            0,
            ws.c_str(),
            static_cast<int>(ws.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (req <= 0) {
            return u8string();
        }

        std::string out(req, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            ws.c_str(),
            static_cast<int>(ws.size()),
            &out[0],
            req,
            nullptr,
            nullptr);

        return to_u8_string(out);
#else
        // Portable conversion without deprecated codecvt:
        // If wchar_t is 2 bytes, treat incoming wchar_t as UTF-16; if 4 bytes, as UTF-32.
        if constexpr (sizeof(wchar_t) == 2)
        {
            std::u16string s16;
            s16.reserve(ws.size());
            for (wchar_t c : ws)
                s16.push_back(static_cast<char16_t>(c));
            return to_u8_string(s16);
        }
        else
        {
            std::u32string s32;
            s32.reserve(ws.size());
            for (wchar_t c : ws)
                s32.push_back(static_cast<char32_t>(c));
            return to_u8_string(s32);
        }
#endif
    }



    // --------------------------------------------------------
    // UTF-16
    // --------------------------------------------------------
    std::u16string to_u16string(const u8string& s)
    {
        std::u16string r;
        if (s.empty())
            return r;

        const unsigned char* data =
            reinterpret_cast<const unsigned char*>(s.data());
        std::size_t index = 0;
        std::size_t size = s.size();
        char32_t cp = 0;

        while (detail::next_code_point(data, size, index, cp))
        {
            if (cp <= 0xFFFF)
            {
                if (cp >= 0xD800 && cp <= 0xDFFF)
                    cp = 0xFFFD;
                r.push_back(static_cast<char16_t>(cp));
            }
            else
            {
                cp -= 0x10000;
                char16_t high = static_cast<char16_t>(0xD800 | ((cp >> 10) & 0x3FF));
                char16_t low = static_cast<char16_t>(0xDC00 | (cp & 0x3FF));
                r.push_back(high);
                r.push_back(low);
            }
        }
        return r;
    }

    u8string to_u8_string(const std::u16string& s16)
    {
        u8string out;
        if (s16.empty())
            return out;

        std::size_t i = 0;
        while (i < s16.size())
        {
            char16_t w1 = s16[i++];
            char32_t cp = 0;

            if (w1 >= 0xD800 && w1 <= 0xDBFF)
            {
                if (i < s16.size())
                {
                    char16_t w2 = s16[i];
                    if (w2 >= 0xDC00 && w2 <= 0xDFFF)
                    {
                        ++i;
                        cp = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;
                    }
                    else
                    {
                        cp = 0xFFFD;
                    }
                }
                else
                {
                    cp = 0xFFFD;
                }
            }
            else if (w1 >= 0xDC00 && w1 <= 0xDFFF)
            {
                cp = 0xFFFD;
            }
            else
            {
                cp = w1;
            }

            detail::append_utf8(cp, out);
        }
        return out;
    }

    // --------------------------------------------------------
    // UTF-32
    // --------------------------------------------------------
    std::u32string to_u32string(const u8string& s)
    {
        std::u32string out;
        if (s.empty())
            return out;

        const unsigned char* data =
            reinterpret_cast<const unsigned char*>(s.data());
        std::size_t index = 0;
        std::size_t size = s.size();
        char32_t cp = 0;

        while (detail::next_code_point(data, size, index, cp))
            out.push_back(cp);

        return out;
    }

    u8string to_u8_string(const std::u32string& s32)
    {
        u8string out;
        if (s32.empty())
            return out;

        for (char32_t cp : s32)
            detail::append_utf8(cp, out);

        return out;
    }

    // --------------------------------------------------------
    // u8str 구현
    // --------------------------------------------------------
    u8str::u8str() = default;

    u8str::u8str(const char* s)
        : value_(to_u8_string(s))
    {
    }

    u8str::u8str(const std::string& s)
        : value_(to_u8_string(s))
    {
    }

    u8str::u8str(const u8string& s)
        : value_(s)
    {
    }

    u8str::u8str(u8string&& s)
        : value_(std::move(s))
    {
    }

    u8str::u8str(const std::wstring& ws)
        : value_(to_u8_string(ws))
    {
    }

    u8str::u8str(const std::u16string& s16)
        : value_(to_u8_string(s16))
    {
    }

    u8str::u8str(const std::u32string& s32)
        : value_(to_u8_string(s32))
    {
    }

    const u8string& u8str::value() const noexcept
    {
        return value_;
    }

    u8string& u8str::value() noexcept
    {
        return value_;
    }

    const char8* u8str::data() const noexcept
    {
        return value_.data();
    }

    std::size_t u8str::size() const noexcept
    {
        return value_.size();
    }

    bool u8str::empty() const noexcept
    {
        return value_.empty();
    }

    void u8str::clear() noexcept
    {
        value_.clear();
    }

    // 변환
    std::string u8str::to_std_string() const
    {
        return mino::core::string::u8::to_std_string(value_);
    }

    std::wstring u8str::to_wstring() const
    {
        return mino::core::string::u8::to_wstring(value_);
    }

    std::u16string u8str::to_u16string() const
    {
        return mino::core::string::u8::to_u16string(value_);
    }

    std::u32string u8str::to_u32string() const
    {
        return mino::core::string::u8::to_u32string(value_);
    }

    // from_* 세터
    u8str& u8str::from_std_string(const std::string& s)
    {
        value_ = to_u8_string(s);
        return *this;
    }

    u8str& u8str::from_cstr(const char* s)
    {
        value_ = to_u8_string(s);
        return *this;
    }

    u8str& u8str::from_wstring(const std::wstring& ws)
    {
        value_ = to_u8_string(ws);
        return *this;
    }

    u8str& u8str::from_u16string(const std::u16string& s16)
    {
        value_ = to_u8_string(s16);
        return *this;
    }

    u8str& u8str::from_u32string(const std::u32string& s32)
    {
        value_ = to_u8_string(s32);
        return *this;
    }

    // --- 추가된 파서 / 세터 구현 ---
    bool u8str::to_int64(int64_t& out) const
    {
        std::string s = to_std_string();
        return mino::core::string::try_parse_int64(s, out);
    }

    bool u8str::to_double(double& out) const
    {
        std::string s = to_std_string();
        return mino::core::string::try_parse_double(s, out);
    }

    bool u8str::to_bool(bool& out) const
    {
        std::string s = to_std_string();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) -> unsigned char {
            return static_cast<unsigned char>(std::tolower(c));
        });
        if (s == "true") { out = true; return true; }
        if (s == "false") { out = false; return true; }
        return false;
    }

    u8str& u8str::from_int64(int64_t v)
    {
        value_ = to_u8_string(std::to_string(v));
        return *this;
    }

    u8str& u8str::from_double(double v)
    {
        // mino::core::string::to_string 템플릿을 사용하여 정밀도 일관성 확보
        std::string s = mino::core::string::to_string<double>(v);
        value_ = to_u8_string(s);
        return *this;
    }

    u8str& u8str::from_bool(bool v)
    {
        value_ = to_u8_string(v ? "true" : "false");
        return *this;
    }
    // --- 추가된 파서 / 세터 구현 끝 ---

    // starts_with / ends_with / contains
    bool u8str::starts_with(const u8str& prefix) const
    {
        if (prefix.size() > size())
            return false;
        return std::equal(prefix.value_.begin(), prefix.value_.end(),
            value_.begin());
    }

    bool u8str::starts_with(const char* prefix_utf8) const
    {
        return starts_with(u8str(prefix_utf8));
    }

    bool u8str::ends_with(const u8str& suffix) const
    {
        if (suffix.size() > size())
            return false;
        std::size_t offset = size() - suffix.size();
        return std::equal(suffix.value_.begin(), suffix.value_.end(),
            value_.begin() + offset);
    }

    bool u8str::ends_with(const char* suffix_utf8) const
    {
        return ends_with(u8str(suffix_utf8));
    }

    bool u8str::contains(const u8str& needle) const
    {
        if (needle.empty())
            return true;
        return value_.find(needle.value_) != u8string::npos;
    }

    bool u8str::contains(const char* needle_utf8) const
    {
        return contains(u8str(needle_utf8));
    }

    // index_of / last_index_of
    long u8str::index_of(const u8str& needle, std::size_t start) const
    {
        if (needle.empty())
            return 0;

        auto pos = value_.find(needle.value_, start);
        return (pos == u8string::npos) ? -1L : static_cast<long>(pos);
    }

    long u8str::index_of(const char* needle_utf8, std::size_t start) const
    {
        return index_of(u8str(needle_utf8), start);
    }

    long u8str::last_index_of(const u8str& needle) const
    {
        if (needle.empty())
            return 0;

        auto pos = value_.rfind(needle.value_);
        return (pos == u8string::npos) ? -1L : static_cast<long>(pos);
    }

    long u8str::last_index_of(const char* needle_utf8) const
    {
        return last_index_of(u8str(needle_utf8));
    }

    // trim 계열
    u8str& u8str::ltrim()
    {
        std::size_t i = 0;
        while (i < value_.size())
        {
            unsigned char c = static_cast<unsigned char>(value_[i]);
            if (!detail::is_ascii_space(c))
                break;
            ++i;
        }
        value_.erase(0, i);
        return *this;
    }

    u8str& u8str::rtrim()
    {
        std::size_t i = value_.size();
        while (i > 0)
        {
            unsigned char c = static_cast<unsigned char>(value_[i - 1]);
            if (!detail::is_ascii_space(c))
                break;
            --i;
        }
        value_.erase(i);
        return *this;
    }

    u8str& u8str::trim()
    {
        return ltrim().rtrim();
    }

    // split
    std::vector<u8str> u8str::split(const char* delim_utf8, bool skip_empty) const
    {
        std::vector<u8str> result;
        u8string delim = to_u8_string(delim_utf8);

        if (delim.empty())
        {
            result.emplace_back(value_);
            return result;
        }

        std::size_t pos = 0;
        while (true)
        {
            std::size_t next = value_.find(delim, pos);
            if (next == u8string::npos)
            {
                u8string tail = value_.substr(pos);
                if (!tail.empty() || !skip_empty)
                    result.emplace_back(tail);
                break;
            }

            u8string part = value_.substr(pos, next - pos);
            if (!part.empty() || !skip_empty)
                result.emplace_back(part);

            pos = next + delim.size();
        }

        return result;
    }

    // replace / replace_all
    u8str& u8str::replace(const u8str& from, const u8str& to)
    {
        if (from.empty())
            return *this;

        auto pos = value_.find(from.value_);
        if (pos != u8string::npos)
            value_.replace(pos, from.size(), to.value_);

        return *this;
    }

    u8str& u8str::replace(const char* from_utf8, const char* to_utf8)
    {
        return replace(u8str(from_utf8), u8str(to_utf8));
    }

    u8str& u8str::replace_all(const u8str& from, const u8str& to)
    {
        if (from.empty())
            return *this;

        std::size_t pos = 0;
        while (true)
        {
            pos = value_.find(from.value_, pos);
            if (pos == u8string::npos)
                break;

            value_.replace(pos, from.size(), to.value_);
            pos += to.size();
        }
        return *this;
    }

    u8str& u8str::replace_all(const char* from_utf8, const char* to_utf8)
    {
        return replace_all(u8str(from_utf8), u8str(to_utf8));
    }

    // UTF-8 코드포인트 인덱스 맵
    std::vector<std::pair<std::size_t, std::size_t>>
        u8str::utf8_index_map(const u8string& s)
    {
        std::vector<std::pair<std::size_t, std::size_t>> map;

        const unsigned char* data =
            reinterpret_cast<const unsigned char*>(s.data());
        std::size_t size = s.size();
        std::size_t index = 0;
        char32_t cp = 0;

        while (index < size)
        {
            std::size_t start = index;
            if (!detail::next_code_point(data, size, index, cp))
                break;
            std::size_t len = index - start;
            map.emplace_back(start, len);
        }

        return map;
    }

    // left / right / mid / substr_utf8 (코드포인트 기준)
    u8str u8str::left(std::size_t n) const
    {
        auto map = utf8_index_map(value_);
        if (map.empty() || n == 0)
            return u8str("");

        if (n >= map.size())
            return u8str(value_);

        std::size_t byte_end = map[n - 1].first + map[n - 1].second;
        return u8str(value_.substr(0, byte_end));
    }

    u8str u8str::right(std::size_t n) const
    {
        auto map = utf8_index_map(value_);
        if (map.empty() || n == 0)
            return u8str("");

        if (n >= map.size())
            return u8str(value_);

        std::size_t start_index = map.size() - n;
        std::size_t byte_start = map[start_index].first;
        return u8str(value_.substr(byte_start));
    }

    u8str u8str::mid(std::size_t pos, std::size_t count) const
    {
        auto map = utf8_index_map(value_);
        if (map.empty() || count == 0 || pos >= map.size())
            return u8str("");

        std::size_t end_pos = pos + count;
        if (end_pos > map.size())
            end_pos = map.size();

        std::size_t byte_start = map[pos].first;
        std::size_t byte_end = map[end_pos - 1].first + map[end_pos - 1].second;

        return u8str(value_.substr(byte_start, byte_end - byte_start));
    }

    u8str u8str::substr_utf8(std::size_t pos, std::size_t count) const
    {
        return mid(pos, count);
    }

    // reverse_utf8
    u8str& u8str::reverse_utf8()
    {
        auto map = utf8_index_map(value_);
        if (map.size() <= 1)
            return *this;

        u8string out;
        out.reserve(value_.size());

        for (std::size_t i = map.size(); i > 0; --i)
        {
            std::size_t start = map[i - 1].first;
            std::size_t len = map[i - 1].second;
            out.append(value_.substr(start, len));
        }

        value_ = std::move(out);
        return *this;
    }

    // pad_left / pad_right (코드포인트 개수 기준)
    u8str& u8str::pad_left(std::size_t total_width, char fill)
    {
        auto map = utf8_index_map(value_);
        std::size_t len_cp = map.size();
        if (len_cp >= total_width)
            return *this;

        std::size_t pad_count = total_width - len_cp;
        u8string pad(pad_count, static_cast<char8>(static_cast<unsigned char>(fill)));

        value_.insert(0, pad);
        return *this;
    }

    u8str& u8str::pad_right(std::size_t total_width, char fill)
    {
        auto map = utf8_index_map(value_);
        std::size_t len_cp = map.size();
        if (len_cp >= total_width)
            return *this;

        std::size_t pad_count = total_width - len_cp;
        u8string pad(pad_count, static_cast<char8>(static_cast<unsigned char>(fill)));

        value_.append(pad);
        return *this;
    }

    // strip_prefix / strip_suffix
    bool u8str::strip_prefix(const u8str& prefix)
    {
        if (!starts_with(prefix))
            return false;
        value_.erase(0, prefix.size());
        return true;
    }

    bool u8str::strip_prefix(const char* prefix_utf8)
    {
        return strip_prefix(u8str(prefix_utf8));
    }

    bool u8str::strip_suffix(const u8str& suffix)
    {
        if (!ends_with(suffix))
            return false;
        value_.erase(value_.size() - suffix.size());
        return true;
    }

    bool u8str::strip_suffix(const char* suffix_utf8)
    {
        return strip_suffix(u8str(suffix_utf8));
    }

    // equals_ignore_case_ascii
    bool u8str::equals_ignore_case_ascii(const u8str& other) const
    {
        if (value_.size() != other.value_.size())
            return false;

        for (std::size_t i = 0; i < value_.size(); ++i)
        {
            unsigned char a = static_cast<unsigned char>(value_[i]);
            unsigned char b = static_cast<unsigned char>(other.value_[i]);

            a = detail::to_lower_ascii(a);
            b = detail::to_lower_ascii(b);

            if (a != b)
                return false;
        }
        return true;
    }

    bool u8str::equals_ignore_case_ascii(const char* other_utf8) const
    {
        return equals_ignore_case_ascii(u8str(other_utf8));
    }

    // to_lower / to_upper
    u8str& u8str::to_lower()
    {
        for (auto& ch : value_)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= 'A' && c <= 'Z')
                ch = static_cast<char8>(c + ('a' - 'A'));
        }
        return *this;
    }

    u8str& u8str::to_upper()
    {
        for (auto& ch : value_)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= 'a' && c <= 'z')
                ch = static_cast<char8>(c - ('a' - 'A'));
        }
        return *this;
    }

    u8str u8str::to_lower_copy() const
    {
        u8str tmp(*this);
        tmp.to_lower();
        return tmp;
    }

    u8str u8str::to_upper_copy() const
    {
        u8str tmp(*this);
        tmp.to_upper();
        return tmp;
    }

    // 연산자
    u8str& u8str::operator+=(const u8str& o)
    {
        value_ += o.value_;
        return *this;
    }

    u8str& u8str::operator+=(const char* o)
    {
        value_ += to_u8_string(o);
        return *this;
    }

    bool operator==(const u8str& a, const u8str& b) noexcept
    {
        return a.value_ == b.value_;
    }

    bool operator!=(const u8str& a, const u8str& b) noexcept
    {
        return !(a == b);
    }

    u8str operator+(const u8str& a, const u8str& b)
    {
        u8str r(a);
        r += b;
        return r;
    }

    u8str operator+(const u8str& a, const char* b)
    {
        u8str r(a);
        r += b;
        return r;
    }

    u8str operator+(const char* a, const u8str& b)
    {
        u8str r(a);
        r += b;
        return r;
    }

} // namespace mino::core::string::u8
