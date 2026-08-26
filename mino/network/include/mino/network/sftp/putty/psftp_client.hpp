#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace mino::network::sftp::putty {

    using progress_callback_t = std::function<void(int)>;

    class psftp_client {
    public:
        psftp_client();
        ~psftp_client();

        // 복사 방지 (리소스 관리)
        psftp_client(const psftp_client&) = delete;
        psftp_client& operator=(const psftp_client&) = delete;

        // SFTP 접속 (현재 프로그램 디렉터리의 psftp 바이너리 구동)
        // hostkey_fingerprint: 배치 모드 연결 시 호스트 키 승인을 위한 SHA256 또는 MD5 핑거프린트
        bool connect(const std::string& host, int port,
            const std::string& user, const std::string& password_or_key_path,
            bool is_key = false,
            const std::string& hostkey_fingerprint = ""
        );

        // 단일 명령 실행 (결과 텍스트 수신 및 에러 검사)
        bool execute(const std::string& cmd,
            std::string& out_response,
            int idle_timeout_seconds = 15);

        // 대용량 파일 전송용 명령 실행 (진행률 콜백 지원)
        bool execute_with_progress(const std::string& cmd,
            std::string& out_response,
            progress_callback_t on_progress,
            int idle_timeout_seconds = 15);

        // 원격 디렉터리 확인 및 계층적 자동 생성
        bool ensure_remote_directory(const std::string& remote_path, int timeout_sec = 15);

        // 안전한 다운로드 (실패 시 불완전 파일 정리 및 이어받기 지원)
        bool download_file(const std::string& remote_file,
            const std::string& local_file,
            progress_callback_t on_progress,
            bool resume = false,
            int idle_timeout = 15);

        // 연결 종료 및 자원 정리
        void disconnect();

    private:
        bool send_command(const std::string& command);
        std::string read_output();
        bool wait_for_prompt(std::string& out_response,
            progress_callback_t on_progress,
            int idle_timeout_seconds);

        std::string to_lower(const std::string& str);
        std::string get_executable_dir();
        std::vector<std::string> split_path(const std::string& path);

        // OS별 핸들 및 프로세스 정보 은닉을 위한 내부 구조체 포인터 (PIMPL 패턴)
        struct platform_context;
        std::unique_ptr<platform_context> context_;

        const std::vector<std::string> error_patterns_;
    };

} // namespace mino::network::sftp::putty
