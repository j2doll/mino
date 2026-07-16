
#include "mino/core/file/path.hpp" 

#if defined(_WIN32)
#   include <windows.h>
#endif

namespace mino::core::file
{
    std::filesystem::path path_from_utf8(const std::string& utf8)
    {
        if (utf8.empty())
            return std::filesystem::path{};

#if defined(_WIN32)
        // UTF-8 -> wide string 변환
        const char* p = utf8.data(); // UTF-8 문자열 데이터 포인터
        int len = static_cast<int>(utf8.size()); // UTF-8 문자열 길이

        int req = MultiByteToWideChar(
            CP_UTF8,
            0,
            p,
            len,
            nullptr,
            0); // 버퍼 크기 계산
        if (req <= 0)
        {
            // 변환 실패 시 narrow path 로 시도(안전한 대안)
            return std::filesystem::path(utf8);
        }

        std::wstring w; // Windows 파일명은 UTF-16 wstring 사용
        w.resize(static_cast<size_t>(req)); // 버퍼 크기에 맞게 resize
        MultiByteToWideChar(
            CP_UTF8,
            0,
            p,
            len,
            &w[0],
            req); // UTF-8 -> UTF-16 변환
        return std::filesystem::path(w);
#else
        // POSIX 계열은 UTF-8 바이트 시퀀스를 그대로 사용
        return std::filesystem::path(utf8);
#endif
    }
    
} // namespace mino::core::file
