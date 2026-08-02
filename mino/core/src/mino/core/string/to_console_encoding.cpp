#include <string>
#include <limits>
#include <string_view>
#include <iomanip>
#include <iostream>
#include <vector>

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

#include "mino/core/string/to_console_encoding.hpp"

namespace mino::core::string {

#if defined(_WIN32)
    // 내부 헬퍼: UTF-8 → 지정 코드페이지(예: 51949, 949) 멀티바이트 변환
 
    static std::string utf8_to_codepage_impl(const std::string& utf8, unsigned int codepage) {
        if (utf8.empty())
            return std::string{};

        // 1) UTF-8 → UTF-16
        int wlen = ::MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8.c_str(),
            static_cast<int>(utf8.size()),
            nullptr,
            0);
        if (wlen <= 0) {
            return std::string{};
        }

        std::wstring wbuf(static_cast<size_t>(wlen), L'\0');
        if (::MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8.c_str(),
            static_cast<int>(utf8.size()),
            &wbuf[0],
            wlen) <= 0)
        {
            return std::string{};
        }

        // 2) UTF-16 → codepage 멀티바이트
        int mblen = ::WideCharToMultiByte(
            codepage,
            0,
            wbuf.c_str(),
            wlen,
            nullptr,
            0,
            nullptr,
            nullptr);
        if (mblen <= 0) {
            return std::string{};
        }

        std::string mbuf(static_cast<size_t>(mblen), '\0');
        if (::WideCharToMultiByte(
            codepage,
            0,
            wbuf.c_str(),
            wlen,
            &mbuf[0],
            mblen,
            nullptr,
            nullptr) <= 0)
        {
            return std::string{};
        }

        return mbuf;
    } 

#endif // _WIN32


#if defined(_WIN32)
    // 내부 헬퍼: 지정 코드페이지(예: 51949, 949) 멀티바이트 → UTF-8 변환
    static std::string codepage_to_utf8_impl(const std::string& mb, unsigned int codepage) {
        if (mb.empty())
            return std::string{};

        // 1) codepage 멀티바이트 → UTF-16
        int wlen = ::MultiByteToWideChar(
            codepage,
            0,
            mb.c_str(),
            static_cast<int>(mb.size()),
            nullptr,
            0);
        if (wlen <= 0) {
            return std::string{};
        }

        std::wstring wbuf(static_cast<size_t>(wlen), L'\0');
        if (::MultiByteToWideChar(
            codepage,
            0,
            mb.c_str(),
            static_cast<int>(mb.size()),
            &wbuf[0],
            wlen) <= 0)
        {
            return std::string{};
        }

        // 2) UTF-16 → UTF-8
        int u8len = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            wbuf.c_str(),
            wlen,
            nullptr,
            0,
            nullptr,
            nullptr);
        if (u8len <= 0) {
            return std::string{};
        }

        std::string u8buf(static_cast<size_t>(u8len), '\0');
        if (::WideCharToMultiByte(
            CP_UTF8,
            0,
            wbuf.c_str(),
            wlen,
            &u8buf[0],
            u8len,
            nullptr,
            nullptr) <= 0)
        {
            return std::string{};
        }

        return u8buf;
    }
#endif // _WIN32

    //////////////////////////////////////////////////////////

    std::string to_console_encoding(const std::string& utf8) {
#if defined(_WIN32)
        // Windows: UTF-8 → EUC-KR(51949) 시도, 실패 시 CP949(949) 폴백

        // 1순위: EUC-KR (51949) 시도
        {
            std::string encoded = utf8_to_codepage_impl(utf8, 51949);
            if (!encoded.empty()) {
                return encoded;
            }
        }

        // 2순위: CP949 (949) 폴백
        {
            std::string encoded = utf8_to_codepage_impl(utf8, 949);
            if (!encoded.empty()) {
                return encoded;
            }
        }

        // 최종 실패: 원본 UTF-8 반환
        return utf8;
#else
        // Linux/macOS: UTF-8 그대로 사용
        return utf8;
#endif
    }

    std::string from_console_encoding(const std::string& console_bytes) {
#if defined(_WIN32)
        // Windows: EUC-KR (51949) 시도, 실패 시 CP949 (949) 시도
        {
            std::string decoded = codepage_to_utf8_impl(console_bytes, 51949);
            if (!decoded.empty()) {
                return decoded;
            }
        }

        {
            std::string decoded = codepage_to_utf8_impl(console_bytes, 949);
            if (!decoded.empty()) {
                return decoded;
            }
        }

        // 최종 실패: 입력을 UTF-8로 간주하여 그대로 반환
        return console_bytes;
#else
        // Linux/macOS: 입력을 UTF-8로 간주
        return console_bytes;
#endif
    }

} // namespace mino::core::string 
