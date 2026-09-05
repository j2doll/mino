#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <filesystem>

#include "mino/core/zip/zip.hpp"

namespace mcz = mino::core::zip;
namespace fs = std::filesystem;

// 테스트 프레임워크 헬퍼 매크로
int g_tests_passed = 0;
int g_tests_failed = 0;

void record_result(const std::string& test_name, bool success) {
    if (success) {
        std::cout << "[PASS] " << test_name << std::endl;
        g_tests_passed++;
    }
    else {
        std::cerr << "[FAIL] " << test_name << std::endl;
        g_tests_failed++;
    }
}

void remove_by_extension(const fs::path& dir_path, const std::string& extension) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (ec) {
                std::cerr << "Failed to remove: " << entry.path() << " (" << ec.message() << ")\n";
            }
        }
    }
}

// -------------------------------------------------------------
// Test 1: 미초기화 상태 방어 테스트
// -------------------------------------------------------------
void test_uninitialized_state() {
    mcz::zip_archiver archiver;
    bool caught_add_data = false;
    bool caught_add_disk = false;
    bool caught_finish = false;

    try {
        std::vector<uint8_t> dummy = { 1, 2, 3 };
        archiver.add_file_data("test.txt", dummy);
    }
    catch (const std::logic_error&) {
        caught_add_data = true;
    }

    try {
        archiver.add_file_from_disk("non_existent.txt", "test.txt");
    }
    catch (const std::logic_error&) {
        caught_add_disk = true;
    }

    try {
        archiver.finish();
    }
    catch (const std::logic_error&) {
        caught_finish = true;
    }

    record_result("Uninitialized State Protection", caught_add_data && caught_add_disk && caught_finish);
}

// -------------------------------------------------------------
// Test 2: init() 인자 및 경로 보안 방어 테스트
// -------------------------------------------------------------
void test_init_validation() {
    // 2.1 빈 경로 지정 차단
    bool caught_empty_path = false;
    try {
        mcz::zip_archiver archiver;
        archiver.init("");
    }
    catch (const std::invalid_argument&) {
        caught_empty_path = true;
    }
    record_result("Init Empty Path Rejection", caught_empty_path);

    // 2.2 디렉터리 경로를 zip 파일명으로 지정했을 때 차단
    fs::create_directories("test_dir_target");
    bool caught_dir_target = false;
    try {
        mcz::zip_archiver archiver;
        archiver.init("test_dir_target");
    }
    catch (const std::runtime_error&) {
        caught_dir_target = true;
    }
    fs::remove("test_dir_target");
    record_result("Init Directory As Zip Path Rejection", caught_dir_target);

    // 2.3 overwrite = false 상태에서 기존 파일 덮어쓰기 차단
    const std::string existing_file = "existing_sample.zip";
    {
        std::ofstream out(existing_file);
        out << "mock zip content";
    }
    bool caught_overwrite_block = false;
    try {
        mcz::zip_archiver archiver;
        archiver.init(existing_file, /*overwrite=*/false);
    }
    catch (const std::runtime_error&) {
        caught_overwrite_block = true;
    }
    fs::remove(existing_file);
    record_result("Init Overwrite False Protection", caught_overwrite_block);
}

// -------------------------------------------------------------
// Test 3: ZIP 엔트리 내부 경로 검증 및 보안 (Zip Slip 방어)
// -------------------------------------------------------------
void test_entry_path_security() {
    mcz::zip_archiver archiver;
    archiver.init("security_test.zip", /*overwrite=*/true);
    std::vector<uint8_t> dummy = { 1, 2, 3 };

    // 3.1 빈 엔트리 경로 차단
    bool caught_empty_entry = false;
    try {
        archiver.add_file_data("", dummy);
    }
    catch (const std::invalid_argument&) {
        caught_empty_entry = true;
    }

    // 3.2 절대 경로 차단 (/etc/passwd)
    bool caught_abs_root = false;
    try {
        archiver.add_file_data("/etc/passwd", dummy);
    }
    catch (const std::invalid_argument&) {
        caught_abs_root = true;
    }

    // 3.3 드라이브 루트 차단 (C:/Windows)
    bool caught_drive_root = false;
    try {
        archiver.add_file_data("C:/Windows/System32", dummy);
    }
    catch (const std::invalid_argument&) {
        caught_drive_root = true;
    }

    // 3.4 Zip Slip 탐색 (../ 상위 디렉터리 접근) 차단
    bool caught_zip_slip = false;
    try {
        archiver.add_file_data("docs/../../secret.txt", dummy);
    }
    catch (const std::invalid_argument&) {
        caught_zip_slip = true;
    }

    archiver.finish();
    fs::remove("security_test.zip");

    record_result("ZIP Entry Empty Path Block", caught_empty_entry);
    record_result("ZIP Entry Absolute Root Block", caught_abs_root);
    record_result("ZIP Entry Drive Root Block", caught_drive_root);
    record_result("ZIP Slip Traversal Block", caught_zip_slip);
}

