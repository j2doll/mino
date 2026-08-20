#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "mino/core/string/string.hpp"

// mino network download httplib
#include "mino/network/ethernet.hpp"
#include "mino/network/downloader/httplib/multipart_downloader.hpp"

// 콘솔 출력 헬퍼 정의
const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
std::ostream& (*endl)(std::ostream&) = std::endl;
auto tce = mino::core::string::to_console_encoding;

// 콘솔에 다운로드 진행률을 출력하는 커스텀 콜백 클래스 구현
class ConsoleProgressCallback
    : public mino::network::downloader::httplib::multipart_progress_callback
{
public:
    bool on_progress(long long total_bytes, long long now_bytes) override
    {
        std::ostringstream oss;
        if (total_bytes > 0)
        {
            double percent = (static_cast<double>(now_bytes) / total_bytes) * 100.0;
            oss << "\r[진행 중] "
                << now_bytes << " / " << total_bytes << " bytes "
                << "(" << std::fixed << std::setprecision(1) << percent << "%)";
        }
        else
        {
            oss << "\r[진행 중] " << now_bytes << " bytes 수신됨...";
        }

        // 제자리 갱신(\r) 출력을 위해 std::cout 직접 flush
        std::cout << tce(oss.str()) << std::flush;

        return true;
    }
};

// NOTE: 예제를 테스트하기 전에 python server.py 를 사전에 실행할 것.
int main(int argc, char* argv[])
{
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        eprint(tce("WSAStartup 실패, 에러 코드: "), result);
        return 1;
    }
#endif

    using multipart_downloader = mino::network::downloader::httplib::multipart_downloader;

    // 1. 명령행 인자 기반 URL 및 저장 경로 설정
    std::string test_url = (argc > 1) ? argv[1] : "http://127.0.0.1:8080/download";
    std::string output_dir = (argc > 2) ? argv[2] : "./downloads";

    // 저장 디렉토리가 없으면 생성
    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);
    if (ec)
    {
        eprint(tce("저장 디렉토리 생성 실패: " + ec.message()));
        return 1;
    }

    // 2. 다운로더 객체 및 프로그레스 콜백 생성
    multipart_downloader downloader;

    ConsoleProgressCallback progress_callback;
    downloader.set_progress_callback(&progress_callback); // 진행률 콜백 설정

    downloader.set_delete_partial_file_on_fail(true); // 다운로드 실패 시 부분 파일 삭제 설정

    std::vector<std::string> saved_files;
    std::string error_message;

    print(tce("다운로드 시작: " + test_url));
    print(tce("저장 디렉토리: " + output_dir));

    // 3. multipart 다운로드 수행
    bool success = downloader.download_multipart(
        test_url, // 다운로드할 URL
        output_dir, // 파일 저장 경로
        saved_files, // 다운로드된 파일 경로를 저장할 벡터
        &error_message); // 다운로드 실패 시 에러 메시지 저장

    std::cout << endl; // 진행률 출력 줄바꿈

    // 4. 결과 확인
    if (success)
    {
        print(tce("다운로드 성공! 총 " + std::to_string(saved_files.size())
            + "개의 파일이 저장되었습니다.")); 

        for (size_t i = 0; i < saved_files.size(); ++i)
        {
            print(tce("  - [" + std::to_string(i + 1) + "] " + saved_files[i]));
        }
    }
    else
    {
        eprint(tce("다운로드 실패: " + error_message));
        return 1;
    }

    return 0;
}
