#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "mino/core/string/string.hpp"
#include "mino/core/log/log.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/ssh/ssh.hpp"

int main(int argc, char* argv[]) {
    namespace mn = mino::network;
    namespace mclt = mino::core::log::tinylog;
    namespace mns = mino::network::ssh;

    using ssh_client = mns::ssh_client;
    using reconnect_policy = mns::reconnect_policy;
    using session_state = mns::session_state;
    using logger = mclt::logger;
    using log_level = mclt::log_level;
    using console_sink = mclt::console_sink;
    using console_sink_config = mclt::console_sink_config;
    using encoding_type = mclt::encoding_type;
    using eol_type = mclt::eol_type;

    // 네트워크 소켓 초기화
    mn::sock mnsock;

    // tinylog 인스턴스 생성 및 콘솔 싱크 연결
    console_sink_config console_cfg;
#ifdef _WIN32
    console_cfg.encoding = encoding_type::cp949;
    console_cfg.eol = eol_type::crlf;
#else
    console_cfg.encoding = encoding_type::utf8;
    console_cfg.eol = eol_type::lf;
#endif
    auto console = std::make_shared<console_sink>("console", console_cfg);
    auto log = std::make_shared<logger>("ssh_app");
    log->add_sink(console);
    log->set_level(log_level::trace);
        
    ssh_client client;
    client.set_logger(log);

    // 서버 및 계정 정보 개별 설정
    client.set_host("127.0.0.1");
    client.set_port(2222);

    client.set_user("admin");
    client.set_password("secret123");

    // 지수 백오프 재연결 정책
    reconnect_policy policy;
    policy.enabled = true;
    policy.max_retries = 5;
    policy.initial_delay = std::chrono::milliseconds(1000);
    policy.max_delay = std::chrono::milliseconds(15000);
    policy.backoff_multiplier = 2.0;
    client.set_reconnect_policy(policy);

    // 세션 상태 변화 로거 등록
    client.set_on_state_changed([log](session_state state) {
            const char* state_str = "UNKNOWN";
            switch (state) {
                case session_state::disconnected:  state_str = "DISCONNECTED";  break;
                case session_state::connecting:    state_str = "CONNECTING";    break;
                case session_state::handshaking:   state_str = "HANDSHAKING";   break;
                case session_state::authenticated: state_str = "AUTHENTICATED"; break;
            }
            log->info("<bright_cyan>[STATE]</bright_cyan> 상태 변경 -> <bold>{}</bold>", state_str);
        });

    // 호스트 키 지문 검증 콜백
    client.set_host_key_verifier([log](const std::string& host, const std::string& fingerprint) -> bool {
        log->warn("<bright_yellow>[호스트 검증]</bright_yellow> 호스트: {}, 지문: {}", host, fingerprint);
        return true;
        });

    // 비동기 수신 데이터 로거 출력 (stdout / stderr 분기)
    client.set_on_stdout([log](uint32_t channel_id, const std::string& data) {
        std::cout << data << std::flush;
        // log-> 를 사용하면 1글자가 1줄씩 출력되므로, 실시간 출력이 필요한 경우 std::cout 사용을 권장
        });

    client.set_on_stderr([log](uint32_t channel_id, const std::string& data) {
        std::cerr << "\033[31m" << data << "\033[0m" << std::flush;
        // log-> 를 사용하면 1글자가 1줄씩 출력되므로, 실시간 출력이 필요한 경우 std::cerr 사용을 권장
        });

    client.set_on_authenticated([log]() {
        log->info("<bright_green>[+] SSH 인증 성공! 명령어를 입력하세요. ('exit' 입력 시 종료)</bright_green>");
        });

    client.set_on_disconnect([log](uint32_t reason, const std::string& msg) {
        log->warn("<bright_yellow>[-] 연결 단절 통보 (사유: {}). 대기 큐 초기화 완료.</bright_yellow>", msg);
        });

    // 워커 기동
    log->info("[*] SSH 백그라운드 클라이언트 시작...");
    client.start();

    // 메인 스레드 명령어 입력 루프 (키보드 입력을 위해 std::cin 사용)
    bool is_windows_server = false; // 리눅스 서버 접속 시 false
    std::string eol = is_windows_server ? "\r\n" : "\n";

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "exit" || input == "quit") {
            client.send_channel_data("exit" + eol);
            break;
        }

        if (client.is_authenticated()) {
            client.send_channel_data(input + eol);
        }
        else {
            log->warn("<bright_yellow>[-] 아직 서버에 인증되지 않았습니다.</bright_yellow>");
        }
    }

    log->info("[*] SSH 클라이언트 정상 종료 처리 중...");
    client.stop();
    log->info("<bright_green>[+] 프로그램이 안전하게 종료되었습니다.</bright_green>");

    return 0;
}
