#include <iostream>
#include <cstdio>

#include "mino/core/string/string.hpp"
#include "mino/core/scope/scope.hpp"

void perform_transaction() {
    namespace mcstr = mino::core::string;
    namespace mscope = mino::core::scope;

    auto tcenc = mcstr::to_console_encoding;

    int transaction_id = 42;
    bool is_success = false;
    FILE* log_file = std::fopen("transaction.log", "w");
    std::cout << tcenc("[1] 스코프 진입: 로그 파일 열림 (ID: ") << transaction_id << ")\n";

    // 스코프 종료 시, 로그 기록 및 파일 닫기
    mino::core::scope::scope_exit on_exit([&transaction_id, &is_success, log_file]() {
        auto tcenc = mino::core::string::to_console_encoding;
        if (log_file) {
            std::fprintf(log_file,
                "Transaction ID: %d, Result: %s\n",
                transaction_id, is_success ? "SUCCESS" : "FAIL");
            std::fclose(log_file);
            std::cout << tcenc("[on_exit][1] 파일 쓰기 및 닫힘 (ID: ") << transaction_id << ")\n";
        }

        std::cout << tcenc("[on_exit][2] 스코프 탈출: 로그 기록 완료 (ID: ") << transaction_id << ")\n";
    });

    // 코드를 단축하여 사용하는 예제
    auto on_exit2 = mscope::make_scope_exit([&]() {
        auto tcenc = mino::core::string::to_console_encoding;
         std::cout << tcenc("[on_exit2] 스코프 탈출\n");
    });
    // NOTE: 스코프가 종료될 때, on_exit2 => on_exit 순으로 호출됨.

    std::cout << tcenc("[2] 작업 진행 중...\n");
    transaction_id = 99; // 최신 값 반영 확인용
    is_success = true;
    std::cout << tcenc("[3] 스코프 탈출: 작업 완료 (ID: ") << transaction_id << ")\n";
}

int main() {
    perform_transaction();
    return 0;
}


