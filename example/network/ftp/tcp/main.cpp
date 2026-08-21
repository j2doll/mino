#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#   include <winsock2.h>
#   include <ws2tcpip.h>
#endif

#include "mino/core/string/string.hpp"

// mino network ftp tcp
#include "mino/network/ethernet.hpp"
#include "mino/network/ftp/tcp/ftp_client.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 공통 테스트 루틴 (FTP, SFTP 공용)
void run_client_test(
    const std::string& label,
    mino::network::ftp::tcp::ftp_client_base& client,
    const std::string& host,
    int port,
    const std::string& user,
    const std::string& pass);

// NOTE: main 구동 하기 전에 다음을 수행한다.
//   python ftp_server.py
//   python sftp_server.py
int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        eprint(tce("WSAStartup 실패, 에러 코드: "), result);
        return 1;
    }
#endif

    namespace mnftcp = mino::network::ftp::tcp;
    using ftp_client = mnftcp::ftp_client;
    using sftp_client = mnftcp::sftp_client;

    // --- [1] FTP 테스트 ---
    ftp_client ftp;
    run_client_test("FTP", ftp, "127.0.0.1", 50021, "test_user", "test_password");

    // --- [2] SFTP 테스트 ---
    sftp_client sftp;
    run_client_test("SFTP", sftp, "127.0.0.1", 50022, "sftp_user", "sftp_pass");

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

// ============================================================================
// 커스텀 프로그레스 리스너 (게이지 바 및 상세 전송량 표시)
// ============================================================================
class custom_progress_listener
    : public mino::network::ftp::tcp::i_progress_listener {
private:
    std::string current_operation;

    void draw_bar(const std::string& type, std::int64_t current, std::int64_t total) {
        const int bar_width = 30;
        double ratio = (total > 0) ? (static_cast<double>(current) / total) : 0.0;
        if (ratio > 1.0) ratio = 1.0;

        int pos = static_cast<int>(bar_width * ratio);

        std::ostringstream oss;
        oss << "\r" << type << " [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) oss << "=";
            else if (i == pos) oss << ">";
            else oss << " ";
        }
        oss << "] " << std::fixed << std::setprecision(1) << (ratio * 100.0) << "% ("
            << current << " / " << total << " bytes)";

        std::cout << tce(oss.str()) << std::flush;

        if (current >= total && total > 0) {
            std::cout << "\n";
        }
    }

public:
    void on_progress(std::int64_t dlnow, std::int64_t dltotal,
        std::int64_t ulnow, std::int64_t ultotal) override {
        if (dltotal > 0) {
            draw_bar("[Download Progress]", dlnow, dltotal);
        }
        if (ultotal > 0) {
            draw_bar("[Upload Progress]  ", ulnow, ultotal);
        }
    }
};

// 공통 테스트 루틴 (FTP, SFTP 공용)
void run_client_test(
    const std::string& label, // 테스트용 라벨 "FTP" 또는 "SFTP"
    mino::network::ftp::tcp::ftp_client_base& client, // FTP/SFTP 클라이언트 객체
    const std::string& host, // FTP/SFTP 서버 호스트
    int port, // FTP/SFTP 서버 포트
    const std::string& user, // 사용자 이름
    const std::string& pass) // 사용자 비밀번호
{
    namespace mnftcp = mino::network::ftp::tcp;
    using filtered_progress_listener = mnftcp::filtered_progress_listener;

    print(tce("\n=================================================="));
    print(tce("  [" + label + " Test Start] Host: " + host + ":" + std::to_string(port)));
    print(tce("=================================================="));

    // 1. 커스텀 프로그레스 리스너 설정 (1% 단위 또는 50ms 주기로 필터링)
    custom_progress_listener custom_listener;
    filtered_progress_listener filtered(&custom_listener, 1, 50); // 1% 단위 변화 또는 50ms 주기로 필터링
    client.set_progress_listener(&filtered); // 진행률 리스너 설정

    // 2. 연결 테스트
    print(tce(">> [1] Connecting..."));
    if (!client.connect(host, port, user, pass)) {
        eprint(tce("[-] Connect Failed: " + client.get_last_error()));
        return;
    }
    print(tce("[+] Connected successfully!"));

    // 3. 디렉터리 목록 조회
    print(tce("\n>> [2] Listing directory (.)..."));
    auto files = client.list_directory(".");
    for (const auto& item : files) {
        std::ostringstream oss;
        oss << "  - " << (item.is_directory ? "[DIR] " : "[FILE]")
            << std::setw(10) << item.size << " bytes : " << item.name;
        print(tce(oss.str()));
    }

    // 4. 업로드용 임시 파일 생성 (진행률 확인을 위해 크기 상향: 50000 라인)
    std::string local_file = "test_data_" + label + ".txt";
    std::string remote_file = "remote_" + label + ".txt";
    std::string downloaded_file = "downloaded_" + label + ".txt";

    {
        // 50000 라인짜리 테스트 파일 생성
        std::ofstream ofs(local_file);
        for (int i = 0; i < 50000; ++i) {
            ofs << label
                << " Client Custom Progress Test Payload Line #"
                << i << "\n";
        }
    }

    // 5. 파일 업로드
    print(tce("\n>> [3] Uploading file: " + local_file + " -> " + remote_file));
    if (!client.upload(local_file, remote_file)) {
        eprint(tce("[-] Upload Failed: " + client.get_last_error()));
    }
    else {
        print(tce("[+] Upload Success!"));
    }

    // 6. 파일 다운로드
    print(tce("\n>> [4] Downloading file: " + remote_file + " -> " + downloaded_file));
    if (!client.download(remote_file, downloaded_file)) {
        eprint(tce("[-] Download Failed: " + client.get_last_error()));
    }
    else {
        print(tce("[+] Download Success!"));
    }

    // 7. 원격 디렉터리 생성 및 삭제 테스트
    std::string test_dir = "test_folder_" + label;
    print(tce("\n>> [5] Directory Operations: " + test_dir));
    if (client.create_directory(test_dir)) {
        print(tce("[+] Created directory: " + test_dir));
        if (client.remove_directory(test_dir)) {
            print(tce("[+] Removed directory: " + test_dir));
        }
        else {
            eprint(tce("[-] Remove directory Failed: " + client.get_last_error()));
        }
    }
    else {
        eprint(tce("[-] Create directory Failed: " + client.get_last_error()));
    }

    // 8. 업로드했던 원격 파일 삭제
    print(tce("\n>> [6] Cleaning up remote file: " + remote_file));
    if (client.delete_file(remote_file)) {
        print(tce("[+] Remote file deleted."));
    }
    else {
        eprint(tce("[-] Delete remote file Failed: " + client.get_last_error()));
    }

    print(tce("=================================================="));
    print(tce("  [" + label + " Test Finished]"));
    print(tce("==================================================\n"));
}
