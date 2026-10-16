#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <memory>

#include "mino/core/resilience/resilience.hpp"

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

//-----------------------------------------------------------------------------
// paginated_list는 메모리에 있는 컬렉션(컨테이너) 데이터를 복사(Copy) 없이
// 가볍고 안전하게 페이지 단위로 나누어 조회(Pagination)할 때 사용.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 1. 페이지네이션 (paginated_list) 테스트
//-----------------------------------------------------------------------------
void test_paginated_list() {
    namespace mcs = mino::core::resilience;
    auto to_enc = mino::core::string::to_console_encoding;

    std::cout << to_enc("\n========================================\n");
    std::cout << to_enc(" [1] Paginated List Test\n");
    std::cout << to_enc("========================================\n");

    std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    int page_size = 5;

    auto page1 = mcs::to_paginated_list(numbers, 1, page_size);
    // 1 개의 페이지를 조회함.
    //  - 페이지 크기가 5 이므로, (int page_size = 5;)
    //  - 1페이지에는 1 ~ 5,
    //  - 2페이지에는 6 ~ 10,
    //  - 3페이지에는 11 ~ 13 이 포함됨.

    // 페이지네이션 정보 출력
    std::cout << to_enc("Total items : ") << page1.total_items << "\n"; // 13
    std::cout << to_enc("Total pages : ") << page1.total_pages << "\n"; // 3        

    // 1페이지 출력
    std::cout << to_enc("Page 1 data : ");
    for (int val : page1.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    // 출력값: 1, 2, 3, 4, 5

    // 3페이지(마지막 페이지) 출력 
    auto page3 = mcs::to_paginated_list(numbers, 3, page_size);
    std::cout << to_enc("Page 3 data : ");
    for (int val : page3.get_page_view()) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    // 출력값: 11, 12, 13

//-----------------------------------------------------------------------------
// * 복사 없는 지연 평가(Zero - Copy & Lazy Evaluation)
//   - 원본 컨테이너를 복사하지 않고, 원본의 참조만 들고 있습니다.
//   - 실제 요소에 접근할 때도 
//     해당 페이지 구간의 시작(`begin`)과 끝(`end`) 반복자만 감싸는
//     `page_view` 구조체를 반환하므로 메모리 할당 및 복사 오버헤드가 전혀 없습니다.
//-----------------------------------------------------------------------------

    // 범위(1~3)를 벗어난 페이지(99) 요청 시, 방어 코드 동작 확인
    auto page_overflow = mcs::to_paginated_list(numbers, 99, page_size);
    std::cout
        << to_enc("Page 99 요청 -> 실제 적용 페이지: ")
        << page_overflow.current_page << "\n";
    // Page 99 요청 -> 실제 적용 페이지: 3
    //  NOTE: 요청 페이지가 총 페이지 수를 초과했으므로,
    //   실제 적용 페이지는 마지막 페이지(3)로 조정됨.

// -----------------------------------------------------------------------------
// * 안전한 경계값 방어 코드(Defensive Programming)
// - page_size가 0 이하일 경우, 1로 보정하여 Divide-by-Zero 오류를 방지.
// - page_number가 1보다 작으면 1로 보정,
//   total_pages보다 크면 마지막 페이지 번호로 자동 보정합니다.
// - 컨테이너 크기보다 더 큰 범위를 건너뛰거나
//   접근하지 않도록 반복자 이동 범위(move_count, skip_count)를 안전하게 클램핑합니다.
//-----------------------------------------------------------------------------
}


//-----------------------------------------------------------------------------
// 긴 자료 구조의 순회 및 출력 예제
// 
// struct User {
//     int id;
//     std::string name;
// };
// 
// void print_user_page(const std::vector<User>& all_users, int page, int page_size) {
//     // 1. 페이지네이션 객체 생성
//     auto paged = resilience::to_paginated_list(all_users, page, page_size);
// 
//     std::cout << "=== [" << paged.current_page << " / " << paged.total_pages
//         << " 페이지] (전체 " << paged.total_items << "개 중) ===" << std::endl;
// 
//     // 2. 복사 없이 현재 페이지 항목만 순회
//     for (const auto& user : paged.get_page_view()) {
//         std::cout << "ID: " << user.id << ", Name: " << user.name << "\n";
//     }
// }
//-----------------------------------------------------------------------------
// void process_in_batches(const std::vector<std::string>& large_dataset, int batch_size) {
//     // 첫 페이지로 전체 페이지 수 계산
//     auto paged = resilience::to_paginated_list(large_dataset, 1, batch_size);
//     int total_pages = paged.total_pages;
// 
//     for (int page = 1; page <= total_pages; ++page) {
//         auto batch = resilience::to_paginated_list(large_dataset, page,  batch_size);
// 
//         std::cout << "Batch " << page << " 처리 중...\n";
//         for (const auto& item : batch.get_page_view()) {
//             // 배치 단위 작업 수행 (예: DB 벌크 삽입, 네트워크 전송 등)
//         }
//     }
// }
//-----------------------------------------------------------------------------
