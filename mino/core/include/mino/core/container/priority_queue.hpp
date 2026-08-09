#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <optional>

// ========================================================================
// 항목            우선순위 큐          이진 힙              피보나치 힙
//                priority_queue        pairing_heap        fibonacci_heap   
// ------------------------------------------------------------------------
// 개념 분류       추상 자료형(ADT)     자료 구조(구현체)    자료 구조(구현체)
// 기능의 정의          실제 구현체          실제 구현체
// 
// 내부 형태       정의되지 않음        완전 이진 트리       원형 이중 연결
// (배열,         (주로 배열로 구현)   리스트 트리 집합
// 리스트 등)
// 
// 삽입(Push)      구현에 따라 다름     O(log N)             O(1)
// 
// 삭제(Pop)       구현에 따라 다름     O(log N)             O(log N)[상환]
// 
// 값 감소         지원 안 하거나 느림  O(log N)             O(1)[상환]
// 
// 병합(Merge)     지원하지 않음        O(N)                 O(1)
// 
// 메모리 소모     없음(구현체별 다름)  매우 적음            매우 큼
// (인덱스 기반   (다중 포인터 사용)
//  탐색)
// 
// 실무 추천       인터페이스 명세      대부분의 상황        특수 알고리즘
//                 설계 시 사용         (캐시 효율 우수)     (다익스트라 등)
// ========================================================================

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>, typename Container = std::vector<T>>
    class  priority_queue {
    public:
        using value_type = T;
        using container_type = Container;
        using compare_type = Compare;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;

        // 1. 기본 생성자
        priority_queue() : c_(), comp_() {}

        // 2. 외부에서 비교자(람다, 함수 객체 등)를 받아오는 생성자 (★필수 추가)
        explicit priority_queue(const Compare& comp) : c_(), comp_(comp) {}

        // 3. 컨테이너와 비교자를 동시에 받아오는 생성자 (선택적 추가)
        priority_queue(const Compare& comp, const Container& cont) : c_(cont), comp_(comp) {}

        [[nodiscard]] bool empty() const noexcept { return c_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return c_.size(); }

        [[nodiscard]] std::optional<value_type> top() const noexcept {
            if (empty()) return std::nullopt;
            return c_.front();
        }

        void push(const value_type& value) {
            c_.push_back(value);
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        void push(value_type&& value) {
            c_.push_back(std::move(value));
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            c_.emplace_back(std::forward<Args>(args)...);
            std::push_heap(c_.begin(), c_.end(), comp_);
        }

        // Non-throwing pop: returns true on success, false when empty
        [[nodiscard]] bool pop() noexcept {
            if (empty()) return false;
            std::pop_heap(c_.begin(), c_.end(), comp_);
            c_.pop_back();
            return true;
        }

        void clear() noexcept {
            c_.clear();
        }

        void swap(priority_queue& other) noexcept {
            using std::swap;
            swap(c_, other.c_);
            swap(comp_, other.comp_);
        }

    private:
        Container c_;
        Compare comp_;
    };

}
