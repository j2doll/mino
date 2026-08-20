#include <iostream>
#include <memory>
#include <vector>
#include <mutex>
#include <filesystem>

#include "mino/core/string/string.hpp"

// mino external log spd
#include <spdlog/spdlog.h>
#include "mino/external/log/spd/spd.hpp"

int main() {
    // ------------------------------------------------------------------------
    // (1) 콘솔 출력 람다 및 헬퍼 정의
    // ------------------------------------------------------------------------
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding; 

    // ------------------------------------------------------------------------
    // (2) 함수 내부 네임스페이스 별칭 정의
    // ------------------------------------------------------------------------
    namespace mels = mino::external::log::spd;
    namespace fs = std::filesystem;

    try {
        print(tce("=== 멀티 싱크 로거 초기화 시작 ==="));

        // 1. 콘솔 컬러 싱크 생성
        auto console_sink = std::make_shared<mels::auto_color_sink<std::mutex>>("ConsoleSink");

        // 2. 파일 싱크 생성 설정
        auto log_file_name = "app_log.txt";
        if (fs::exists(log_file_name)) {
            fs::remove(log_file_name); // 기존 로그 파일 삭제 (테스트 용도)
        }

#ifdef _WIN32
        // Windows 환경 설정
        auto encoding_type = mels::log_encoding::cp949; // CP949 (EUC-KR 호환)
        auto line_type = mels::line_ending::crlf;       // CRLF
        bool use_bom = false;
#else
        // Linux/macOS 환경 설정
        auto encoding_type = mels::log_encoding::utf8;  // UTF-8
        auto line_type = mels::line_ending::lf;         // LF
        bool use_bom = false;
#endif

        // 인코딩 및 개행이 지정된 파일 싱크 생성
        auto file_sink = std::make_shared<mels::encoding_file_sink_mt>(
            log_file_name,
            encoding_type,
            line_type,
            use_bom
        );

        // ★ 핵심: 파일 싱크에는 태그(<red>, <bold> 등)를 제거해 주는 'strip_tags_formatter' 설정
        file_sink->set_formatter(std::make_unique<mels::strip_tags_formatter>());

        // 3. 두 개의 싱크를 하나의 복합 로거로 묶기 (콘솔 + 파일)
        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
        auto logger = std::make_shared<::spdlog::logger>("multi_logger", sinks.begin(), sinks.end());
        ::spdlog::register_logger(logger);

        print(tce("로거가 등록되었습니다. 테스트 로그를 출력합니다."));

        // 4. 테스트 로그 출력
        logger->info("<bold>과일 바구니</bold>: <yellow>바나나</yellow>는 노랗고, <red>사과</red>는 빨갛다.");
        logger->warn("<italic>시스템 상태</italic>: 현재 트래픽 <cyan>정상</cyan> 범주이나 메모리는 <orange>주의</orange> 단계입니다.");
        logger->error("<dark_red><bold>[CRITICAL]</bold></dark_red> 데이터베이스 <light_blue>응답 지연</light_blue> 발생");

        logger->flush(); // 로그를 즉시 플러시하여 파일에 기록

        // 결과 설명 안내
        print(endl, tce("=== 로그 출력 완료 ==="));
        print(tce("* 화면: 태그가 파싱되어 컬러 및 볼드/이탤릭 서식이 적용되어 출력되었습니다."));
        print(tce("* 파일("), log_file_name, tce("): 'strip_tags_formatter'에 의해 태그가 제거된 순수 텍스트가 기록되었습니다."));

    }
    catch (const ::spdlog::spdlog_ex& ex) {
        eprint(tce("Failed to initialize logger: "), ex.what());
        return 1;
    }
    catch (const std::exception& ex) {
        eprint(tce("Exception occurred: "), ex.what());
        return 1;
    }

    return 0;
}
