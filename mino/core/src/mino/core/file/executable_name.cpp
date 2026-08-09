#include "mino/core/file/executable_name.hpp"

#if defined(_WIN32)
#   include <windows.h>
#   include <string>
static std::string wstring_to_utf8_local(const std::wstring& w) {
    if (w.empty()) 
        return {};

    int srcLen = static_cast<int>(w.size());
    int required = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), srcLen, nullptr, 0, nullptr, nullptr);
    if (required <= 0) 
        return {};

    std::string out;
    out.resize(static_cast<size_t>(required));

    int written = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), srcLen, out.data(), required, nullptr, nullptr);
    if (written <= 0) 
        return {};
    
    return out;
}
#endif

namespace mino::core::file {

    // std::string 버전 (에러코드)
    std::string executable_name(
        std::error_code& ec,
        name_type type,
        const exec_path_options& opt) 
    {

        auto p = executable_path(ec, opt);
        if (ec) return {};
        if (type == name_type::stem) {
#if defined(_WIN32)
            return wstring_to_utf8_local(p.stem().wstring());
#else
            return p.stem().string();
#endif
        }
#if defined(_WIN32)
        return wstring_to_utf8_local(p.filename().wstring());
#else
        return p.filename().string();
#endif
    }

    // std::string 버전 (무-예외)
    std::string executable_name(
        name_type type,
        const exec_path_options& opt) {

        std::error_code ec;
        auto s = executable_name(ec, type, opt);
        return ec ? std::string{} : s;
    }

    // std::wstring 버전 (에러코드)
    std::wstring executable_name_w(
        std::error_code& ec,
        name_type type,
        const exec_path_options& opt) {

        auto p = executable_path(ec, opt);
        if (ec) return {};
        if (type == name_type::stem) {
            return p.stem().wstring();
        }
        return p.filename().wstring();
    }

    // std::wstring 버전 (무-예외)
    std::wstring executable_name_w(
        name_type type,
        const exec_path_options& opt) {

        std::error_code ec;
        auto s = executable_name_w(ec, type, opt);
        return ec ? std::wstring{} : s;
    }

} // namespace mino::core::file
