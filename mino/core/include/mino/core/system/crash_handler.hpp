#pragma once

#include <string>
#include <functional>
#include <vector>

#ifdef _WIN32
    // Windows SEH 핸들러
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    // Windows SEH 핸들러
    #include <windows.h> // Windows 전용: _EXCEPTION_POINTERS 사용을 위해 여기서만 포함
#else
    // POSIX 시그널 핸들러 (SIGSEGV, SIGABRT 등)
    #include <signal.h> // POSIX 전용: siginfo_t 사용을 위해 여기서만 포함
#endif 

namespace mino::core::system {

        /**
         * @brief 크래시 발생 시 실행될 콜백 함수 타입
         * @param log_message 크래시 원인 및 콜 스택 정보가 포함된 전체 로그
         */
        using crash_callback = std::function<void(const std::string& log_message)>;

        class  crash_handler {
        public:
            /**
             * @brief 크래시 핸들러를 시스템에 등록합니다.
             * @param callback 크래시 발생 시 실행할 커스텀 로직 (파일 저장, 서버 전송 등)
             */
            static void initialize(crash_callback callback = nullptr);

            /**
             * @brief 현재 시점의 콜 스택(Call Stack) 정보를 문자열 리스트로 가져옵니다.
             * @return std::vector<std::string> 스택 프레임별 정보 (함수명, 주소 등)
             */
            static std::vector<std::string> get_stack_trace();

        private:
            // OS별 네이티브 핸들러
#ifdef _WIN32
            // Windows Structured Exception Handling (SEH) 기반 핸들러
            static long __stdcall windows_exception_handler(struct _EXCEPTION_POINTERS* info);
#else
            // 구현과 동일하게 SA_SIGINFO 용 3-인자 형식으로 선언합니다.
            static void posix_signal_handler(int sig, siginfo_t* si, void* unused);
            static std::string addr_to_line(void* addr);
#endif

            // C++ terminate 핸들러
            static void cxx_terminate_handler();

            static crash_callback s_callback; // 등록된 콜백 함수
            static void handle_crash(const std::string& type, const std::string& reason);
        };
            
} 
