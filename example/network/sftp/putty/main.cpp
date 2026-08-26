#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/sftp/sftp.hpp"

namespace fs = std::filesystem;

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
    // 1. 실행 파일 디렉터리 기준으로 파일 경로 계산
    fs::path exe_dir = fs::absolute(argv[0]).parent_path();
    fs::path local_upload_path = exe_dir / "large_file.zip";
    fs::path local_download_path = exe_dir / "downloaded_file.zip";

    // 2. 테스트용 더미 파일이 없으면 자동 생성 (1MB)
    if (!fs::exists(local_upload_path)) {
        std::cout << ">> Generating test file: " << local_upload_path.string() << std::endl;
        std::ofstream out(local_upload_path, std::ios::binary);
        std::vector<char> buffer(1024 * 1024, 'A');
        out.write(buffer.data(), buffer.size());
        out.close();
    }

    // Windows 역슬래시(\) 문제를 방지하기 위해 generic_string(/) 사용
    const std::string upload_src = local_upload_path.generic_string();
    const std::string download_dst = local_download_path.generic_string();

    mino::network::sock mnsock;
    using psftp_client = mino::network::sftp::putty::psftp_client;
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
    // std::string hostkey_fingerprint = "";

    if (!client.connect(sftp_ip, sftp_port, sftp_user, sftp_password_or_key, is_key, hostkey_fingerprint)) {
        std::cerr << ">> Connection failed." << std::endl;
        return 1;
    }
    std::cout << ">> Connected successfully!\n" << std::endl;

    std::string response;

    // 3. 디렉터리 확인/생성 및 진입 (/hello 로 이동됨)
    std::string target_dir = "/hello"; // remote dir 
    std::string local_tmp_dir = "C:/tmp"; // local dir : C:/tmp/large_file.zip 존재

    if (!client.ensure_remote_directory(target_dir)) {
        std::cerr << ">> Directory setup failed." << std::endl;
        client.disconnect();
        return 1;
    }
    std::cout << ">> Directory verified & entered: " << target_dir << std::endl;

    // 4. 업로드 실행 (로컬 경로는 슬래시 기반 절대 경로, 원격은 파일명만 전달)
    std::cout << "\n[2] Uploading large_file.zip..." << std::endl;

    // 로컬 작업 디렉터리 변경 (C:/tmp)
    auto lcd_cmd = "lcd " + local_tmp_dir;
    if (!client.execute(lcd_cmd, response, 15)) {
        std::cerr << ">> Failed to change local directory:\n" << response << std::endl;
        client.disconnect();
        return 1;
    }

    // 현재 작업 디렉터리 확인
    client.execute("pwd", response);
    std::cout << "[PWD] " << response << std::endl;

    client.execute("lpwd", response);
    std::cout << "[LPWD] " << response << std::endl;

    std::string put_cmd = "put large_file.zip ";
    auto idle_timeout = 15;
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

    // 5. 다운로드 실행 (원격은 파일명, 로컬은 슬래시 기반 절대 경로)
    std::cout << "\n[3] Downloading large_file.zip..." << std::endl;
    auto remote_file = "large_file.zip";
    auto resume_download = false;
    bool down_success = client.download_file(
        remote_file,
        download_dst,
        [](int percent) { print_progress_bar(percent); },
        resume_download,
        idle_timeout
    );
    std::cout << std::endl;

    if (down_success) {
        std::cout << ">> Download complete." << std::endl;
    }
    else {
        std::cerr << ">> Download failed." << std::endl;
    }

    client.disconnect();
    std::cout << "\n[4] Session disconnected." << std::endl;

    return 0;
}
