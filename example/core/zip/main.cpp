#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <filesystem>

#include "mino/core/zip/zip.hpp"

namespace mcz = mino::core::zip;
namespace fs = std::filesystem;

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

void test_uninitialized_state() {
    mcz::zip_archiver archiver;
    std::vector<uint8_t> dummy = { 1, 2, 3 };

    bool rejected_add_data = !archiver.add_file_data("test.txt", dummy);
    bool rejected_add_disk = !archiver.add_file_from_disk("non_existent.txt", "test.txt");
    bool rejected_finish = !archiver.finish();

    record_result("Uninitialized State Protection", rejected_add_data && rejected_add_disk && rejected_finish);
}

void test_init_validation() {
    // 빈 경로 거부
    mcz::zip_archiver archiver1;
    bool rejected_empty_path = !archiver1.init("");
    record_result("Init Empty Path Rejection", rejected_empty_path);

    // 디렉터리 경로 거부
    fs::create_directories("test_dir_target");
    mcz::zip_archiver archiver2;
    bool rejected_dir_target = !archiver2.init("test_dir_target");
    fs::remove("test_dir_target");
    record_result("Init Directory As Zip Path Rejection", rejected_dir_target);

    // overwrite = false 시 기존 파일 덮어쓰기 방어
    const std::string existing_file = "existing_sample.zip";
    {
        std::ofstream out(existing_file);
        out << "mock zip content";
    }
    mcz::zip_archiver archiver3;
    bool rejected_overwrite = !archiver3.init(existing_file, false);
    fs::remove(existing_file);
    record_result("Init Overwrite False Protection", rejected_overwrite);
}

void test_entry_path_security() {
    mcz::zip_archiver archiver;
    archiver.init("security_test.zip", true);
    std::vector<uint8_t> dummy = { 1, 2, 3 };

    bool rejected_empty_entry = !archiver.add_file_data("", dummy);
    bool rejected_abs_root = !archiver.add_file_data("/etc/passwd", dummy);
    bool rejected_drive_root = !archiver.add_file_data("C:/Windows/System32", dummy);
    bool rejected_zip_slip = !archiver.add_file_data("docs/../../secret.txt", dummy);

    archiver.finish();
    fs::remove("security_test.zip");

    record_result("ZIP Entry Empty Path Block", rejected_empty_entry);
    record_result("ZIP Entry Absolute Root Block", rejected_abs_root);
    record_result("ZIP Entry Drive Root Block", rejected_drive_root);
    record_result("ZIP Slip Traversal Block", rejected_zip_slip);
}

void test_disk_file_handling() {
    const std::string empty_file = "empty_file.dat";
    {
        std::ofstream out(empty_file, std::ios::binary);
    }
    fs::create_directories("dummy_dir");

    mcz::zip_archiver archiver;
    archiver.init("disk_edge_test.zip", true);

    bool rejected_non_existent = !archiver.add_file_from_disk("file_that_does_not_exist.bin", "file.bin");
    bool rejected_dir_as_file = !archiver.add_file_from_disk("dummy_dir", "dir_as_file");
    bool success_empty_file = archiver.add_file_from_disk(empty_file, "empty_file.dat");

    archiver.finish();

    fs::remove(empty_file);
    fs::remove("dummy_dir");
    fs::remove("disk_edge_test.zip");

    record_result("Disk Non-Existent File Rejection", rejected_non_existent);
    record_result("Disk Directory As File Rejection", rejected_dir_as_file);
    record_result("Disk Zero-Byte File Success", success_empty_file);
}

