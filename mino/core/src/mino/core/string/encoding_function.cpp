#include <string>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <cstring>

#if defined(_WIN32)
#   include <windows.h>
#else
#   include <locale>
#   include <iconv.h> // POSIX iconv
#   define USE_ICONV
#endif

#include "mino/core/string/encoding_function.hpp"

namespace {

    // UTF-16 <-> UTF-32 수동 변환
    bool utf16_to_utf32_impl(const std::u16string& u16, std::u32string& out)
    {
        out.clear();
        out.reserve(u16.size());
        for (std::size_t i = 0; i < u16.size(); ++i) {
            char16_t cu = u16[i];
            if (cu >= 0xD800 && cu <= 0xDBFF) {
                if (i + 1 >= u16.size()) return false;
                char16_t cu2 = u16[i + 1];
                if (cu2 < 0xDC00 || cu2 > 0xDFFF) return false;
                uint32_t high = static_cast<uint32_t>(cu - 0xD800);
                uint32_t low = static_cast<uint32_t>(cu2 - 0xDC00);
                uint32_t cp = (high << 10) + low + 0x10000;
                out.push_back(static_cast<char32_t>(cp));
                ++i;
            }
            else if (cu >= 0xDC00 && cu <= 0xDFFF) {
                return false;
            }
            else {
                out.push_back(static_cast<char16_t>(cu));
            }
        }
        return true;
    }

    bool utf32_to_utf16_impl(const std::u32string& u32, std::u16string& out)
    {
        out.clear();
        out.reserve(u32.size());
        for (char32_t cp : u32) {
            uint32_t v = static_cast<uint32_t>(cp);
            if (v <= 0xFFFF) {
                if (v >= 0xD800 && v <= 0xDFFF) return false;
                out.push_back(static_cast<char16_t>(v));
            }
            else if (v <= 0x10FFFF) {
                v -= 0x10000;
                char16_t high = static_cast<char16_t>(0xD800 + ((v >> 10) & 0x3FF));
                char16_t low = static_cast<char16_t>(0xDC00 + (v & 0x3FF));
                out.push_back(high);
                out.push_back(low);
            }
            else {
                return false;
            }
        }
        return true;
    }

#if defined(_WIN32)

    // 공통: 멀티바이트→유니코드(UTF-16)
    bool wide_from_multi(const std::string& in, UINT codepage, std::u16string& out)
    {
        out.clear();
        if (in.empty()) {
            out = u"";
            return true;
        }

        int needed = ::MultiByteToWideChar(
            codepage,
            0,
            in.data(),
            static_cast<int>(in.size()),
            nullptr,
            0);
        if (needed <= 0) {
            return false;
        }

        std::wstring wtmp;
        wtmp.resize(static_cast<std::size_t>(needed));
        int written = ::MultiByteToWideChar(
            codepage,
            0,
            in.data(),
            static_cast<int>(in.size()),
            wtmp.data(),
            needed);
        if (written != needed) {
            // DWORD err = ::GetLastError();
            return false;
        }

        out.assign(reinterpret_cast<const char16_t*>(wtmp.data()),
            reinterpret_cast<const char16_t*>(wtmp.data()) + wtmp.size());
        return true;
    }

