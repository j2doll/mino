#include <iostream>
#include <string>
#include <optional>
#include <iomanip>

#include "mino/core/config/config.hpp"
#include "mino/core/string/to_console_encoding.hpp"

int main() {
    auto& config = mino::core::config::config_manager::get_instance(); // 싱글톤 인스턴스 가져오기
    auto to_console_encoding = mino::core::string::to_console_encoding; 

    // 기본 설정 파일 경로 지정
    config.set_config_file("app_config.conf");

    // config 파일 로딩
    if (!config.load()) {
        std::cerr << "Failed to load configuration file." << std::endl;
        return 1;
    }

    // 모든 항목 출력 (데이터 라인 및 주석/빈줄 포함)
    std::cout << "---------- app_config.conf contents ----------" << std::endl;
    auto all = config.get_all();
    for (const auto& line : all) {
        if (line.is_data)
        {
            // key = value 형식의 자료가 있는 경우
            auto key = to_console_encoding(line.key);
            auto value = to_console_encoding(line.value);
            std::cout << "[DATA] " << key << " = " << value << std::endl;
        }
        else
        {
            auto raw_text = to_console_encoding(line.raw_text);
            std::cout << "[RAW]  " << raw_text << std::endl;
        }
    }
    std::cout << "----------------------------------------------" << std::endl;

    // 기존 테스트 코드: 개인 항목 읽기 예시
    auto app_name_opt = config.get_string("app.name");
    std::string app_name = app_name_opt ? *app_name_opt : std::string("Default App");
    std::cout << "app.name: " << to_console_encoding(app_name) << std::endl;

    auto int_opt = config.get_int("test.int");
    if (int_opt)
    {
        std::cout << "test.int: " << *int_opt << std::endl;
    }
    else
    {
        std::cout << "test.int: <missing or parse error>" << std::endl;
    }

    auto bool_opt = config.get_bool("test.bool");
    if (bool_opt)
    {
        std::cout << std::boolalpha << "test.bool: " << *bool_opt << std::noboolalpha << std::endl;
    }
    else
    {
        std::cout << "test.bool: <missing or parse error>" << std::endl;
    }

    auto dbl_opt = config.get_double("test.double");
    if (dbl_opt)
    {
        std::cout << "test.double: " << std::fixed << std::setprecision(6) << *dbl_opt << std::endl;
    }
    else
    {
        std::cout << "test.double: <missing or parse error>" << std::endl;
    }

    return 0;
}