void test_compression_levels_and_encodings() {
    bool test_success = true;

    // UTF-8
    {
        mcz::zip_archiver archiver;
        test_success &= archiver.init("archive_utf8.zip", true, mcz::compression_level::default_level, mcz::path_encoding::utf8);

        std::vector<uint8_t> empty_buffer;
        std::string sample = "Compression level and encoding verification string payload.";
        std::vector<uint8_t> buffer(sample.begin(), sample.end());

        test_success &= archiver.add_file_data("empty.txt", empty_buffer);
        test_success &= archiver.add_file_data("store.txt", buffer, mcz::compression_level::store);
        test_success &= archiver.add_file_data("fastest.txt", buffer, mcz::compression_level::fastest);
        test_success &= archiver.add_file_data("fast.txt", buffer, mcz::compression_level::fast);
        test_success &= archiver.add_file_data("default.txt", buffer, mcz::compression_level::default_level);
        test_success &= archiver.add_file_data("maximum.txt", buffer, mcz::compression_level::maximum);

        test_success &= archiver.finish();
    }

    // CP949
    {
        mcz::zip_archiver archiver;
        test_success &= archiver.init("archive_cp949.zip", true, mcz::compression_level::default_level, mcz::path_encoding::cp949);

        std::string sample = "CP949 Encoding stream.";
        std::vector<uint8_t> buffer(sample.begin(), sample.end());
        test_success &= archiver.add_file_data("data/payload.dat", buffer);

        test_success &= archiver.finish();
    }

    fs::remove("archive_utf8.zip");
    fs::remove("archive_cp949.zip");

    record_result("All Compression Levels and Encodings", test_success);
}

void test_progress_callback_and_cancellation() {
    int call_count = 0;
    int last_seen_pct = -1;
    bool step_violation = false;

    {
        mcz::zip_archiver archiver;
        archiver.init("progress_step_test.zip", true);

        archiver.set_progress_callback([&](const mcz::progress_info& info) -> bool {
            call_count++;
            if (last_seen_pct != -1) {
                if (info.percentage - last_seen_pct < 20 && info.percentage != 100) {
                    step_violation = true;
                }
            }
            last_seen_pct = info.percentage;
            return true;
            }, 20);

        std::vector<uint8_t> large_buffer(2 * 1024 * 1024, 0xAB);
        archiver.add_file_data("large.bin", large_buffer, mcz::compression_level::fastest);
        archiver.finish();
    }
    fs::remove("progress_step_test.zip");

    bool step_valid = (!step_violation && call_count >= 2);
    record_result("Progress Callback Step Validation", step_valid);

    const std::string cancel_zip = "cancelled_archive.zip";
    mcz::zip_archiver archiver;
    archiver.init(cancel_zip, true);

    archiver.set_progress_callback([](const mcz::progress_info& info) -> bool {
        if (info.percentage >= 0) {
            return false;
        }
        return true;
        }, 1);

    std::vector<uint8_t> large_buffer(1024 * 1024, 0xFF);
    bool add_failed = !archiver.add_file_data("cancel.bin", large_buffer);
    bool file_cleaned_up = !fs::exists(cancel_zip);

    record_result("Progress Callback Cancellation & Rollback", add_failed && file_cleaned_up);
}

void test_post_finish_protection() {
    mcz::zip_archiver archiver;
    archiver.init("finish_guard_test.zip", true);

    std::vector<uint8_t> buffer = { 'O', 'K' };
    archiver.add_file_data("ok.txt", buffer);
    archiver.finish();

    bool rejected_post_add = !archiver.add_file_data("after_finish.txt", buffer);

    fs::remove("finish_guard_test.zip");
    record_result("Post-Finish Modification Guard", rejected_post_add);
}

void test_move_semantics() {
    bool move_success = true;

    mcz::zip_archiver source;
    move_success &= source.init("move_test.zip", true);

    std::vector<uint8_t> buffer = { 'M', 'O', 'V', 'E' };
    move_success &= source.add_file_data("first.txt", buffer);

    mcz::zip_archiver moved_target(std::move(source));
    move_success &= moved_target.add_file_data("second.txt", buffer);

    mcz::zip_archiver assigned_target;
    assigned_target = std::move(moved_target);
    move_success &= assigned_target.add_file_data("third.txt", buffer);

    move_success &= assigned_target.finish();

    bool file_exists = fs::exists("move_test.zip");
    fs::remove("move_test.zip");

    record_result("Move Constructor & Move Assignment", move_success && file_exists);
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << " Running Pure C++17 ZIP Archiver (Noexcept/Return-code) Tests" << std::endl;
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
