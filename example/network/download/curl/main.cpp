#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>

#include "mino/core/string/string.hpp"

// mino network downloader curl
#include "mino/network/ethernet.hpp"
#include "mino/network/downloader/curl/multipart_downloader.hpp"

// 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 진행률 콜백 인터페이스 구현
class console_progress_callback
    : public mino::network::downloader::curl::multipart_progress_callback
{
public:
    bool on_progress(long long total_bytes, long long now_bytes) override
    {
        if (total_bytes > 0)
        {
            double percentage = (static_cast<double>(now_bytes) / static_cast<double>(total_bytes)) * 100.0;

            std::ostringstream oss;
            oss << "\r[진행률] "
                << std::fixed << std::setprecision(1) << percentage << "%"
                << " (" << now_bytes << " / " << total_bytes << " bytes)";
            std::cout << tce(oss.str()) << std::flush;
        }
        else
        {
            std::ostringstream oss;
            oss << "\r[다운로드 중] " << now_bytes << " bytes";
            std::cout << tce(oss.str()) << std::flush;
        }

        return true;
    }
};

// NOTE: 예제를 테스트하기 전에 python server.py 를 사전에 실행할 것.
int main(int argc, char* argv[])
{
    auto ret_sock = mino::network::init_socket();
    if (ret_sock.has_value()) {
        eprint(tce("[오류] 소켓 초기화 실패: " + ret_sock.value()));
        return 1;
    }

    std::string test_url = (argc > 1) ? argv[1] : "http://127.0.0.1:8080/download";
    std::string output_dir = (argc > 2) ? argv[2] : "./downloads";

    print(tce("========================================"));
    print(tce("Multipart Downloader 테스트 시작"));
    print(tce("URL       : " + test_url));
    print(tce("출력 폴더 : " + output_dir));
    print(tce("========================================"));
      
    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);
    if (ec) {
        eprint(tce("저장 디렉터리 생성 실패: " + ec.message()));
        mino::network::close_socket();
        return 1;
    }

    namespace mndcurl = mino::network::downloader::curl;
    using multipart_downloader = mndcurl::multipart_downloader;

    multipart_downloader downloader;
    console_progress_callback progress_cb; // 진행률 콜백 객체 생성

    downloader.set_progress_callback(&progress_cb); // 진행률 콜백 등록
    downloader.set_delete_partial_file_on_fail(true); // 다운로드 실패 시 임시 파일 삭제 설정

    std::vector<std::string> saved_files;
    std::string error_message;

    bool success = downloader.download_multipart(
        test_url, // 다운로드할 URL
        output_dir, // 파일을 저장할 디렉토리
        saved_files, // 저장된 파일 경로 목록 (출력)
        &error_message); // 오류 메시지 (출력)

    print(""); // 진행률 표시 이후 줄바꿈

    if (success){
        // 다운로드 성공 시 저장된 파일 목록 출력
        auto file_count = std::to_string(saved_files.size());
        print(tce("[성공] 파일 다운로드 완료! (" + file_count + "개 파일)"));

        for (size_t i = 0; i < saved_files.size(); ++i)
        {
            auto& file = saved_files[i];
            print(tce("  - [" + std::to_string(i + 1) + "] " + file));
        }
    } else {
        eprint(tce("[실패] 다운로드 중 오류 발생: " + error_message));
        mino::network::close_socket();
        return 1;
    }

    mino::network::close_socket();
    return 0;
}