    // 공통: 유니코드(UTF-16)→멀티바이트
    bool multi_from_wide(const std::u16string& in, UINT codepage, std::string& out)
    {
        out.clear();
        if (in.empty()) {
            out.clear();
            return true;
        }

        std::wstring wtmp(
            reinterpret_cast<const wchar_t*>(in.data()),
            reinterpret_cast<const wchar_t*>(in.data()) + in.size());

        int needed = ::WideCharToMultiByte(
            codepage,
            0,
            wtmp.data(),
            static_cast<int>(wtmp.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        if (needed <= 0) {
            // DWORD err = ::GetLastError();
            return false;
        }

        out.resize(static_cast<std::size_t>(needed));
        int written = ::WideCharToMultiByte(
            codepage,
            0,
            wtmp.data(),
            static_cast<int>(wtmp.size()),
            out.data(),
            needed,
            nullptr,
            nullptr);
        if (written != needed) {
            // DWORD err = ::GetLastError();
            return false;
        }

        return true;
    }

#else // !_WIN32

    // -----------------------
    // UTF-8 <-> UTF-32 (직접 구현)
    // -----------------------
    bool decode_utf8_to_u32(const std::string& in, std::u32string& out)
    {
        out.clear();
        const unsigned char* s = reinterpret_cast<const unsigned char*>(in.data());
        std::size_t len = in.size();
        std::size_t i = 0;
        while (i < len) {
            unsigned char c = s[i];
            uint32_t cp = 0;
            std::size_t seq_len = 0;
            if (c <= 0x7F) {
                cp = c;
                seq_len = 1;
            }
            else if ((c >> 5) == 0x6) { // 110x xxxx
                seq_len = 2;
                cp = c & 0x1F;
            }
            else if ((c >> 4) == 0xE) { // 1110 xxxx
                seq_len = 3;
                cp = c & 0x0F;
            }
            else if ((c >> 3) == 0x1E) { // 11110 xxx
                seq_len = 4;
                cp = c & 0x07;
            }
            else {
                return false;
            }

            if (i + seq_len > len) return false;

            for (std::size_t k = 1; k < seq_len; ++k) {
                unsigned char cc = s[i + k];
                if ((cc >> 6) != 0x2) return false; // continuation byte 10xxxxxx
                cp = (cp << 6) | (cc & 0x3F);
            }

            // overlong / invalid checks
            if (seq_len == 2 && cp < 0x80) return false;
            if (seq_len == 3 && cp < 0x800) return false;
            if (seq_len == 4 && cp < 0x10000) return false;
            if (cp > 0x10FFFF) return false;
            if (cp >= 0xD800 && cp <= 0xDFFF) return false; // surrogate halves

            out.push_back(static_cast<char32_t>(cp));
            i += seq_len;
        }
        return true;
    }

    bool encode_u32_to_utf8(const std::u32string& in, std::string& out)
    {
        out.clear();
        for (char32_t c32 : in) {
            uint32_t cp = static_cast<uint32_t>(c32);
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            }
            else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0xFFFF) {
                if (cp >= 0xD800 && cp <= 0xDFFF) return false;
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0x10FFFF) {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else {
                return false;
            }
        }
        return true;
    }

    // 비-Windows: UTF-8 <-> UTF-16/32 (codecvt 대체: 직접 구현)
#if defined(USE_ICONV)
    // Forward declaration: iconv helper used below.
    bool iconv_convert(const char* from_charset,
                       const char* to_charset,
                       const std::string& in,
                       std::string& out);
#endif

    bool u16_from_utf8_codecvt(const std::string& in, std::u16string& out)
    {
#if defined(USE_ICONV)
        // iconv path: prefer iconv if configured for performance/locale correctness
        std::string tmp;
        if (!iconv_convert("UTF-8", "UTF-16LE", in, tmp)) {
            // 일부 환경에서는 UTF-16BE 등으로 동작할 수 있으므로 실패 시 fallthrough to manual
        }
        else {
            // tmp now contains UTF-16LE bytes; convert to u16string
            if (tmp.size() % 2 != 0) return false;
            out.resize(tmp.size() / 2);
            for (std::size_t i = 0; i < out.size(); ++i) {
                uint8_t b0 = static_cast<uint8_t>(tmp[i * 2]);
                uint8_t b1 = static_cast<uint8_t>(tmp[i * 2 + 1]);
                out[i] = static_cast<char16_t>((b1 << 8) | b0);
            }
            return true;
        }
#endif
        // fallback: decode UTF-8 to UTF-32 then convert UTF-32 -> UTF-16
        std::u32string u32;
        if (!decode_utf8_to_u32(in, u32)) return false;
        return utf32_to_utf16_impl(u32, out);
    }

    bool utf8_from_u16_codecvt(const std::u16string& in, std::string& out)
    {
#if defined(USE_ICONV)
        // Try iconv UTF-16LE -> UTF-8
        std::string tmp;
        // convert u16string -> bytes UTF-16LE
        tmp.resize(in.size() * 2);
        for (std::size_t i = 0; i < in.size(); ++i) {
            uint16_t v = static_cast<uint16_t>(in[i]);
            tmp[i * 2] = static_cast<char>(v & 0xFF);
            tmp[i * 2 + 1] = static_cast<char>((v >> 8) & 0xFF);
        }
        if (iconv_convert("UTF-16LE", "UTF-8", tmp, out)) return true;
        // else fallthrough to manual
#endif
        std::u32string u32;
        if (!utf16_to_utf32_impl(in, u32)) return false;
        return encode_u32_to_utf8(u32, out);
    }

    bool u32_from_utf8_codecvt(const std::string& in, std::u32string& out)
    {
        return decode_utf8_to_u32(in, out);
    }

    bool utf8_from_u32_codecvt(const std::u32string& in, std::string& out)
    {
        return encode_u32_to_utf8(in, out);
    }

#if defined(USE_ICONV)
    // iconv 변환
    bool iconv_convert(const char* from_charset,
        const char* to_charset,
        const std::string& in,
        std::string& out)
    {
        out.clear();
        iconv_t cd = iconv_open(to_charset, from_charset);
        if (cd == (iconv_t)-1) return false;

        std::size_t inbytesleft = in.size();
        std::size_t outbufsz = in.size() * 6 + 32; // 넉넉히
        std::vector<char> buf(outbufsz);
        char* inbuf = const_cast<char*>(in.data());
        char* outptr = buf.data();
        std::size_t outbytesleft = outbufsz;

        while (inbytesleft > 0) {
            size_t r = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);
            if (r == (size_t)-1) {
                iconv_close(cd);
                return false;
            }
        }
        out.assign(buf.data(), outbufsz - outbytesleft);
        iconv_close(cd);
        return true;
    }
#endif // USE_ICONV

#endif // _WIN32

