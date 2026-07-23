#pragma once

#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

namespace mino::core::resilience {

    // 특정 페이지의 데이터 범위를 가리키는 뷰 구조체 (복사 발생 없음)
    template <typename Iterator>
    struct page_view {
        Iterator begin_iterator;
        Iterator end_iterator;

        Iterator begin() const { return begin_iterator; }
        Iterator end() const { return end_iterator; }
    };

    // 페이지네이션 결과 정보를 담는 구조체
    template <typename Container>
    struct paginated_result {
        using iterator_type = typename Container::const_iterator;

        int current_page;
        int page_size;
        int total_items;
        int total_pages;

        const Container& source_data; // 원본 데이터 참조

        // 실제 데이터 접근을 위한 반복자 뷰 반환 (지연 평가)
        page_view<iterator_type> get_page_view() const {
            if (source_data.empty()) {
                return { source_data.begin(), source_data.end() };
            }

            int skip_count = (current_page - 1) * page_size;

            // 시작 위치 계산 (데이터 전체 개수를 넘지 않도록 제한)
            auto begin_it = source_data.begin();
            std::advance(begin_it, std::min<int>(skip_count, total_items));

            // 끝 위치 계산 (시작 위치로부터 page_size만큼 이동하되, 컨테이너 끝을 넘지 않도록 제한)
            auto end_it = begin_it;

            // std::distance의 반환형과 page_size의 타입이 다르므로 narrowing 경고를 피하기 위해
            // 반복자 차이 타입을 사용하여 안전하게 계산한다.
            using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
            difference_type available = std::distance(begin_it, source_data.end());
            difference_type move_count = std::min<difference_type>(static_cast<difference_type>(page_size), available);

            std::advance(end_it, move_count);

            return { begin_it, end_it };
        }
    };

    // C#의 확장 메서드 역할을 하는 페이지네이션 팩토리 함수
    template <typename Container>
    auto to_paginated_list(const Container& container, int page_number, int page_size) {
        // 1. 페이지당 아이템 수 방어 코드 (0으로 나누기 오류 및 음수 방지)
        if (page_size < 1) page_size = 1;

        int total_items = static_cast<int>(container.size());

        // 2. 총 페이지 수 계산
        int total_pages = (total_items + page_size - 1) / page_size;
        if (total_pages == 0) total_pages = 1;

        // 3. 요청 페이지 번호 방어 코드
        if (page_number < 1) page_number = 1;
        if (page_number > total_pages) page_number = total_pages;

        return paginated_result<Container>{
            page_number,
                page_size,
                total_items,
                total_pages,
                container
        };
    }

} 