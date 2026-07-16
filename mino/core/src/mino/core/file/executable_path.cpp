#include "mino/core/file/executable_path.hpp"

#if defined(_WIN32)
#   ifndef NOMINMAX
#      define NOMINMAX
#   endif
#   include <windows.h>
#elif defined(__APPLE__)
#   include <mach-o/dyld.h>
#   include <limits.h>
#   include <unistd.h>
#else
#   include <limits.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <cerrno>
#endif

#include <vector>
#include <string>

namespace mino::core::file {

#if defined(_WIN32)
    static std::filesystem::path strip_win_device_prefix(const std::wstring& in) {
        static const std::wstring kUNC = L"\\\\?\\UNC\\";
        static const std::wstring kDEV = L"\\\\?\\";
        if (in.rfind(kUNC, 0) == 0) {
            std::wstring rest = in.substr(kUNC.size());
            return std::filesystem::path(L"\\\\" + rest);
        }
        if (in.rfind(kDEV, 0) == 0) {
            return std::filesystem::path(in.substr(kDEV.size()));
        }
        return std::filesystem::path(in);
    }

    // Windows: wstring -> UTF-8 std::string 변환 헬퍼
    static std::string wstring_to_utf8(const std::wstring& w) {
        if (w.empty()) return {};
        // WideCharToMultiByte에 전달할 길이(문자 수)
        int srcLen = static_cast<int>(w.size());
        int required = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), srcLen, nullptr, 0, nullptr, nullptr);
        if (required <= 0) return {};
        std::string out;
        out.resize(static_cast<size_t>(required));
        int written = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), srcLen, out.data(), required, nullptr, nullptr);
        if (written <= 0) return {};
        return out;
    }
#endif

    // 경로 정규화(심볼릭/정션 해제 요청 시)
    static std::filesystem::path normalize_path(
        const std::filesystem::path& p,
        const exec_path_options& opt,
        std::error_code& ec) {

        if (!opt.resolve_symlink) return p;
#if defined(_WIN32)
        auto w = std::filesystem::weakly_canonical(p, ec);
        if (!ec) return w;
        ec.clear();
        return p;
#else
        char buf[PATH_MAX];
        if (::realpath(p.c_str(), buf) != nullptr) {
            return std::filesystem::path(buf);
        }
        ec.clear();
        auto w = std::filesystem::weakly_canonical(p, ec);
        if (!ec) return w;
        ec.clear();
        return p;
#endif
    }

#if defined(_WIN32)
    static std::filesystem::path win_executable_path(
        std::error_code& ec,
        const exec_path_options& opt) {

        std::wstring result;
        DWORD size = 32768;
        for (;;) {
            std::vector<wchar_t> buf(size, L'\0');
            DWORD len = ::GetModuleFileNameW(nullptr, buf.data(),
                static_cast<DWORD>(buf.size()));
            if (len == 0) {
                ec = std::error_code(static_cast<int>(::GetLastError()),
                    std::system_category());
                return {};
            }
            if (len < size - 1) {
                result.assign(buf.data(), len);
                break;
            }
            size *= 2;
        }

        ec.clear();
        if (opt.windows_keep_device_prefix) {
            return std::filesystem::path(result);
        }
        return strip_win_device_prefix(result);
    }
#elif defined(__APPLE__)
    static std::filesystem::path mac_executable_path(
        std::error_code& ec,
        const exec_path_options&) {

        uint32_t size = 0;
        if (_NSGetExecutablePath(nullptr, &size) == 0) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }
        std::vector<char> buf(size, '\0');
        if (_NSGetExecutablePath(buf.data(), &size) != 0) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }
        ec.clear();
        return std::filesystem::path(buf.data());
    }
#else
    static std::filesystem::path linux_executable_path(
        std::error_code& ec,
        const exec_path_options& opt) {

        std::vector<char> buf(PATH_MAX, '\0');
        for (;;) {
            ssize_t n = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
            if (n == -1) {
                ec = std::error_code(errno, std::generic_category());
                return {};
            }
            if (static_cast<size_t>(n) < buf.size() - 1) {
                buf[static_cast<size_t>(n)] = '\0';
                break;
            }
            buf.resize(buf.size() * 2, '\0');
        }

        std::string pathStr(buf.data());
        const std::string deletedSuffix = " (deleted)";
        if (pathStr.size() > deletedSuffix.size() &&
            pathStr.compare(pathStr.size() - deletedSuffix.size(),
                deletedSuffix.size(), deletedSuffix) == 0) {
            pathStr.erase(pathStr.size() - deletedSuffix.size());
            if (opt.prefer_realpath_on_linux_deleted) {
                char rbuf[PATH_MAX];
                if (::realpath(pathStr.c_str(), rbuf) != nullptr) {
                    ec.clear();
                    return std::filesystem::path(rbuf);
                }
            }
        }

        ec.clear();
        return std::filesystem::path(pathStr);
    }