    // 공통 예외 헬퍼
    static inline void throw_conv_err(const char* what) {
        throw std::runtime_error(what);
    }

} // namespace (anon)

namespace mino::core::string
{
    // =============================
    // UTF-8 <-> UTF-16
    // =============================
    bool utf8_to_utf16(const std::string& utf8, std::u16string& out)
    {
#if defined(_WIN32)
        return wide_from_multi(utf8, CP_UTF8, out);
#else
        return u16_from_utf8_codecvt(utf8, out);
#endif
    }

    bool utf16_to_utf8(const std::u16string& u16, std::string& out)
    {
#if defined(_WIN32)
        return multi_from_wide(u16, CP_UTF8, out);
#else
        return utf8_from_u16_codecvt(u16, out);
#endif
    }

    std::wstring utf8_to_utf16(const std::string& utf8)
    {
#if defined(_WIN32)
        if (utf8.empty()) return std::wstring{};
        int needed = ::MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8.data(),
            static_cast<int>(utf8.size()),
            nullptr,
            0);
        if (needed <= 0) {
            return std::wstring{};
        }
        std::wstring w;
        w.resize(static_cast<std::size_t>(needed));
        int written = ::MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8.data(),
            static_cast<int>(utf8.size()),
            &w[0],
            needed);
        if (written != needed) {
            return std::wstring{};
        }
        return w;
#else
        // 포팅: wchar_t가 4바이트(대부분 POSIX)일 때는 UTF-32로 처리
        std::u32string u32;
        if (!utf8_to_utf32(utf8, u32)) return std::wstring{};
        std::wstring out;
        if (sizeof(wchar_t) == 4) {
            out.resize(u32.size());
            for (std::size_t i = 0; i < u32.size(); ++i) out[i] = static_cast<wchar_t>(u32[i]);
            return out;
        }
        else if (sizeof(wchar_t) == 2) {
            std::u16string u16;
            if (!utf32_to_utf16(u32, u16)) return std::wstring{};
            out.resize(u16.size());
            for (std::size_t i = 0; i < u16.size(); ++i) out[i] = static_cast<wchar_t>(u16[i]);
            return out;
        }
        return std::wstring{};
