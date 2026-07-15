#pragma once

#include <functional>
#include <utility>
#include <stdexcept>
#include <vector>
#include <algorithm>

// 이항 힙(Binomial Heap)은 일반적인 이진 힙(Binary Heap)보다
// 두 힙을 하나로 합치는 연산(Merge/Union)을 훨씬 더 효율적으로 처리하기 위해
// 고안된 고급 우선순위 큐(Priority Queue) 자료구조입니다.
// 
// +-----------------+---------------+-----------------+------------------------+
// | 연산            | 이진 힙       | 이항 힙         | 특징 요약              |
// |                 | Binary Heap   | Binomial Heap   |                        |
// +-----------------+---------------+-----------------+------------------------+
// | 우선순위 상위   |               |                 |                        |
// | 원소 확인       |               |                 |                        |
// | Find Min        | O(1)          | O(log n)        | 루트 노드들만 확인     |
// |                 |               |                 |                        |
// | 합치기          |               |                 |                        |
// | Union (Merge)   | O(n)          | O(log n)        | 이진수 덧셈처럼 합침   |
// |                 |               |                 |                        |
// | 삽입            |               |                 |                        |
// | Insert          | O(log n)      | Amortized O(1)  | B_0 트리를 Union       |
// |                 |               |                 |                        |
// | 최솟값 추출     |               |                 |                        |
// | Extract Min     | O(log n)      | O(log n)        | 루트 제거 후 자식합침  |
// |                 |               |                 |                        |
// | 값 감소         |               |                 |                        |
// | Decrease Key    | O(log n)      | O(log n)        | 부모와 비교하며 스왑   |
// |                 |               |                 |                        |
// | 삭제            |               |                 |                        |
// | Delete          | O(log n)      | O(log n)        | 값 낮춘 후 최솟값 추출 |
// +-----------------+---------------+-----------------+------------------------+
// 

namespace mino::core::container {

    template <typename T, typename Compare = std::less<T>>
    class  binomial_heap {
    public:
        using value_type = T;
        using compare_type = Compare;
        using size_type = std::size_t;

