#include "mino/external/log/sqlite/sqlite.hpp"

#include "mino/core/datetime/datetime.hpp"
#include "mino/core/string/string.hpp"
#include "mino/core/log/log.hpp" 

#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <set>
#include <queue>

// 멀티스레드 환경에서 독립적으로 호출할 수 있는 로깅 함수 예시
void worker_log_task(int thread_id, const std::string& table_name) {
    using namespace mino::external::log::sqlite;

    // 각 스레드 내에서 포맷팅과 사용자 정의 타입을 활용한 로깅 수행
    auto now = mino::core::datetime::unit::date_time::current_date_time();

    sqlog_info(table_name, "Worker thread {0} started at {1}", thread_id, now);

    // 스레드별 데이터 시뮬레이션
    std::vector<int> task_metrics = { thread_id * 10, thread_id * 20, thread_id * 30 };
    sqlog_debug(table_name, "Thread {0} metrics vector: {1}", thread_id, task_metrics);

    // 약간의 작업 지연 시뮬레이션
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sqlog_info(table_name, "Worker thread {0} successfully finished its task.", thread_id);
}

int main() {
    namespace mcs = mino::core::string;
    namespace mcsu8 = mino::core::string::u8;
    using u8str = mcsu8::u8str;
    using mutex_string = mcs::mutex_string;

    namespace mels = mino::external::log::sqlite;
    using sqlite_logger = mels::sqlite_logger;
    using logger_config = mels::logger_config;

    // 1. 싱글톤 인스턴스 참조 가져오기
    auto& slog = sqlite_logger::get_instance();

    // 2. 로거 설정 구성
    logger_config config;
    config.db_path = "application.db";
    config.max_capacity_bytes = 20 * 1024 * 1024; // 20MB 제한
    config.table_names = { "system_logs", "user_logs" };

    // 3. 로거 초기화
    if (auto err = slog.initialize(config)) {
        std::cerr << "Logger initialization failed: " << *err << "\n";
        return 1;
    }

    std::cout << "DB Path: " << slog.get_db_path().string() << "\n";
    std::cout << "Max Capacity (Bytes): " << slog.get_max_capacity_bytes() << "\n\n";

    // -------------------------------------------------------------------------
    // 4. 기본 로그 레벨 및 포맷팅 테스트 (순차적 '{}' 및 인덱스 지정형 '{0}, {1}')
    // -------------------------------------------------------------------------
    mels::sqlog_trace("system_logs", "Trace message with sequential args: {}, {}", 1, "start");
    mels::sqlog_debug("system_logs", "Debug message with indexed args: {1}, {0}", "world", "Hello");
    mels::sqlog_info("system_logs", "Info message with repeated index: hello {1}, {0}, {1}", 100, 42);

    // -------------------------------------------------------------------------
    // 5. 사용자 정의 타입 및 컨테이너 출력 테스트
    // -------------------------------------------------------------------------
    auto current_dt = mino::core::datetime::unit::date_time::current_date_time();
    auto span = mino::core::datetime::unit::time_span::from_minutes(30);
    mino::core::string::u8::u8str utf8_msg("유니코드 한글 및 특수문자 ● 😀 u8str 테스트");
    mino::core::string::mutex_string safe_msg("스레드 안전한 mutex_string 객체");

    mels::sqlog_info("system_logs", "Current Date-Time: {0}", current_dt);
    mels::sqlog_info("system_logs", "Time Span duration: {0}", span);
    mels::sqlog_info("system_logs", "UTF-8 String: {0}", utf8_msg);
    mels::sqlog_info("system_logs", "Mutex String: {0}", safe_msg);

    // -------------------------------------------------------------------------
    // 6. 다양한 표준 컨테이너 출력 테스트 (vector, map, set 등)
    // -------------------------------------------------------------------------
    std::vector<int> scores = { 95, 88, 76, 100 };
    std::map<std::string, int> user_scores = { {"Alice", 95}, {"Bob", 88} };
    std::set<std::string> unique_tags = { "error", "warning", "info" };

    mels::sqlog_warning("user_logs", "Scores vector: {0}", scores);
    mels::sqlog_warning("user_logs", "User scores map: {0}", user_scores);
    mels::sqlog_warning("user_logs", "Unique tags set: {0}", unique_tags);

    // -------------------------------------------------------------------------
    // 7. 멀티스레드 환경 테스트 (별도 함수 호출)
    // -------------------------------------------------------------------------
    std::cout << "\nStarting multi-threaded worker tasks...\n";
    std::vector<std::thread> workers;
    for (int i = 1; i <= 4; ++i) {
        // 인덱스 0번("system_logs")과 1번("user_logs")을 번갈아가며 타겟 테이블로 지정
        std::string target_table = (i % 2 == 0) ? "user_logs" : "system_logs";
        workers.emplace_back(worker_log_task, i, target_table);
    }

    // 모든 스레드 종료 대기
    for (auto& t : workers) {
        t.join();
    }
    std::cout << "All multi-threaded worker tasks completed.\n\n";

    // -------------------------------------------------------------------------
    // 8. 에러 처리 및 종료 테스트
    // -------------------------------------------------------------------------
    if (auto err = mels::sqlog_error("non_existent_table", "This should fail because table is not registered.")) {
        // 참고: sqlog_* 내부 함수가 optional 에러를 반환하게 처리하거나 직접 write_log를 부를 수 있습니다.
    }

    // 직접 write_log를 통한 에러 검증
    if (auto err = slog.write_log_error("non_existent_table", "Direct error test.")) {
        std::cerr << "Expected error caught: " << *err << "\n";
    }

    uint64_t count = 0;
    while(true) {
        count++;
        std::cout << "Running ... (" << count << ")\n";
        mels::sqlog_info("system_logs", "Heartbeat log entry number : {0}", count);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // 가장 최근 3개의 로그만 확인
    // SELECT*
    // FROM system_logs
    // ORDER BY id DESC
    // LIMIT 3;

    // 로거 종료
    slog.close();

    // 종료 후 로깅 시도
    if (auto err = slog.write_log_info(0, "Trying to log after close.")) {
        std::cout << "Expected close error caught: " << *err << "\n";
    }

    return 0;
}
