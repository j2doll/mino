#include "mino/core/exception/stack_traced_exception.hpp"

#include <sstream>

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <dbghelp.h>
#elif defined(__linux__) || defined(__APPLE__)
#   include <execinfo.h>
#   include <cxxabi.h>
#   include <memory>
#   include <dlfcn.h>
#   include <unistd.h>
#   include <array>
#endif

namespace mino::core::exception {

    namespace {
#if defined(__linux__)
        // 리눅스: 주소와 모듈 정보를 기반으로 파일명 및 라인 번호 복원
        std::string resolve_linux_source_location(void* addr) {
            Dl_info info;
            if (!dladdr(addr, &info) || !info.dli_fname) {
                return "";
            }

            // PIE(Position Independent Executable) 또는 공유 라이브러리의 상대 오프셋 계산
            uintptr_t base_addr = reinterpret_cast<uintptr_t>(info.dli_fbase);
            uintptr_t target_addr = reinterpret_cast<uintptr_t>(addr);
            uintptr_t offset = target_addr - base_addr;

            char cmd[2048];
            // PIE/공유객체는 오프셋 주소로 조회하고, 일반 바이너리는 절대 주소로 fallback 시도
            snprintf(cmd, sizeof(cmd),
                "addr2line -C -e \"%s\" -f -p %lx 2>/dev/null || addr2line -C -e \"%s\" -f -p %p 2>/dev/null",
                info.dli_fname, offset, info.dli_fname, addr);

            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
            if (!pipe) {
                return "";
            }

            std::array<char, 512> buffer;
            std::string result;
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }

            // 개행 문자 제거
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }

            // 디버그 심볼(-g)이 없는 Release 모드이거나 심볼을 못 찾으면 "??:0" 등이 반환됨
            if (result.empty() || result.find("??:0") != std::string::npos || result.find("??:?") != std::string::npos) {
                return "";
            }

            return result;
        }
#endif
    } // anonymous namespace

    stack_traced_exception::stack_traced_exception(const std::string& message)
        : std::runtime_error(message) {
        capture_stack_trace();
    }

    const std::vector<std::string>& stack_traced_exception::call_stack() const noexcept {
        return frames_;
    }

    std::string stack_traced_exception::to_string() const {
        std::ostringstream oss;
        oss << "exception: " << what() << "\n--- call stack (most recent call first) ---\n";
        for (size_t i = 0; i < frames_.size(); ++i) {
            oss << "  #" << i << " " << frames_[i] << "\n";
        }
        return oss.str();
    }

    void stack_traced_exception::capture_stack_trace() {
        constexpr int max_frames = 64;
        void* stack[max_frames];

#if defined(_WIN32)
        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        SymInitialize(process, nullptr, TRUE);

        // 프레임 0(capture_stack_trace) 및 프레임 1(생성자) 스킵
        USHORT frame_count = CaptureStackBackTrace(2, max_frames - 2, stack, nullptr);

        char symbol_buffer[sizeof(SYMBOL_INFO) + 256 * sizeof(char)] = { 0 };
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;

        IMAGEHLP_LINE64 line_info{};
        line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement = 0;

        for (USHORT i = 0; i < frame_count; ++i) {
            DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
            std::string func_name = "[unknown function]";
            std::string location = "";

            if (SymFromAddr(process, address, 0, symbol)) {
                func_name = symbol->Name;
            }

            // Debug 모드에서 PDB가 있으면 파일명:라인 획득, Release 모드면 자동으로 생략됨
            if (SymGetLineFromAddr64(process, address, &displacement, &line_info)) {
                location = std::string(" at ") + line_info.FileName + ":" + std::to_string(line_info.LineNumber);
            }

            frames_.push_back(func_name + "()" + location);
        }

#elif defined(__linux__) || defined(__APPLE__)
        int size = backtrace(stack, max_frames);
        char** symbols = backtrace_symbols(stack, size);

        // 프레임 0(capture_stack_trace) 및 프레임 1(생성자) 스킵
        for (int i = 2; i < size; ++i) {
            std::string frame_desc = symbols[i];

#if defined(__linux__)
            // Debug 모드(-g)인 경우 addr2line을 통해 "함수명 at 파일:라인" 획득 시도
            std::string resolved = resolve_linux_source_location(stack[i]);
            if (!resolved.empty()) {
                frames_.push_back(resolved);
                continue;
            }
#endif

            // Release 모드이거나(-g 없음) 조회 실패 시: 디맹글링된 함수명/기본 심볼 문자열 사용
            Dl_info info;
            if (dladdr(stack[i], &info) && info.dli_sname) {
                int status = -1;
                char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    frame_desc = demangled;
                    free(demangled);
                }
                else {
                    frame_desc = info.dli_sname;
                }
            }
            frames_.push_back(frame_desc);
        }
        free(symbols);
#endif
    }

} // namespace mino::core::exception