#endif
    }

    std::string utf16_to_utf8(const std::wstring& utf16)
    {
#if defined(_WIN32)
        if (utf16.empty()) {
            return std::string{};
        }

        UINT codepage = CP_UTF8;

        int needed = ::WideCharToMultiByte(
            codepage,
            0,
            utf16.data(),
            static_cast<int>(utf16.size()),
            nullptr,
            0,
            nullptr,
            nullptr); 
        if (needed <= 0) {
            // DWORD err = ::GetLastError(); 
            return std::string{};
        }

        std::string out;
        out.resize(static_cast<std::size_t>(needed));

        int written = ::WideCharToMultiByte(
                codepage,
                0,
                utf16.data(),
                static_cast<int>(utf16.size()),
                &out[0],
                needed,
                nullptr,
                nullptr);
        if (written != needed) {
            // DWORD err = ::GetLastError(); 
            return std::string{};
        }

        return out;
#else
        if (sizeof(wchar_t) == 4) {
            std::u32string u32;
            u32.resize(utf16.size());
            for (std::size_t i = 0; i < utf16.size(); ++i) u32[i] = static_cast<char32_t>(utf16[i]);
            std::string out;
            if (!utf8_from_u32_codecvt(u32, out)) return std::string{};
            return out;
        }
        else if (sizeof(wchar_t) == 2) {
            std::u16string u16;
            u16.resize(utf16.size());
            for (std::size_t i = 0; i < utf16.size(); ++i) u16[i] = static_cast<char16_t>(utf16[i]);
            std::string out;
            if (!utf16_to_utf8(u16, out)) return std::string{};
            return out;
        }
        return std::string{};
#endif
    }

    // =============================
    // UTF-8 <-> UTF-32
    // =============================
    bool utf8_to_utf32(const std::string& utf8, std::u32string& out)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf8_to_utf16(utf8, u16)) return false;
        return utf16_to_utf32(u16, out);
#else
        return u32_from_utf8_codecvt(utf8, out);
#endif
    }

    bool utf32_to_utf8(const std::u32string& u32, std::string& out)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf32_to_utf16(u32, u16)) return false;
        return utf16_to_utf8(u16, out);
#else
        return utf8_from_u32_codecvt(u32, out);
#endif
    }

    // =============================
    // UTF-16 <-> UTF-32
    // =============================
    bool utf16_to_utf32(const std::u16string& u16, std::u32string& out)
    {
        return utf16_to_utf32_impl(u16, out);
    }

    bool utf32_to_utf16(const std::u32string& u32, std::u16string& out)
    {
        return utf32_to_utf16_impl(u32, out);
    }

    // =============================
    // UTF-8/16 <-> CP949
    // =============================
    bool utf8_to_cp949(const std::string& utf8, std::string& out_cp949)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf8_to_utf16(utf8, u16)) return false;
        return utf16_to_cp949(u16, out_cp949);
#else
#if defined(USE_ICONV)
        return iconv_convert("UTF-8", "CP949", utf8, out_cp949)
            || iconv_convert("UTF-8", "EUC-KR", utf8, out_cp949);
#else
        (void)utf8; (void)out_cp949;
        return false;
#endif
#endif
    }

    bool cp949_to_utf8(const std::string& cp949, std::string& out_utf8)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!cp949_to_utf16(cp949, u16)) return false;
        return utf16_to_utf8(u16, out_utf8);
#else
#if defined(USE_ICONV)
        return iconv_convert("CP949", "UTF-8", cp949, out_utf8)
            || iconv_convert("EUC-KR", "UTF-8", cp949, out_utf8);
#else
        (void)cp949; (void)out_utf8;
        return false;
