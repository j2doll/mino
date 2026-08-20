#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <iomanip>
#include <sstream>

#include "mino/core/string/string.hpp"

// mino network ftp curl
#include "mino/network/ethernet.hpp"
#include "mino/network/ftp/curl/ftp_client.hpp"

// 콘솔 출력 헬퍼 및 함수 포인터 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 직접 정의하는 커스텀 진행 현황 리스너
class custom_progress_listener
    : public mino::network::ftp::curl::i_progress_listener
{
public:
    void on_progress(std::int64_t dlnow, std::int64_t dltotal,
        std::int64_t ulnow, std::int64_t ultotal) override {
        if (dlnow < 0 || dltotal < 0 || ulnow < 0 || ultotal < 0) {
            return;
        }

        // 다운로드 진행 상황 포맷
        if (dltotal > 0) {
            double pct = (static_cast<double>(dlnow) / dltotal) * 100.0;
            std::ostringstream oss;
            oss << "[Custom Listener] 다운로드 중: "
                << std::fixed << std::setprecision(1) << pct << "% "
                << "(" << dlnow << " / " << dltotal << " bytes)";
            print(tce(oss.str()));
        }

        // 업로드 진행 상황 포맷
        if (ultotal > 0) {
            double pct = (static_cast<double>(ulnow) / ultotal) * 100.0;
            std::ostringstream oss;
            oss << "[Custom Listener] 업로드 중: "
                << std::fixed << std::setprecision(1) << pct << "% "
                << "(" << ulnow << " / " << ultotal << " bytes)";
            print(tce(oss.str()));
        }
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        eprint(tce("WSAStartup 실패, 에러 코드: "), result);
        return 1;
    }
#endif

    namespace mnfcurl = mino::network::ftp::curl;
    using ftp_client = mnfcurl::ftp_client;
    using sftp_client = mnfcurl::sftp_client;
    using filtered_progress_listener = mnfcurl::filtered_progress_listener;
    using file_info = mnfcurl::file_info;

    print(tce("=== CURL 지원 프로토콜 목록 ==="));
    print(tce(mnfcurl::get_curl_supported_protocols()));
    print(tce("===============================\n"));

    // 0. 업로드용 로컬 테스트 파일 자동 생성
    const std::string local_file_path = "local_test.txt";
    {
        std::ofstream ofs(local_file_path);
        if (ofs.is_open()) {
            ofs << "Hello, this is an automated test file for FTP/SFTP upload.\n";
            ofs << "Created at runtime by main.cpp.\n";
            ofs.close();
            print(tce("[Local] 업로드용 임시 파일 생성 완료: " + local_file_path));
        }
        else {
            eprint(tce("[Local] 업로드용 파일 생성 실패: " + local_file_path));
        }
    }

    // 1. 프로그레스 리스너 설정 (main.cpp에 정의한 커스텀 리스너 주입)
    auto base_listener = std::make_shared<custom_progress_listener>();
    filtered_progress_listener progress_filter(base_listener, 1, 200); // 1% 이상 변화 또는 200ms 이상 경과 시 이벤트 전달

    // 2. FTP 클라이언트 인스턴스 생성 및 리스너 등록
    ftp_client ftp;
    ftp.set_progress_listener(&progress_filter); // 리스너 등록

    std::string ftp_host = "127.0.0.1";
    int ftp_port = 50021;
    std::string ftp_user = "test_user";
    std::string ftp_pass = "test_password";

    print(tce("[FTP] " + ftp_host + ":" + std::to_string(ftp_port) + " 에 접속 시도..."));
    if (!ftp.connect(ftp_host, ftp_port, ftp_user, ftp_pass)) {
        eprint(tce("[FTP] 접속 실패: " + ftp.get_last_error()));
    }
    else {
        print(tce("[FTP] 접속 성공"));

        // FTP 파일 업로드 테스트
        print(tce("[FTP] 파일 업로드 시도..."));
        if (!ftp.upload(local_file_path, "/remote/test.txt")) {
            eprint(tce("[FTP] 업로드 실패: " + ftp.get_last_error()));
        }

        // FTP 파일 다운로드 테스트
        print(tce("[FTP] 파일 다운로드 시도..."));
        if (!ftp.download("/remote/test.txt", "downloaded_test.txt")) {
            eprint(tce("[FTP] 다운로드 실패: " + ftp.get_last_error()));
        }

        // FTP 디렉터리 목록 조회 테스트
        print(tce("[FTP] 디렉터리 목록 조회..."));
        std::vector<file_info> files = ftp.list_directory("/remote");
        for (const auto& file : files) {
            std::string file_desc = " - " + std::string(file.is_directory ? "[DIR] " : "[FILE] ")
                + file.name + " (" + std::to_string(file.size) + " bytes)";
            print(tce(file_desc));
        }
    }

    print(tce("\n----------------------------------------\n"));

    // 3. SFTP 클라이언트 인스턴스 테스트
    sftp_client sftp;
    sftp.set_progress_listener(&progress_filter); // 리스너 등록

    std::string sftp_host = "127.0.0.1";
    int sftp_port = 50022;
    std::string sftp_user = "sftp_user";
    std::string sftp_pass = "sftp_pass";

    print(tce("[SFTP] " + sftp_host + ":" + std::to_string(sftp_port) + " 에 접속 시도..."));
    if (!sftp.connect(sftp_host, sftp_port, sftp_user, sftp_pass)) {
        eprint(tce("[SFTP] 접속 실패: " + sftp.get_last_error()));
    }
    else {
        print(tce("[SFTP] 접속 성공"));

        // SFTP 파일 업로드 테스트
        print(tce("[SFTP] 파일 업로드 시도..."));
        if (!sftp.upload(local_file_path, "/remote/sftp_test.txt")) {
            eprint(tce("[SFTP] 업로드 실패: " + sftp.get_last_error()));
        }

        // SFTP 파일 다운로드 테스트
        print(tce("[SFTP] 파일 다운로드 시도..."));
        if (!sftp.download("/remote/sftp_test.txt", "downloaded_sftp_test.txt")) {
            eprint(tce("[SFTP] 다운로드 실패: " + sftp.get_last_error()));
        }

        // SFTP 디렉터리 목록 조회 테스트
        print(tce("[SFTP] 디렉터리 목록 조회..."));
        std::vector<file_info> files = sftp.list_directory("/remote");
        for (const auto& file : files) {
            std::string file_desc = " - " + std::string(file.is_directory ? "[DIR] " : "[FILE] ")
                + file.name + " (" + std::to_string(file.size) + " bytes)";
            print(tce(file_desc));
        }
    }

    // 4. 리스너 해제
    ftp.remove_progress_listener();
    sftp.remove_progress_listener();

    return 0;
}