// -------------------------------------------------------------
// Test 4: 디스크 파일 핸들링 및 엣지 케이스 (0바이트, 존재 여부)
// -------------------------------------------------------------
void test_disk_file_handling() {
    const std::string empty_file = "empty_file.dat";
    {
        std::ofstream out(empty_file, std::ios::binary); // 0바이트 파일 생성
    }

    fs::create_directories("dummy_dir");

    mcz::zip_archiver archiver;
    archiver.init("disk_edge_test.zip", /*overwrite=*/true);

    // 4.1 존재하지 않는 파일 읽기 시도 차단
    bool caught_non_existent = false;
    try {
        archiver.add_file_from_disk("file_that_does_not_exist.bin", "file.bin");
    }
    catch (const std::runtime_error&) {
        caught_non_existent = true;
    }

    // 4.2 디렉터리를 일반 파일로 추가 시도 차단
    bool caught_dir_as_file = false;
    try {
        archiver.add_file_from_disk("dummy_dir", "dir_as_file");
    }
    catch (const std::runtime_error&) {
        caught_dir_as_file = true;
    }

    // 4.3 0바이트 파일 추가 정상 처리
    bool success_empty_file = false;
    try {
        archiver.add_file_from_disk(empty_file, "empty_file.dat");
        success_empty_file = true;
    }
    catch (...) {
        success_empty_file = false;
    }

    archiver.finish();

    fs::remove(empty_file);
    fs::remove("dummy_dir");
    fs::remove("disk_edge_test.zip");

    record_result("Disk Non-Existent File Rejection", caught_non_existent);
    record_result("Disk Directory As File Rejection", caught_dir_as_file);
    record_result("Disk Zero-Byte File Success", success_empty_file);
}