#endif
#endif
    }

    bool utf16_to_cp949(const std::u16string& u16, std::string& out_cp949)
    {
#if defined(_WIN32)
        return multi_from_wide(u16, 949 /* CP949 */, out_cp949);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!utf16_to_utf8(u16, utf8)) return false;
        return utf8_to_cp949(utf8, out_cp949);
#else
        (void)u16; (void)out_cp949;
        return false;
#endif
#endif
    }

    bool cp949_to_utf16(const std::string& cp949, std::u16string& out_u16)
    {
#if defined(_WIN32)
        return wide_from_multi(cp949, 949 /* CP949 */, out_u16);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!cp949_to_utf8(cp949, utf8)) return false;
        return utf8_to_utf16(utf8, out_u16);
#else
        (void)cp949; (void)out_u16;
        return false;
#endif
#endif
    }

    // =============================
    // 추가: ISO-2022-KR
    // =============================
    bool utf8_to_iso2022kr(const std::string& utf8, std::string& out_iso2022kr)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf8_to_utf16(utf8, u16)) return false;
        return utf16_to_iso2022kr(u16, out_iso2022kr);
#else
#if defined(USE_ICONV)
        // 일부 환경은 "ISO-2022-KR"만 인식, 별칭 거의 동일
        return iconv_convert("UTF-8", "ISO-2022-KR", utf8, out_iso2022kr);
#else
        (void)utf8; (void)out_iso2022kr;
        return false;
#endif
#endif
    }

    bool iso2022kr_to_utf8(const std::string& iso2022kr, std::string& out_utf8)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!iso2022kr_to_utf16(iso2022kr, u16)) return false;
        return utf16_to_utf8(u16, out_utf8);
#else
#if defined(USE_ICONV)
        return iconv_convert("ISO-2022-KR", "UTF-8", iso2022kr, out_utf8);
#else
        (void)iso2022kr; (void)out_utf8;
        return false;
#endif
#endif
    }

    bool utf16_to_iso2022kr(const std::u16string& u16, std::string& out_iso2022kr)
    {
#if defined(_WIN32)
        return multi_from_wide(u16, 50225 /* ISO-2022-KR */, out_iso2022kr);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!utf16_to_utf8(u16, utf8)) return false;
        return utf8_to_iso2022kr(utf8, out_iso2022kr);
#else
        (void)u16; (void)out_iso2022kr;
        return false;
#endif
#endif
    }

    bool iso2022kr_to_utf16(const std::string& iso2022kr, std::u16string& out_u16)
    {
#if defined(_WIN32)
        return wide_from_multi(iso2022kr, 50225 /* ISO-2022-KR */, out_u16);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!iso2022kr_to_utf8(iso2022kr, utf8)) return false;
        return utf8_to_utf16(utf8, out_u16);
#else
        (void)iso2022kr; (void)out_u16;
        return false;
#endif
#endif
    }

    // =============================
    // 추가: JOHAB (CP 1361)
    // =============================
    bool utf8_to_johab(const std::string& utf8, std::string& out_johab)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf8_to_utf16(utf8, u16)) return false;
        return utf16_to_johab(u16, out_johab);
#else
#if defined(USE_ICONV)
        return iconv_convert("UTF-8", "JOHAB", utf8, out_johab);
#else
        (void)utf8; (void)out_johab;
        return false;
#endif
#endif
    }

    bool johab_to_utf8(const std::string& johab, std::string& out_utf8)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!johab_to_utf16(johab, u16)) return false;
        return utf16_to_utf8(u16, out_utf8);
#else
#if defined(USE_ICONV)
        return iconv_convert("JOHAB", "UTF-8", johab, out_utf8);
#else
        (void)johab; (void)out_utf8;
        return false;
#endif
#endif
    }

    bool utf16_to_johab(const std::u16string& u16, std::string& out_johab)
    {
#if defined(_WIN32)
        return multi_from_wide(u16, 1361 /* JOHAB */, out_johab);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!utf16_to_utf8(u16, utf8)) return false;
        return utf8_to_johab(utf8, out_johab);
