#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <filesystem>

#include "mino/core/csv/csv.hpp"
#include "mino/core/csv/excel_csv_writer.hpp"

int main() {
    std::cout << "=== CSV & Excel Writer Public Interface Test Start ===" << std::endl;

    // --------------------------------------------------
    // 1. mino::core::csv::csv_handler 퍼블릭 멤버 테스트
    // --------------------------------------------------
    {
        std::string temp_file_path = "test_csv_handler_output.csv";
        {
            std::cout << "\n[1] Testing csv_handler..." << std::endl;

            // 생성자 테스트 (기본 및 커스텀 로거 지정)
            mino::core::csv::csv_handler handler(nullptr);
            handler.set_logger(nullptr);

            // 1-1. read_csv_string 테스트
            std::string sample_csv_data =
                "Name,Age,City\n"
                "\"Hong, Gildong\",30,\"Seoul\"\n"
                "\"Kim\",25,\"Busan\"";

            auto parsed_rows = handler.read_csv_string(sample_csv_data, true);

            assert(parsed_rows.size() == 3);
            assert(parsed_rows[0][0] == "Name");
            assert(parsed_rows[1][0] == "Hong, Gildong");
            assert(parsed_rows[2][2] == "Busan");
            std::cout << "  - read_csv_string: PASSED" << std::endl;

            // 1-2. write_csv 테스트
            std::vector<std::vector<std::string>> export_data = {
                {"ID", "Content", "Status"},
                {"1", "Line 1\nLine 2", "OK"},
                {"2", "Quoted \"Value\"", "Pending"}
            };

            bool write_ok = handler.write_csv(
                temp_file_path,
                export_data,
                mino::core::csv::line_break_type::crlf,
                true
            );
            assert(write_ok == true);
            std::cout << "  - write_csv: PASSED" << std::endl;

            // 1-3. read_csv 테스트 (작성한 파일 읽기)
            auto read_file_rows = handler.read_csv(temp_file_path, true);
            assert(read_file_rows.size() == 3);
            assert(read_file_rows[1][1] == "Line 1\nLine 2");
            assert(read_file_rows[2][1] == "Quoted \"Value\"");
            std::cout << "  - read_csv: PASSED" << std::endl;

        }
        // 임시 파일 삭제
        std::error_code ec;
        std::filesystem::remove(temp_file_path, ec);
        if (ec) {
            std::cout << "  - Failed to delete temporary Excel file: "
                << ec.message() << std::endl;
        }
        else {
            std::cout << "  - Temporary Excel file deleted." << std::endl;
        }
    }

    // --------------------------------------------------
    // 2. mino::core::csv::excel_csv_writer 퍼블릭 멤버 테스트
    // --------------------------------------------------
    {
        std::string temp_excel_path = "test_excel_writer_output.csv";
        {
            std::cout << "\n[2] Testing excel_csv_writer..." << std::endl;

            mino::core::csv::excel_csv_writer writer;

            // 2-1. Setter 멤버 함수 테스트
            writer.set_file_path(temp_excel_path);
            writer.set_max_file_size(2 * 1024 * 1024); // 2MB

            bool size_str_parsed = writer.set_max_file_size("10MB");
            assert(size_str_parsed == true);

            writer.set_max_files(5);
            writer.set_charset("UTF-8");
            std::cout << "  - Setters (file_path, size, files, charset): PASSED" << std::endl;

            // 2-2. initialize 테스트
            bool init_ok = writer.initialize();
            assert(init_ok == true);
            std::cout << "  - initialize: PASSED" << std::endl;

            // 2-3. write_header 및 get_header_count 테스트
            std::vector<std::string> headers = { "Code", "Name", "Score" };
            bool header_ok = writer.write_header(headers);
            assert(header_ok == true);
            assert(writer.get_header_count() == 3);
            std::cout << "  - write_header & get_header_count: PASSED" << std::endl;

            // 2-4. write_row 테스트
            std::vector<std::string> row1 = { "A001", "Alice", "95.5" };
            bool row1_ok = writer.write_row(row1);
            assert(row1_ok == true);
            std::cout << "  - write_row: PASSED" << std::endl;

            // 2-5. write_row_pure_text 테스트 (텍스트 강제 ="" 형식)
            std::vector<std::string> row2 = { "00123", "Bob", "80.0" };
            bool row2_ok = writer.write_row_pure_text(row2);
            assert(row2_ok == true);
            std::cout << "  - write_row_pure_text: PASSED" << std::endl;

            // 2-6. 헤더 컬럼 수 불일치 시 예외/실패 검증
            std::vector<std::string> invalid_row = { "A002", "Charlie" }; // 2개 (헤더는 3개)
            bool invalid_row_ok = writer.write_row(invalid_row);
            assert(invalid_row_ok == false);
            std::cout << "  - Header count mismatch validation: PASSED" << std::endl;

        }
        // 소멸자 정상 작동 및 파일 리소스 해제를 위해 객체 스코프 정리 후 삭제 가능
        std::error_code ec;
        std::filesystem::remove(temp_excel_path, ec);
        if (ec) {
            std::cout << "  - Failed to delete temporary Excel file: "
                << ec.message() << std::endl;
        } else {
            std::cout << "  - Temporary Excel file deleted." << std::endl;
        }
    }

    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
    return 0;
}
