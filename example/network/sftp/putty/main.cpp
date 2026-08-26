#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/sftp/sftp.hpp"

void print_progress_bar(int percent) {
    int bar_width = 30;
    std::cout << "\r[";
    int pos = bar_width * percent / 100;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << percent << "% " << std::flush;
}

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    namespace mcs = mino::core::string;

    namespace mnsp = mino::network::sftp::putty;
    using psftp_client = mnsp::psftp_client;
    psftp_client client;

    std::cout << "[1] Connecting to SFTP server..." << std::endl;

    auto sftp_ip = "127.0.0.1";
    // auto sftp_port = 8022;
    auto sftp_port = 9022;

    auto sftp_user = "testuser";
    auto sftp_password_or_key = "testpass";
    auto is_key = false;

    // std::string fingerprint = "y6CIn2/HAPNl+OfRXmrmKJvgINcXy5wCampYqfC3Szw"; // 8022
    std::string fingerprint = "QXtzSnrG1m4bXyK9qL1fiPLV6Trgu5gFnjT6PNMEixk"; // 9022
    std::string hostkey_fingerprint = "SHA256:" + fingerprint;

    if (!client.connect(
        sftp_ip, sftp_port, // SFTP 서버 IP와 포트
        sftp_user, sftp_password_or_key, is_key, // 사용자명과 비밀번호 또는 키 경로(키 사용 시)
        hostkey_fingerprint)) // 호스트 키 핑거프린트 (배치 모드 연결 시 필요)
    {
        std::cerr << ">> Connection failed." << std::endl;
        return 1;
    }
    std::cout << ">> Connected successfully!\n" << std::endl;

    std::string response;

    // 3. 디렉터리 확인/생성 및 진입 (/hello 로 이동됨)
    std::string remote_dir = "/hello"; // Remote dir 
    if (!client.cd_remote_directory(remote_dir)) { // 원격 경로로 이동
        std::cerr << ">> Directory setup failed." << std::endl;
        client.disconnect();
        return 1;
    }
    std::cout << ">> Directory verified & entered: " << remote_dir << std::endl;

    // 4. 업로드 실행 (로컬 경로는 슬래시 기반 절대 경로, 원격은 파일명만 전달)
    std::string local_tmp_dir = "C:/tmp"; // Local dir
    std::string local_file_name = "large_file.zip"; // Local file name
    std::cout << "\n[2] Uploading " << local_file_name << " ..." << std::endl;
    auto lcd_cmd = "lcd " + local_tmp_dir; // local cd
    auto idle_timeout = 15; // idle timeout in seconds
    if (!client.execute(lcd_cmd, response, idle_timeout)) { // 로컬 작업 디렉터리 변경 (C:/tmp)
        std::cerr << ">> Failed to change local directory:\n" << response << std::endl;
        client.disconnect();
        return 1;
    }
    // NOTE: 경로에 공란(space)가 있는 경우, \" 로 감싸야 함. 예: lcd "C:/My Documents"
    // 가능하면 경로에 공란이 없는 경로를 사용하는 것이 좋음.

    // 현재 작업 디렉터리 확인
    std::cout << "------------------------\n";
    client.execute("pwd", response);
    std::cout << mcs::replace(response, "\n", " ") << std::endl;
    client.execute("lpwd", response);
    std::cout << mcs::replace(response, "\n", " ") << std::endl;
    std::cout << "------------------------\n";

    // 업로드 명령 (put)
    std::string put_cmd = "put " + local_file_name; // "put large_file.zip"
    bool up_success = client.execute_with_progress(
        put_cmd,
        response,
        [](int percent) { print_progress_bar(percent); },
        idle_timeout
    );
    std::cout << std::endl;

    if (up_success) {
        std::cout << ">> Upload complete." << std::endl;
    }
    else {
        std::cerr << ">> Upload failed:\n" << response << std::endl;
    }

    // 현재 작업 디렉터리 확인
    std::cout << "------------------------\n";
    client.execute("pwd", response);
    std::cout << mcs::replace(response, "\n", " ") << std::endl;
    client.execute("lpwd", response);
    std::cout << mcs::replace(response, "\n", " ") << std::endl;
    std::cout << "------------------------\n";

    // 5. 다운로드 실행 (원격은 파일명, 로컬은 슬래시 기반 절대 경로)
    std::string remote_file = "large_file.zip";
    std::cout << "\n[3] Downloading " << remote_file << " ..." << std::endl;

    bool resume_download = false; // 이어받기 여부 (true: 이어받기, false: 새로 다운로드)
    std::string download_dst = local_tmp_dir + "/other_download.zip"; // 다운로드 대상 경로 (C:/tmp/other_download.zip)
    bool down_success = client.download_file( // 다운로드 실행
        remote_file,
        download_dst,
        [](int percent) { print_progress_bar(percent); },
        resume_download,
        idle_timeout
    );
    std::cout << std::endl;

    if (down_success) {
        std::cout << ">> Download complete." << std::endl;
    } else {
        std::cerr << ">> Download failed." << std::endl;
    }

    client.disconnect();
    std::cout << "\n[4] Session disconnected." << std::endl;

    return 0;
}