#else
        (void)u16; (void)out_johab;
        return false;
#endif
#endif
    }

    bool johab_to_utf16(const std::string& johab, std::u16string& out_u16)
    {
#if defined(_WIN32)
        return wide_from_multi(johab, 1361 /* JOHAB */, out_u16);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!johab_to_utf8(johab, utf8)) return false;
        return utf8_to_utf16(utf8, out_u16);
#else
        (void)johab; (void)out_u16;
        return false;
#endif
#endif
    }

    // =============================
    // 추가: MacKorean
    // =============================
    bool utf8_to_mackorean(const std::string& utf8, std::string& out_mackor)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!utf8_to_utf16(utf8, u16)) return false;
        return utf16_to_mackorean(u16, out_mackor);
#else
#if defined(USE_ICONV)
        // 일부 환경은 "MacKorean" 또는 "MAC-KOREAN" 로 노출
        if (iconv_convert("UTF-8", "MacKorean", utf8, out_mackor)) return true;
        return iconv_convert("UTF-8", "MAC-KOREAN", utf8, out_mackor);
#else
        (void)utf8; (void)out_mackor;
        return false;
#endif
#endif
    }

    bool mackorean_to_utf8(const std::string& mackor, std::string& out_utf8)
    {
#if defined(_WIN32)
        std::u16string u16;
        if (!mackorean_to_utf16(mackor, u16)) return false;
        return utf16_to_utf8(u16, out_utf8);
#else
#if defined(USE_ICONV)
        if (iconv_convert("MacKorean", "UTF-8", mackor, out_utf8)) return true;
        return iconv_convert("MAC-KOREAN", "UTF-8", mackor, out_utf8);
#else
        (void)mackor; (void)out_utf8;
        return false;
#endif
#endif
    }

    bool utf16_to_mackorean(const std::u16string& u16, std::string& out_mackor)
    {
#if defined(_WIN32)
        // Windows는 Mac 코드페이지 계열(10000~)을 지원할 수 있음 (환경에 따라 미탑재 가능)
        return multi_from_wide(u16, 10003 /* MacKorean */, out_mackor);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!utf16_to_utf8(u16, utf8)) return false;
        return utf8_to_mackorean(utf8, out_mackor);
#else
        (void)u16; (void)out_mackor;
        return false;
