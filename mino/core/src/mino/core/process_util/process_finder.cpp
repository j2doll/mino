#include <algorithm>
#include <iterator>
#include <cctype>

#include "mino/core/process_util/process_finder.hpp"

#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
    #include <windows.h>
    #include <tlhelp32.h>
#elif defined(__linux__)
    #define OS_LINUX
    #include <filesystem>
    #include <fstream>
    #include <cctype>
#endif

namespace mino::core::process_util {

    namespace {
        bool contains_substring_ignore_case(std::string_view str, std::string_view target) {
            if (target.empty()) return true;
            if (str.size() < target.size()) return false;

            auto it = std::search(
                str.begin(), str.end(),
                target.begin(), target.end(),
                [](char ch1, char ch2) {
                    return std::tolower(static_cast<unsigned char>(ch1)) ==
                        std::tolower(static_cast<unsigned char>(ch2));
                }
            );
            return it != str.end();
        }

#if defined(OS_WINDOWS)
        std::string wstring_to_utf8(const std::wstring& wstr) {
            if (wstr.empty()) return "";
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
            std::string str_to(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &str_to[0], size_needed, NULL, NULL);
            return str_to;
        }
#endif
    } // namespace

#if defined(OS_WINDOWS)
    std::vector<process_info> get_active_processes() {
        std::vector<process_info> process_list;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return process_list;

        PROCESSENTRY32W process_entry;
        process_entry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &process_entry)) {
            do {
                process_info info;
                info.pid = static_cast<uint32_t>(process_entry.th32ProcessID);
                info.process_name = wstring_to_utf8(process_entry.szExeFile);
                process_list.push_back(info);
            } while (Process32NextW(snapshot, &process_entry));
        }
        CloseHandle(snapshot);
        return process_list;
    }
#elif defined(OS_LINUX)
    std::vector<process_info> get_active_processes() {
        std::vector<process_info> process_list;
        namespace fs = std::filesystem;
        if (!fs::exists("/proc")) return process_list;

        for (const auto& entry : fs::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            std::string filename = entry.path().filename().string();
            if (!std::all_of(filename.begin(), filename.end(), ::isdigit)) continue;

            uint32_t pid = std::stoul(filename);
            std::ifstream comm_file(entry.path() / "comm");
            std::string process_name;
            if (comm_file >> process_name) {
                process_info info;
                info.pid = pid;
                info.process_name = process_name;
                process_list.push_back(info);
            }
        }
        return process_list;
    }
#else
    std::vector<process_info> get_active_processes() { return {}; }
#endif

    // [수정됨] 목록(src_processes)을 인자로 받아 필터링하는 핵심 로직
    std::vector<process_info> get_processes_by_name(
        const std::vector<process_info>& src_processes,
        std::string_view target_name,
        bool case_sensitive
    ) {
        if (target_name.empty()) {
            return src_processes;
        }

        std::vector<process_info> filtered_list;
        std::copy_if(src_processes.begin(), src_processes.end(), std::back_inserter(filtered_list),
            [target_name, case_sensitive](const process_info& info) {
                if (case_sensitive) {
                    return info.process_name.find(target_name) != std::string::npos;
                }
                else {
                    return contains_substring_ignore_case(info.process_name, target_name);
                }
            });

        return filtered_list;
    }

    // 기존의 인터페이스도 새 함수를 호출하도록 단순화
    std::vector<process_info> get_processes_by_name(std::string_view target_name, bool case_sensitive) {
        return get_processes_by_name(get_active_processes(), target_name, case_sensitive);
    }

}
