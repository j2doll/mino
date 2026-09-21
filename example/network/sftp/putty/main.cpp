#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/core/log/log.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/sftp/sftp.hpp"

// 진행률 표시 바 길이 설정
void print_progress_bar(int percent) {
    int bar_width = 30;
    std::cout << "\r[";
    int pos = bar_width * percent / 100;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << percent << "% " << std::flush;
}

// 바를 표시할 필요가 없는 경우, percent 값만을 로깅 처리
void print_percent_log(int percent) {
    // 레지스트리에 등록된 "main" 로거를 가져와 퍼센트 출력
    auto logger = mino::core::log::tinylog::logger::get("main");
    if (logger) {
        logger->info("Transfer progress: {}%", percent);
    }
}

int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    namespace mcs = mino::core::string;
    namespace mlog = mino::core::log::tinylog;
    namespace mnsp = mino::network::sftp::putty;
    using psftp_client = mnsp::psftp_client;
    using console_sink_config = mlog::console_sink_config;

    // 1. 메인 어플리케이션 로거 구성
    auto main_logger = std::make_shared<mlog::logger>("main");
    console_sink_config console_config;
#ifdef _WIN32
    console_config.encoding = mlog::encoding_type::cp949;
    console_config.eol = mlog::eol_type::crlf;
#else
    console_config.encoding = mlog::encoding_type::utf8;
    console_config.eol = mlog::eol_type::lf;
#endif
    auto console_sink = std::make_shared<mlog::console_sink>("console", console_config);
    main_logger->add_sink(console_sink);
    mlog::logger::register_logger(main_logger);

    // 2. psftp 클라이언트 초기화 (생성된 로거 전달)
    psftp_client client(main_logger);

    main_logger->info("Target PSFTP Path: {}", client.get_psftp_path());
    main_logger->info("[1] Connecting to SFTP server...");

    auto sftp_ip = "127.0.0.1";
    auto sftp_port = 9022;

    auto sftp_user = "testuser";
    auto sftp_password_or_key = "testpass";
    auto is_key = false;

    // Set your host key fingerprint here (SHA256 or MD5)
    std::string fingerprint = "QXtzSnrG1m4bXyK9qL1fiPLV6Trgu5gFnjT6PNMEixk";
    std::string hostkey_fingerprint = "SHA256:" + fingerprint;

    if (!client.connect(
        sftp_ip, sftp_port,
        sftp_user, sftp_password_or_key, is_key,
        hostkey_fingerprint))
    {
        main_logger->error("<red>Connection failed.</red>");
        return 1;
    }
    main_logger->info("<green>Connected successfully!</green>");

    std::string response;

    // 3. 디렉터리 확인/생성 및 진입
    std::string remote_dir = "/hello";
    if (!client.cd_remote_directory(remote_dir)) {
        main_logger->error("<red>Directory setup failed for: {}</red>", remote_dir);
        client.disconnect();
        return 1;
    }
    main_logger->info("Directory verified & entered: {}", remote_dir);

    // 4. 업로드 실행
    std::string local_tmp_dir = "C:/tmp";
    std::string local_file_name = "large_file.zip";
    main_logger->info("[2] Uploading {} ...", local_file_name);

    auto lcd_cmd = "lcd " + local_tmp_dir;
    auto idle_timeout = 15;
    if (!client.execute(lcd_cmd, response, idle_timeout)) {
        main_logger->error("<red>Failed to change local directory:</red>\n{}", response);
        client.disconnect();
        return 1;
    }

    client.execute("pwd", response);
    main_logger->info("Remote pwd: {}", mcs::replace(response, "\n", " "));
    client.execute("lpwd", response);
    main_logger->info("Local lpwd: {}", mcs::replace(response, "\n", " "));

    std::string put_cmd = "put " + local_file_name;
    bool up_success = client.execute_with_progress(
        put_cmd,
        response,
        [](int percent) { print_progress_bar(percent); },
        idle_timeout
    );
    std::cout << std::endl; // 진행 바 개행

    if (up_success) {
        main_logger->info("<green>Upload complete.</green>");
    }
    else {
        main_logger->error("<red>Upload failed:</red>\n{}", response);
    }

    client.execute("pwd", response);
    main_logger->info("Remote pwd: {}", mcs::replace(response, "\n", " "));
    client.execute("lpwd", response);
    main_logger->info("Local lpwd: {}", mcs::replace(response, "\n", " "));

    // 5. 다운로드 실행
    std::string remote_file = "large_file.zip";
    main_logger->info("[3] Downloading {} ...", remote_file);

    bool resume_download = false;
    std::string download_dst = local_tmp_dir + "/other_download.zip";
    bool down_success = client.download_file(
        remote_file,
        download_dst,
        [](int percent) { print_progress_bar(percent); },
        resume_download,
        idle_timeout
    );
    std::cout << std::endl; // 진행 바 개행

    if (down_success) {
        main_logger->info("<green>Download complete.</green>");
    }
    else {
        main_logger->error("<red>Download failed.</red>");
    }

    // 6. 연결 종료
    client.disconnect();
    main_logger->info("[4] Session disconnected.");

    return 0;
}
