#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

#include "mino/core/string/print.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/ftp/tcp/ftp_client.hpp"

#include "mino/network_openssl/ftps/ftps_client.hpp"

namespace mcsp = mino::core::string::print;

// print.hpp 유틸리티를 활용한 진행률 리스너 (snake_case 규격)
class custom_progress_listener : public mino::network::ftp::tcp::i_progress_listener {
public:
    void on_progress(std::int64_t dlnow, std::int64_t dltotal,
        std::int64_t ulnow, std::int64_t ultotal) override {
        if (dltotal > 0) {
            double pct = (static_cast<double>(dlnow) / static_cast<double>(dltotal)) * 100.0;
            mcsp::print("\r[FTPS Download] {}/{} Bytes ({}%)", dlnow, dltotal, static_cast<int>(pct));
            if (dlnow >= dltotal) {
                mcsp::println("");
            }
        }
        if (ultotal > 0) {
            double pct = (static_cast<double>(ulnow) / static_cast<double>(ultotal)) * 100.0;
            mcsp::print("\r[FTPS Upload] {}/{} Bytes ({}%)", ulnow, ultotal, static_cast<int>(pct));
            if (ulnow >= ultotal) {
                mcsp::println("");
            }
        }
    }
};

int main(int argc, char* argv[]) {
    // 1. 소켓 라이프사이클 관리 (Windows WSAStartup/WSACleanup 자동 처리)
    mino::network::sock socket_guard;

    // 2. 서버 접속 기본 매개변수 설정
    std::string ftps_host = "127.0.0.1";
    int ftps_port = 990; 
    std::string ftps_user = "test";
    std::string ftps_pass = "test"; 

    if (argc >= 4) {
        ftps_host = argv[1];
        ftps_port = std::stoi(argv[2]);
        ftps_user = argv[3];
        ftps_pass = (argc >= 5) ? argv[4] : "";
    }

    mcsp::println("========================================");
    mcsp::println("  FTPS Client Test Routine (Implicit)   ");
    mcsp::println("  Target: {}:{}", ftps_host, ftps_port);
    mcsp::println("========================================");

    // 3. ftps_client 인스턴스 초기화 및 리스너 등록
    mino::network_openssl::ftps::ftps_client client;
    client.set_tls_verification(false);
    client.set_data_protection_mode(mino::network_openssl::ftps::ftps_data_protection::private_ssl);

    custom_progress_listener progress;
    client.set_progress_listener(&progress);

    // 4. FTPS 접속 및 로그인
    mcsp::println("[Step 1] Connecting to FTPS server...");
    if (!client.connect(ftps_host, ftps_port, ftps_user, ftps_pass)) {
        mcsp::eprintln("[-] Connect failed: {}", client.get_last_error());
        return 1;
    }
    mcsp::println("[+] Connected and authenticated successfully!");

    // 5. 디렉터리 목록 조회
    mcsp::println("\n[Step 2] Listing root directory contents...");
    std::vector<mino::network_openssl::ftps::file_info> files = client.list_directory("/");
    mcsp::println("[+] Found {} entries:", files.size());
    for (const auto& item : files) {
        mcsp::println("    - {} {} ({} bytes)",
            (item.is_directory ? "[DIR]  " : "[FILE] "),
            item.name,
            item.size);
    }

    // 6. 디렉터리 생성 및 삭제 테스트
    std::string test_dir = "mino_test_dir";
    mcsp::println("\n[Step 3] Creating directory '{}'...", test_dir);
    if (client.create_directory(test_dir)) {
        mcsp::println("[+] Directory created.");
        if (client.remove_directory(test_dir)) {
            mcsp::println("[+] Directory removed successfully.");
        }
        else {
            mcsp::eprintln("[-] Failed to remove directory: {}", client.get_last_error());
        }
    }
    else {
        mcsp::eprintln("[-] Directory creation failed: {}", client.get_last_error());
    }

    // 7. 업로드 테스트용 로컬 파일 생성
    std::string local_test_file = "ftps_test_upload.txt";
    std::string remote_test_file = "ftps_remote_test.txt";
    std::string downloaded_file = "ftps_test_downloaded.txt";

    {
        std::ofstream dummy_out(local_test_file, std::ios::binary);
        for (int i = 0; i < 50000; ++i) {
            dummy_out << "FTPS transmission test line: " << i << "\n";
        }
    }

    // 8. 파일 업로드 테스트
    mcsp::println("\n[Step 4] Uploading file '{}' -> '{}'...", local_test_file, remote_test_file);
    if (!client.upload(local_test_file, remote_test_file)) {
        mcsp::eprintln("[-] Upload failed: {}", client.get_last_error());
    }
    else {
        mcsp::println("[+] File uploaded successfully.");
    }

    // 9. 파일 다운로드 테스트
    mcsp::println("\n[Step 5] Downloading file '{}' -> '{}'...", remote_test_file, downloaded_file);
    if (!client.download(remote_test_file, downloaded_file)) {
        mcsp::eprintln("[-] Download failed: {}", client.get_last_error());
    }
    else {
        mcsp::println("[+] File downloaded successfully.");
    }

    // 10. 원격 테스트 파일 삭제
    mcsp::println("\n[Step 6] Deleting remote file '{}'...", remote_test_file);
    if (!client.delete_file(remote_test_file)) {
        mcsp::eprintln("[-] Delete failed: {}", client.get_last_error());
    }
    else {
        mcsp::println("[+] Remote file deleted successfully.");
    }

    client.remove_progress_listener();
    mcsp::println("\n========================================");
    mcsp::println("     All FTPS tests finished!           ");
    mcsp::println("========================================");

    return 0;
}