#endif
#endif
    }

    bool mackorean_to_utf16(const std::string& mackor, std::u16string& out_u16)
    {
#if defined(_WIN32)
        return wide_from_multi(mackor, 10003 /* MacKorean */, out_u16);
#else
#if defined(USE_ICONV)
        std::string utf8;
        if (!mackorean_to_utf8(mackor, utf8)) return false;
        return utf8_to_utf16(utf8, out_u16);
#else
        (void)mackorean; (void)out_u16;
        return false;
#endif
#endif
    }

    // =============================
    // 기존 throwing helpers + 추가 코덱 throwing
    // =============================
    std::u16string utf8_to_utf16_or_throw(const std::string& utf8)
    {
        std::u16string out; if (!utf8_to_utf16(utf8, out)) throw_conv_err("utf8_to_utf16 failed"); return out;
    }
    std::string utf16_to_utf8_or_throw(const std::u16string& u16)
    {
        std::string out; if (!utf16_to_utf8(u16, out)) throw_conv_err("utf16_to_utf8 failed"); return out;
    }
    std::u32string utf8_to_utf32_or_throw(const std::string& utf8)
    {
        std::u32string out; if (!utf8_to_utf32(utf8, out)) throw_conv_err("utf8_to_utf32 failed"); return out;
    }
    std::string utf32_to_utf8_or_throw(const std::u32string& u32)
    {
        std::string out; if (!utf32_to_utf8(u32, out)) throw_conv_err("utf32_to_utf8 failed"); return out;
    }
    std::u32string utf16_to_utf32_or_throw(const std::u16string& u16)
    {
        std::u32string out; if (!utf16_to_utf32(u16, out)) throw_conv_err("utf16_to_utf32 failed"); return out;
    }
    std::u16string utf32_to_utf16_or_throw(const std::u32string& u32)
    {
        std::u16string out; if (!utf32_to_utf16(u32, out)) throw_conv_err("utf32_to_utf16 failed"); return out;
    }
    std::string utf8_to_cp949_or_throw(const std::string& utf8)
    {
        std::string out; if (!utf8_to_cp949(utf8, out)) throw_conv_err("utf8_to_cp949 failed"); return out;
    }
    std::string cp949_to_utf8_or_throw(const std::string& cp949)
    {
        std::string out; if (!cp949_to_utf8(cp949, out)) throw_conv_err("cp949_to_utf8 failed"); return out;
    }
    std::string utf16_to_cp949_or_throw(const std::u16string& u16)
    {
        std::string out; if (!utf16_to_cp949(u16, out)) throw_conv_err("utf16_to_cp949 failed"); return out;
    }
    std::u16string cp949_to_utf16_or_throw(const std::string& cp949)
    {
        std::u16string out; if (!cp949_to_utf16(cp949, out)) throw_conv_err("cp949_to_utf16 failed"); return out;
    }

    std::string utf8_to_iso2022kr_or_throw(const std::string& utf8)
    {
        std::string out; if (!utf8_to_iso2022kr(utf8, out)) throw_conv_err("utf8_to_iso2022kr failed"); return out;
    }
    std::string iso2022kr_to_utf8_or_throw(const std::string& iso2022kr)
    {
        std::string out; if (!iso2022kr_to_utf8(iso2022kr, out)) throw_conv_err("iso2022kr_to_utf8 failed"); return out;
    }
    std::u16string iso2022kr_to_utf16_or_throw(const std::string& iso2022kr)
    {
        std::u16string out; if (!iso2022kr_to_utf16(iso2022kr, out)) throw_conv_err("iso2022kr_to_utf16 failed"); return out;
    }
    std::string utf16_to_iso2022kr_or_throw(const std::u16string& u16)
    {
        std::string out; if (!utf16_to_iso2022kr(u16, out)) throw_conv_err("utf16_to_iso2022kr failed"); return out;
    }

    std::string utf8_to_johab_or_throw(const std::string& utf8)
    {
        std::string out; if (!utf8_to_johab(utf8, out)) throw_conv_err("utf8_to_johab failed"); return out;
    }
    std::string johab_to_utf8_or_throw(const std::string& johab)
    {
        std::string out; if (!johab_to_utf8(johab, out)) throw_conv_err("johab_to_utf8 failed"); return out;
    }
    std::u16string johab_to_utf16_or_throw(const std::string& johab)
    {
        std::u16string out; if (!johab_to_utf16(johab, out)) throw_conv_err("johab_to_utf16 failed"); return out;
    }
    std::string utf16_to_johab_or_throw(const std::u16string& u16)
    {
        std::string out; if (!utf16_to_johab(u16, out)) throw_conv_err("utf16_to_johab failed"); return out;
    }

    std::string utf8_to_mackorean_or_throw(const std::string& utf8)
    {
        std::string out; if (!utf8_to_mackorean(utf8, out)) throw_conv_err("utf8_to_mackorean failed"); return out;
    }
    std::string mackorean_to_utf8_or_throw(const std::string& mackor)
    {
        std::string out; if (!mackorean_to_utf8(mackor, out)) throw_conv_err("mackorean_to_utf8 failed"); return out;
    }
    std::u16string mackorean_to_utf16_or_throw(const std::string& mackor)
    {
        std::u16string out; if (!mackorean_to_utf16(mackor, out)) throw_conv_err("mackorean_to_utf16 failed"); return out;
    }
    std::string utf16_to_mackorean_or_throw(const std::u16string& u16)
    {
        std::string out; if (!utf16_to_mackorean(u16, out)) throw_conv_err("utf16_to_mackorean failed"); return out;
    }

} 