// -------------------------------------------------------------
// Test 5: 모든 압축 레벨 및 인코딩 복합 압축 검증
// -------------------------------------------------------------
void test_compression_levels_and_encodings() {
    bool test_success = true;

    try {
        // 5.1 UTF-8 인코딩 및 전 레벨 테스트
        {
            mcz::zip_archiver archiver;
            archiver.init("archive_utf8.zip", /*overwrite=*/true, mcz::compression_level::default_level, mcz::path_encoding::utf8);

            std::vector<uint8_t> empty_buffer;
            std::string sample = "Compression level and encoding verification string payload.";
            std::vector<uint8_t> buffer(sample.begin(), sample.end());

            archiver.add_file_data("empty.txt", empty_buffer);
            archiver.add_file_data("store.txt", buffer, mcz::compression_level::store);
            archiver.add_file_data("fastest.txt", buffer, mcz::compression_level::fastest);
            archiver.add_file_data("fast.txt", buffer, mcz::compression_level::fast);
            archiver.add_file_data("default.txt", buffer, mcz::compression_level::default_level);
            archiver.add_file_data("maximum.txt", buffer, mcz::compression_level::maximum);

            archiver.finish();
        }

        // 5.2 CP949 인코딩 테스트
        {
            mcz::zip_archiver archiver;
            archiver.init("archive_cp949.zip", /*overwrite=*/true, mcz::compression_level::default_level, mcz::path_encoding::cp949);

            std::string sample = "CP949 Encoding stream.";
            std::vector<uint8_t> buffer(sample.begin(), sample.end());
            archiver.add_file_data("data/payload.dat", buffer);

            archiver.finish();
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected exception in level/encoding test: " << e.what() << std::endl;
        test_success = false;
    }

    fs::remove("archive_utf8.zip");
    fs::remove("archive_cp949.zip");

    record_result("All Compression Levels and Encodings", test_success);
}

// -------------------------------------------------------------
// Test 6: 진행률 콜백 호출 주기 및 사용자 취소(Rollback) 검증
// -------------------------------------------------------------
void test_progress_callback_and_cancellation() {
    // 6.1 Step 간격(20%) 준수 검증
    int call_count = 0;
    int last_seen_pct = -1;
    bool step_violation = false;

    {
        mcz::zip_archiver archiver;
        archiver.init("progress_step_test.zip", /*overwrite=*/true);

        archiver.set_progress_callback([&](const mcz::progress_info& info) -> bool {
            call_count++;
            if (last_seen_pct != -1) {
                if (info.percentage - last_seen_pct < 20 && info.percentage != 100) {
                    step_violation = true;
                }
            }
            last_seen_pct = info.percentage;
            return true;
            }, /*percent_step=*/20);

        std::vector<uint8_t> large_buffer(2 * 1024 * 1024, 0xAB);
        archiver.add_file_data("large.bin", large_buffer, mcz::compression_level::fastest);
        archiver.finish();
    }
    fs::remove("progress_step_test.zip");

    bool step_valid = (!step_violation && call_count >= 2);
    record_result("Progress Callback Step Validation", step_valid);

    // 6.2 콜백에서 false 반환 시 즉시 취소 및 파일 삭제(Rollback) 검증
    const std::string cancel_zip = "cancelled_archive.zip";
    bool caught_cancel_exception = false;

    try {
        mcz::zip_archiver archiver;
        archiver.init(cancel_zip, /*overwrite=*/true);

        archiver.set_progress_callback([](const mcz::progress_info& info) -> bool {
            if (info.percentage >= 0) {
                return false; // 첫 진행 시 즉시 취소
            }
            return true;
            }, 1);

        std::vector<uint8_t> large_buffer(1024 * 1024, 0xFF);
        archiver.add_file_data("cancel.bin", large_buffer);
        archiver.finish();
    }
    catch (const std::runtime_error&) {
        caught_cancel_exception = true;
    }

    // 소멸자에 의해 불완전 파일이 제거되었는지 확인
    bool file_cleaned_up = !fs::exists(cancel_zip);

    record_result("Progress Callback Cancellation & Rollback", caught_cancel_exception && file_cleaned_up);
}

// -------------------------------------------------------------
// Test 7: finish() 호출 이후 수정 차단 검증
// -------------------------------------------------------------
void test_post_finish_protection() {
    mcz::zip_archiver archiver;
    archiver.init("finish_guard_test.zip", /*overwrite=*/true);

    std::vector<uint8_t> buffer = { 'O', 'K' };
    archiver.add_file_data("ok.txt", buffer);
    archiver.finish();

    bool caught_post_add = false;
    try {
        archiver.add_file_data("after_finish.txt", buffer);
    }
    catch (const std::logic_error&) {
        caught_post_add = true;
    }

    fs::remove("finish_guard_test.zip");
    record_result("Post-Finish Modification Guard", caught_post_add);
}

// -------------------------------------------------------------
// Test 8: 이동 생성 및 이동 대입 (Move Semantics) 검증
// -------------------------------------------------------------
void test_move_semantics() {
    bool move_success = true;

    try {
        mcz::zip_archiver source;
        source.init("move_test.zip", /*overwrite=*/true);

        std::vector<uint8_t> buffer = { 'M', 'O', 'V', 'E' };
        source.add_file_data("first.txt", buffer);

        // 8.1 이동 생성자 검증
        mcz::zip_archiver moved_target(std::move(source));
        moved_target.add_file_data("second.txt", buffer);

        // 8.2 이동 대입 연산자 검증
        mcz::zip_archiver assigned_target;
        assigned_target = std::move(moved_target);
        assigned_target.add_file_data("third.txt", buffer);

        assigned_target.finish();

    }
    catch (const std::exception& e) {
        std::cerr << "Move semantics failure: " << e.what() << std::endl;
        move_success = false;
    }

    bool file_exists = fs::exists("move_test.zip");
    fs::remove("move_test.zip");

    record_result("Move Constructor & Move Assignment", move_success && file_exists);
}

// -------------------------------------------------------------
// 메인 테스트 러너
// -------------------------------------------------------------
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << " Running Pure C++17 ZIP Archiver Comprehensive Tests" << std::endl;
    std::cout << "==================================================" << std::endl;

    std::cout << "Current Path: " << fs::current_path().string() << std::endl;
    remove_by_extension(fs::current_path(), ".zip");

    test_uninitialized_state();
    test_init_validation();
    test_entry_path_security();
    test_disk_file_handling();
    test_compression_levels_and_encodings();
    test_progress_callback_and_cancellation();
    test_post_finish_protection();
    test_move_semantics();

    std::cout << "==================================================" << std::endl;
    std::cout << " Test Summary: " << g_tests_passed << " Passed, "
        << g_tests_failed << " Failed." << std::endl;
    std::cout << "==================================================" << std::endl;

    return (g_tests_failed == 0) ? 0 : 1;
}
