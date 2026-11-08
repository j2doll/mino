#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "mino/core/csv/csv.hpp"

TEST_CASE("csv_handler: read_csv_string 기본 및 특수문자 파싱 테스트", "[csv][handler]") {
    using mino::core::csv::csv_handler;
    using std::string;
    using std::vector;

    csv_handler handler;

    SECTION("표준 쉼표 구분 및 CRLF/LF 개행 파싱") {
        string content = "col1,col2,col3\r\nval1,val2,val3\nval4,val5,val6";
        vector<vector<string>> result = handler.read_csv_string(content);

        REQUIRE(result.size() == 3);
        CHECK(result[0] == vector<string>{ "col1", "col2", "col3" });
        CHECK(result[1] == vector<string>{ "val1", "val2", "val3" });
        CHECK(result[2] == vector<string>{ "val4", "val5", "val6" });
    }

    SECTION("따옴표 내부 쉼표 및 개행 처리 (RFC 4180)") {
        string content = "\"line 1\nline 2\",\"comma,inside\",simple\r\n\"escaped \"\"quote\"\"\",end";
        vector<vector<string>> result = handler.read_csv_string(content);

        REQUIRE(result.size() == 2);
        CHECK(result[0] == vector<string>{"line 1\nline 2", "comma,inside", "simple"});
        CHECK(result[1] == vector<string>{"escaped \"quote\"", "end"});
    }

    SECTION("UTF-8 BOM 헤더 제거 확인") {
        string content = "\xEF\xBB\xBFid,name\r\n1,mino";
        vector<vector<string>> result = handler.read_csv_string(content);

        REQUIRE(result.size() == 2);
        CHECK(result[0][0] == "id");
        CHECK(result[0][1] == "name");
        CHECK(result[1][0] == "1");
        CHECK(result[1][1] == "mino");
    }

    SECTION("trim_empty_tails 옵션 동작 확인") {
        string content = "a,b,,\r\n1,2,3,";

        vector<vector<string>> trimmed = handler.read_csv_string(content, true);
        REQUIRE(trimmed.size() == 2);
        CHECK(trimmed[0] == vector<string>{"a", "b"});
        CHECK(trimmed[1] == vector<string>{"1", "2", "3"});

        vector<vector<string>> untrimmed = handler.read_csv_string(content, false);
        REQUIRE(untrimmed.size() == 2);
        CHECK(untrimmed[0] == vector<string>{"a", "b", "", ""});
        CHECK(untrimmed[1] == vector<string>{"1", "2", "3", ""});
    }
}

TEST_CASE("csv_handler: write_csv 파일 쓰기 및 읽기 라운드트립", "[csv][handler]") {
    namespace fs = std::filesystem;
    using mino::core::csv::csv_handler;
    using mino::core::csv::line_break_type;
    using std::string;
    using std::vector;

    csv_handler handler;
    const string test_file = "temp_test_handler.csv";

    // 이전 잔여 파일 정리
    if (fs::exists(test_file)) {
        fs::remove(test_file);
    }

    vector<vector<string>> data = {
        {"ID", "Name", "Desc"},
        {"1", "Mino", "Line1\nLine2"},
        {"2", "Quote\"Test", "Val,with,comma"}
    };

    SECTION("write_csv 기본 옵션(CRLF) 및 read_csv 라운드트립") {
        bool write_ok = handler.write_csv(test_file, data, line_break_type::crlf, true);
        REQUIRE(write_ok);
        REQUIRE(fs::exists(test_file));

        vector<vector<string>> loaded = handler.read_csv(test_file, false);
        REQUIRE(loaded.size() == 3);
        CHECK(loaded[0] == data[0]);
        CHECK(loaded[1] == data[1]);
        CHECK(loaded[2] == data[2]);
    }

    SECTION("존재하지 않는 파일 읽기 시 빈 벡터 반환") {
        vector<vector<string>> empty_result = handler.read_csv("non_existent_file_xyz.csv");
        CHECK(empty_result.empty());
    }

    if (fs::exists(test_file)) {
        fs::remove(test_file);
    }
}

TEST_CASE("excel_csv_writer: 기본 헤더 및 데이터 쓰기 동작", "[csv][excel_writer]") {
    namespace fs = std::filesystem;
    using mino::core::csv::excel_csv_writer;
    using std::string;
    using std::vector;

    const string test_file = "temp_test_excel_writer.csv";
    if (fs::exists(test_file)) {
        fs::remove(test_file);
    }

    {
        excel_csv_writer writer;
        writer.set_file_path(test_file);
        writer.set_charset("UTF-8");

        SECTION("용량 문자열 파싱 검증") {
            CHECK(writer.set_max_file_size("10MB"));
            CHECK(writer.set_max_file_size("512 KB"));
            CHECK_FALSE(writer.set_max_file_size("invalid_size"));
        }

        SECTION("헤더 쓰기 및 열 개수 유효성 검사") {
            vector<string> headers = { "ID", "Name", "Code" };
            REQUIRE(writer.write_header(headers));
            CHECK(writer.get_header_count() == 3);

            // 중복 헤더 쓰기 시도 방지
            CHECK_FALSE(writer.write_header(headers));

            // 열 개수 불일치 행 삽입 방지
            vector<string> invalid_row = { "1", "OnlyTwo" };
            CHECK_FALSE(writer.write_row(invalid_row));

            // 유효한 일반 행 삽입
            vector<string> valid_row = { "1", "Mino", "A01" };
            CHECK(writer.write_row(valid_row));

            // 강제 텍스트 형식 (="value") 행 삽입
            vector<string> text_row = { "002", "TextNum", "01234" };
            CHECK(writer.write_row_pure_text(text_row));
        }
    }

    if (fs::exists(test_file)) {
        fs::remove(test_file);
    }
}
