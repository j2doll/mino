#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#include "mino/core/system/system.hpp"

#ifdef _WIN32 
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <dbghelp.h>
#   pragma comment(lib, "dbghelp.lib")
#else
#   include <csignal>
#   include <execinfo.h>
#   include <dlfcn.h>
#   include <cxxabi.h>
#   include <unistd.h>
#   include <memory>
#   include <array>
#   include <cstdio>
#   include <cstdlib>
#endif

namespace mino::core::system {

        crash_callback crash_handler::s_callback = nullptr;

#ifdef _WIN32
        // track whether SymInitialize succeeded
        static bool g_symInitialized = false;
#endif

        void crash_handler::initialize(crash_callback callback) {
            s_callback = callback;

            std::set_terminate(crash_handler::cxx_terminate_handler);

#ifdef _WIN32
            SetUnhandledExceptionFilter(crash_handler::windows_exception_handler);
            // Initialize dbghelp symbols and set options
            if (SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
                SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
                g_symInitialized = true;
            } else {
                g_symInitialized = false;
            }
#else
            // Use sigaction with SA_SIGINFO and a minimal, async-signal-safe handler.
            const int signals[] = { SIGSEGV, SIGFPE, SIGILL, SIGABRT, SIGBUS };
            struct sigaction sa;
            std::memset(&sa, 0, sizeof(sa));
            sa.sa_sigaction = crash_handler::posix_signal_handler;
            sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
            sigemptyset(&sa.sa_mask);

            for (int sig : signals) {
                sigaction(sig, &sa, nullptr);
            }
#endif
        }

#ifdef _WIN32
        // long __stdcall crash_handler::windows_exception_handler(struct _EXCEPTION_POINTERS* info) {
        //     std::ostringstream oss;
        //     oss << "Exception Code: 0x" << std::hex << info->ExceptionRecord->ExceptionCode;
        //     handle_crash("Windows SEH Exception", oss.str());
        //     return EXCEPTION_EXECUTE_HANDLER;
        // }

        long __stdcall crash_handler::windows_exception_handler(struct _EXCEPTION_POINTERS* info) {
            // Minimal, non-allocating handler: format into stack buffer and emit via WinAPI.
            char buf[256];
            int len = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "Unhandled SEH Exception Code: 0x%08X\n",
                info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0u);
            if (len < 0) len = (int)sizeof(buf) - 1;
            buf[len] = '\0';

            // Write to stderr if available (WriteFile is safe enough here) and to the debugger output.
            HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
            if (hErr != INVALID_HANDLE_VALUE && hErr != NULL) {
                DWORD written = 0;
                WriteFile(hErr, buf, (DWORD)strlen(buf), &written, NULL);
            }
            OutputDebugStringA(buf);

            // Do not call into other complex C++ code here. Let process terminate or be handled by caller.
            return EXCEPTION_EXECUTE_HANDLER;
        }

#else
        // POSIX: async-signal-safe handler. Keep it minimal.
        void crash_handler::posix_signal_handler(int sig, siginfo_t* /*si*/, void* /*unused*/) {
            char buf[128];
            int len = snprintf(buf, sizeof(buf), "Fatal signal %d caught. Exiting.\n", sig);
            if (len > 0) {
                ssize_t wr = write(STDERR_FILENO, buf, (size_t)std::min(len, (int)(sizeof(buf)-1)));
                (void)wr;
            }
            // Use _Exit to avoid flushing stdio or invoking destructors.
            _Exit(128 + (sig & 0xff));
        }

        std::string crash_handler::addr_to_line(void* addr) {
            char path[1024];
            ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if (len == -1) return "";
            path[len] = '\0';

            std::stringstream ss_addr;
            ss_addr << std::hex << reinterpret_cast<uintptr_t>(addr);

            // addr2line 명령어를 통해 파일명과 라인 추출
            std::string cmd = "addr2line -e " + std::string(path) + " " + ss_addr.str() + " -f -C -i";

            std::array<char, 128> buffer;
            std::string result;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

            if (!pipe) return "";

            while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }

            if (!result.empty() && result.find("??") == std::string::npos) {
                std::replace(result.begin(), result.end(), '\n', ' ');
                return " at " + result;
            }
            return "";
        }
#endif

        void crash_handler::cxx_terminate_handler() {
            handle_crash("C++ Terminate", "Unhandled C++ Exception");
            std::abort();
        }

        std::vector<std::string> crash_handler::get_stack_trace() {
            std::vector<std::string> stack_frames;

#ifdef _WIN32
            void* stack[64];
            unsigned short frames = CaptureStackBackTrace(0, 64, stack, NULL);
            HANDLE process = GetCurrentProcess();

            for (unsigned short i = 0; i < frames; ++i) {
                // allocate buffer safely and zero it
                const size_t bufSize = sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR);
                std::vector<char> buffer(bufSize);
                memset(buffer.data(), 0, bufSize);

                PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer.data());
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = MAX_SYM_NAME;

                DWORD64 displacement = 0;
                std::stringstream ss;

                if (g_symInitialized && SymFromAddr(process, (DWORD64)stack[i], &displacement, symbol)) {
                    ss << "#" << i << " 0x" << std::hex << symbol->Address << " in " << symbol->Name;

                    IMAGEHLP_LINE64 line;
                    memset(&line, 0, sizeof(line));
                    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                    DWORD lineDisplacement = 0;

                    if (SymGetLineFromAddr64(process, (DWORD64)stack[i], &lineDisplacement, &line)) {
                        ss << " at " << line.FileName << ":" << std::dec << line.LineNumber;
                    }
                    stack_frames.push_back(ss.str());
                }
                else {
                    stack_frames.push_back("#" + std::to_string(i) + " [Unknown Address]");
                }
            }
#else
            void* callstack[64];
            int frames = backtrace(callstack, 64);
            char** strs = backtrace_symbols(callstack, frames);

            for (int i = 0; i < frames; ++i) {
                std::string line_info = addr_to_line(callstack[i]);
                if (!line_info.empty()) {
                    stack_frames.push_back("#" + std::to_string(i) + " " + line_info);
                }
                else {
                    stack_frames.push_back(std::string(strs[i]));
                }
            }
            free(strs);
#endif
            return stack_frames;
        }

        void crash_handler::handle_crash(const std::string& type, const std::string& reason) {
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);

            std::stringstream ss;
            ss << "=====================================================\n";
            ss << "[CRASH REPORT] " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
            ss << "=====================================================\n";
            ss << "- Type: " << type << "\n";
            ss << "- Reason: " << reason << "\n";
            ss << "-----------------------------------------------------\n";
            ss << "[CALL STACK]\n";

            std::vector<std::string> trace = get_stack_trace();
            for (const auto& frame : trace) {
                ss << frame << "\n";
            }
            ss << "=====================================================\n";

            std::string full_log = ss.str();

            if (s_callback) {
                s_callback(full_log);
            }
            else {
                std::cerr << full_log << std::endl;
            }
        }

} 