        // 노드 구조체: value, degree(차수), child(가장 왼쪽 자식), sibling(오른쪽 형제)
        struct node {
            value_type value;
            size_type degree = 0;
            node* child = nullptr;
            node* sibling = nullptr;
            template <typename... Args>
            node(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        // 생성자: 빈 힙 생성
        binomial_heap() : head_(nullptr), size_(0), comp_(Compare()) {}
        // 소멸자: 내부 노드들 모두 삭제
        ~binomial_heap() { clear(); }

        // 빈지 확인: 사이즈가 0이면 true
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        // 요소 개수 반환
        [[nodiscard]] size_type size() const noexcept { return size_; }

        // 최상위(우선순위가 가장 높은) 값 참조 반환
        // 1) 비어있으면 예외 발생
        // 2) 내부적으로 루트 리스트에서 최대(또는 비교로 정의된 우선순위) 노드를 찾음
        const value_type& top() const {
            if (empty()) throw std::runtime_error("Heap is empty");
            return find_max_node()->value;
        }

        // 인플레이스 생성: 노드를 생성하여 임시 힙으로 만들고 merge 수행
        // 반환값: 생성된 노드 포인터
        template <typename... Args>
        node* emplace(Args&&... args) {
            // 1) 새 노드 동적 할당
            node* new_node = new node(std::forward<Args>(args)...);
            // 2) 단일 노드만 가진 임시 힙 구성
            binomial_heap temp_heap;
            temp_heap.head_ = new_node;
            temp_heap.size_ = 1;
            // 3) 현재 힙과 합치기(이 작업은 O(1) amortized 삽입처럼 동작)
            this->merge(temp_heap);
            // 4) 새 노드 포인터 반환
            return new_node;
        }

        // 편의 함수: const value_type& 를 받아서 emplace 호출
        node* push(const value_type& value) { return emplace(value); }

        // 최상위 요소 제거
        // 동작 요약:
        // 1) 비었으면 예외
        // 2) 루트 리스트에서 최대(우선순위) 루트 노드와 그 이전 노드를 찾음
        // 3) 해당 루트를 루트 리스트에서 제거
        // 4) 제거한 노드의 자식들을 뒤집어서 새로운 힙으로 구성
        // 5) 삭제한 노드 메모리 해제, size 감소
        // 6) 새로 구성한 자식 힙을 현재 힙과 merge
        void pop() {
            if (empty()) throw std::runtime_error("Heap is empty");

            // 1) 루트 리스트 순회해서 우선순위가 가장 높은 루트를 찾음
            node* max_prev = nullptr;
            node* max_curr = head_;
            node* prev = nullptr;
            node* curr = head_;
            value_type max_val = head_->value;

            while (curr) {
                // 1-1) comp_는 기본적으로 std::less<T> 이므로
                //      comp_(a, b) == true 이면 b가 더 우선순위(큰 값)
                if (comp_(max_val, curr->value)) {
                    max_val = curr->value;
                    max_prev = prev;
                    max_curr = curr;
                }
                prev = curr;
                curr = curr->sibling;
            }

            // 2) 발견한 루트 노드를 루트 리스트에서 제거
            if (max_prev) max_prev->sibling = max_curr->sibling;
            else head_ = max_curr->sibling;

            // 3) 제거한 루트의 자식 리스트를 역순으로 뒤집음
            node* child_curr = max_curr->child;
            node* reversed_child_head = nullptr;
            while (child_curr) {
                node* next = child_curr->sibling;
                // 3-1) 현재 자식의 sibling 포인터를 뒤집어진 리스트의 헤드로 연결
                child_curr->sibling = reversed_child_head;
                reversed_child_head = child_curr;
                child_curr = next;
            }

            // 4) 뒤집힌 자식들을 별도 임시 힙으로 만들어 merge 준비
            binomial_heap temp_heap;
            temp_heap.head_ = reversed_child_head;

            // 자식 노드 수 계산
            size_type child_count = count_nodes(reversed_child_head);
            temp_heap.size_ = child_count;

            // 5) 삭제할 노드 메모리 해제 및 size 조정
            // 현재 size_는 전체 노드 수를 가리키고 있으므로,
            // 자식들을 temp_heap로 분리한 시점에서 current 힙에서 해당 자식 수를 빼야 함.
            size_type old_size = size_;
            // 빼야 할 값 = 1 (삭제할 루트) + child_count (분리된 자식들)
            if (old_size < (1 + child_count)) {
                // 안전을 위해 0으로 맞춤(이상 상태면 clear)
                size_ = 0;
            } else {
                size_ = old_size - 1 - child_count;
            }
            delete max_curr;

            // 6) 자식 힙을 현재 힙과 합침
            this->merge(temp_heap);
        }

        // 다른 힙과 합치기 (merge/union)
        // 1) 자기 자신과의 merge는 무시
        // 2) 루트 리스트들을 degree 기준으로 정렬된 상태로 병합 (merge_roots)
        // 3) 병합된 루트 리스트를 순회하면서 동일 차수 트리들을 적절히 결합(link_trees)
        // 4) other 힙은 비워짐(head=nullptr, size=0)
        void merge(binomial_heap& other) {
            if (this == &other || other.empty()) return;

            // 1) 루트 리스트들을 degree 오름차순으로 병합
            head_ = merge_roots(head_, other.head_);
            // 2) 크기 합산
            size_ += other.size_;
            // 3) other를 비우기
            other.head_ = nullptr;
            other.size_ = 0;

            if (!head_) return;

            // 4) 병합된 루트 리스트에서 차수가 같은 인접 트리들을 결합
            node* prev = nullptr;
            node* curr = head_;
            node* next = curr->sibling;

            while (next) {
                // 4-1) 케이스: curr과 next의 degree가 다르거나
                //       next의 다음 형제가 존재하고 그 차수가 curr과 같으면 이동
                if ((curr->degree != next->degree) || (next->sibling && next->sibling->degree == curr->degree)) {
                    prev = curr;
                    curr = next;
                }
                // 4-2) next의 루트가 우선순위가 더 높다면 curr 뒤에 next를 남기고 curr을 next의 자식으로 연결
                else if (comp_(next->value, curr->value)) {
                    curr->sibling = next->sibling;
                    link_trees(next, curr);
                }
                // 4-3) curr의 루트가 우선순위가 더 높다면 next를 curr의 자식으로 연결
                else {
                    if (!prev) head_ = next;
                    else prev->sibling = next;
                    link_trees(curr, next);
                    curr = next;
                }
                next = curr->sibling;
            }
        }

        // 모든 노드들 삭제 (재귀적으로)
        void clear() noexcept {
            destroy_nodes(head_);
            head_ = nullptr;
            size_ = 0;
        }

    private:
        node* head_;
        size_type size_;
        Compare comp_;

        // 루트 리스트에서 우선순위(비교 기준)에 따라 최대(또는 우선순위 최상) 노드 포인터 반환
        // 1) head_부터 sibling을 따라서 비교하며 최댓값 노드를 추적
        node* find_max_node() const {
            node* curr = head_;
            node* max_node = head_;
            while (curr) {
                if (comp_(max_node->value, curr->value)) max_node = curr;
                curr = curr->sibling;
            }
            return max_node;
        }

        // child를 parent의 자식으로 연결 (child는 parent의 가장 왼쪽 자식이 됨)
        // 1) child의 sibling을 부모의 기존 자식으로 설정
        // 2) parent->child를 child로 갱신
        // 3) parent의 degree 증가
        void link_trees(node* child, node* parent) {
            child->sibling = parent->child;
            parent->child = child;
            parent->degree++;
        }

        // 두 루트 리스트를 degree 오름차순으로 병합하여 하나의 리스트로 만듦
        // 1) 두 리스트를 병합(병합 정렬의 merge 단계처럼)하여 degree가 증가하는 리스트 생성
        // 2) 반환값: 병합된 루트 리스트의 헤드
        node* merge_roots(node* h1, node* h2) {
            if (!h1) return h2;
            if (!h2) return h1;
            node* root_head = nullptr;
            node* root_tail = nullptr;
            while (h1 && h2) {
                // 1) degree가 작은 쪽을 chosen으로 선택
                node*& chosen = (h1->degree <= h2->degree) ? h1 : h2;
                if (!root_head) { root_head = root_tail = chosen; }
                else { root_tail->sibling = chosen; root_tail = chosen; }
                // 2) chosen을 다음으로 이동시켜 병합 진행
                chosen = chosen->sibling;
            }
            // 3) 남은 리스트 연결
            if (h1) root_tail->sibling = h1;
            if (h2) root_tail->sibling = h2;
            return root_head;
        }

        // 재귀적으로 노드들 삭제
        // 1) 현재 노드의 child가 있으면 먼저 자식 트리를 삭제
        // 2) 현재 노드를 delete한 후 sibling으로 이동
        void destroy_nodes(node* n) noexcept {
            while (n) {
                node* next = n->sibling;
                if (n->child) destroy_nodes(n->child);
                delete n;
                n = next;
            }
        }

        // 변경 추가: 주어진 루트 리스트(각 루트는 서브트리를 가짐)의 전체 노드 수를 계산
        size_type count_nodes(node* n) const noexcept {
            size_type count = 0;
            while (n) {
                // 현재 루트 노드 포함
                count += 1;
                // 자식(하위 서브트리)도 재귀적으로 계산
                if (n->child) count += count_nodes(n->child);
                n = n->sibling;
            }
            return count;
        }
    };

}  
