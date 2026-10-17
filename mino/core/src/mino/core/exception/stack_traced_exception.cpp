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
#endif

namespace mino::core::exception {

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