#endif

    // -------- 에러코드 버전(기존과 동일, 무-예외) --------
    std::filesystem::path executable_path(
        std::error_code& ec,
        const exec_path_options& opt) {

#if defined(_WIN32)
        auto raw = win_executable_path(ec, opt);
        if (ec) return {};
        return normalize_path(raw, opt, ec);
#elif defined(__APPLE__)
        auto raw = mac_executable_path(ec, opt);
        if (ec) return {};
        return normalize_path(raw, opt, ec);
#else
        auto raw = linux_executable_path(ec, opt);
        if (ec) return {};
        return normalize_path(raw, opt, ec);
#endif
    }

    // -------- 예외 버전 -> 무-예외로 변경(빈 값 반환) --------
    std::filesystem::path executable_path(const exec_path_options& opt) {
        std::error_code ec;
        auto p = executable_path(ec, opt);
        // 더 이상 throw 하지 않음. 실패 시 빈 path 반환.
        return ec ? std::filesystem::path{} : p;
    }

    std::filesystem::path executable_dir(
        std::error_code& ec,
        const exec_path_options& opt) {

        auto p = executable_path(ec, opt);
        if (ec) return {};
        return p.parent_path();
    }

    // 예외 버전 -> 무-예외로 변경(빈 값 반환)
    std::filesystem::path executable_dir(const exec_path_options& opt) {
        std::error_code ec;
        auto d = executable_dir(ec, opt);
        // 더 이상 throw 하지 않음. 실패 시 빈 path 반환.
        return ec ? std::filesystem::path{} : d;
    }

    // -------- 문자열 래퍼: 예외 버전도 무-예외로 변경 --------
    std::string executable_path_string(
        std::error_code& ec,
        const exec_path_options& opt) {

        auto p = executable_path(ec, opt);
        if (ec) return {};
#if defined(_WIN32)
        return wstring_to_utf8(p.wstring());
#else
        return p.string();
#endif
    }

    // 예외 버전 -> 무-예외로 변경(빈 문자열 반환)
    std::string executable_path_string(const exec_path_options& opt) {
        std::error_code ec;
        auto s = executable_path_string(ec, opt);
        return ec ? std::string{} : s;
    }

    std::wstring executable_path_wstring(
        std::error_code& ec,
        const exec_path_options& opt) {

        auto p = executable_path(ec, opt);
        if (ec) return {};
        return p.wstring();
    }

    // 예외 버전 -> 무-예외로 변경(빈 문자열 반환)
    std::wstring executable_path_wstring(const exec_path_options& opt) {
        std::error_code ec;
        auto s = executable_path_wstring(ec, opt);
        return ec ? std::wstring{} : s;
    }

    std::string executable_dir_string(
        std::error_code& ec,
        const exec_path_options& opt) {

        auto p = executable_dir(ec, opt);
        if (ec) return {};
#if defined(_WIN32)
        return wstring_to_utf8(p.wstring());
#else
        return p.string();
#endif
    }

    // 예외 버전 -> 무-예외로 변경(빈 문자열 반환)
    std::string executable_dir_string(const exec_path_options& opt) {
        std::error_code ec;
        auto s = executable_dir_string(ec, opt);
        return ec ? std::string{} : s;
    }

    std::wstring executable_dir_wstring(
        std::error_code& ec,
        const exec_path_options& opt) {

        auto p = executable_dir(ec, opt);
        if (ec) return {};
        return p.wstring();
    }

    // 예외 버전 -> 무-예외로 변경(빈 문자열 반환)
    std::wstring executable_dir_wstring(const exec_path_options& opt) {
        std::error_code ec;
        auto s = executable_dir_wstring(ec, opt);
        return ec ? std::wstring{} : s;
    }

} // namespace mino::core::file
