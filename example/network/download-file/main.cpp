#include <iostream>
#include <filesystem>

#include "mino/core/string/string.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network/downloader/curl/file_downloader.hpp"

int main(int argc, char* argv[]) {
    namespace mcs = mino::core::string;
    namespace mn = mino::network;
    namespace mndc = mino::network::downloader::curl;

    using file_downloader = mndc::file_downloader;
    auto tce = mino::core::string::to_console_encoding;

    mn::sock mnsock;

    file_downloader downloader;

    downloader.set_progress_callback([](curl_off_t dlnow, curl_off_t dltotal) {
        auto tce = mino::core::string::to_console_encoding;
        if (dltotal > 0) {
            double percentage = (static_cast<double>(dlnow) / static_cast<double>(dltotal)) * 100.0;
            auto ret = tce("\r진행률: ") + std::to_string(static_cast<int>(percentage)) + "% (" + std::to_string(dlnow) + " / " + std::to_string(dltotal) + " bytes)";
            std::cout << ret << std::flush;
        }
        else {
            auto ret = tce("\r다운로드 중: ") + std::to_string(dlnow) + " bytes";
            std::cout << ret << std::flush;
        }
        return true;
    });

    std::string sample_url
        = "https://raw.githubusercontent.com/j2doll/j2doll/refs/heads/main/README.md";

    // 1. 수동 이름 지정 다운로드 방식
    std::filesystem::path manual_path = "./README.md";
    std::cout << tce("=== 1. 수동 이름 지정 다운로드 테스트 ===\n")
        << tce(" 다운로드 파일명: ") << manual_path.string() << "\n\n";

    if (downloader.download(sample_url, manual_path)) {
        std::cout
            << tce("\n[성공] 수동 저장 완료: ")
            << std::filesystem::absolute(manual_path).string() << "\n\n";
    }
    else {
        std::cerr
            << tce("\n[실패] 수동 다운로드 실패! 원인: ")
            << downloader.get_last_error() << "\n\n";
    }

    // 2. 자동 이름 지정 다운로드 방식
    std::filesystem::path target_directory = "./downloads";
    std::cout << tce("=== 2. 자동 이름 설정 다운로드 테스트 ===\n")
        << tce(" 다운로드 경로: ") << target_directory.string() << "\n\n";

    std::filesystem::path auto_saved_path = downloader.download_auto_name(sample_url, target_directory);

    if (!auto_saved_path.empty()) {
        std::cout << tce("\n[성공] 자동 저장 완료!\n");
        std::cout
            << tce(" - 최종 경로: ")
            << std::filesystem::absolute(auto_saved_path).string() << "\n";
        std::cout
            << tce(" - 추출된 파일명: ")
            << auto_saved_path.filename().string() << "\n\n";
    }
    else {
        std::cerr
            << tce("\n[실패] 자동 다운로드 실패! 원인: ")
            << downloader.get_last_error() << "\n\n";
    }

    return 0;
}
