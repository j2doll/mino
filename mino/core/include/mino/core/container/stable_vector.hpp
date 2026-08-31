#pragma once

#include <vector>
#include <memory>
#include <iterator>
#include <initializer_list>
#include <utility>
#include <optional>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::vector       |  std::deque        |  stable_vector
// ------------+--------------------+--------------------+------------------
// 메모리 구조 | 단일 연속 배열     | 다중 청크(Chunk)   | 간접 참조 포인터 테이블
// 요소 메모리 | 재할당 시 전체 이동| 청크 내 이동       | 노드 개별 할당 (주소 불변)
// 참조 안정성 | 재할당 시 무효화   | 양 끝 삽입 시 무효 | 영구 불변 (절대 무효화 안 됨)
// ------------+--------------------+--------------------+------------------
// 임의 접근   | O(1) (최속)        | O(1) (2중 역참조)  | O(1) (2중 역참조)
// 캐시 효율성 | 최상 (연속 메모리) | 우수               | 보통 (포인터 역참조)
// 삽입 (뒤쪽) | O(1) [상환]        | O(1)               | O(1) [상환]
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 범용 연속 컨테이너 | 양방향 큐          | 원소의 포인터/참조를 외부에
//             |                    |                    | 안전하게 보관해야 하는 경우
// ============+====================+====================+==================
//
// stable_vector는 내부적으로 포인터 인덱스 테이블(std::vector<T*>)을 유지하여
// 벡터가 재할당(Reallocation)되더라도 기존 요소의 메모리 주소와 참조가
// 절대 무효화되지 않는(Stable Reference) 컨테이너입니다.
//
// stable_vector<int> sv;
// 
// // [1] 요소 추가
// sv.push_back(10);
// sv.push_back(20);
// 
// // [2] 요소의 주소 및 참조 보관 (안정성 테스트)
// int* ptr10 = &sv[0];
// int& ref20 = sv[1];
// 
// // [3] 대량 삽입으로 내부 벡터 재할당 유발
// for (int i = 30; i <= 100; i += 10) {
//     sv.push_back(i);
// }
// 
// // 재할당 후에도 이전에 얻은 포인터와 참조가 100% 안전하게 유지됨
// std::cout << "*ptr10: " << *ptr10 << std::endl; // 10
// std::cout << "ref20: "  << ref20  << std::endl; // 20
// 
// // [4] 임의 접근 및 수정
// sv[0] = 99;
// std::cout << "*ptr10 after update: " << *ptr10 << std::endl; // 99
// 
// // [5] 초기화
// sv.clear();
// std::cout << "Is empty: " << sv.empty() << std::endl; // 1 (true)
// 

namespace mino::core::container {

    template <typename T, typename Allocator = std::allocator<T>>
    class stable_vector {
    private:
        using ptr_allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;
        std::vector<T*, ptr_allocator_t> m_impl;
        Allocator m_alloc;

        void clear_internal() noexcept {
            for (auto* ptr : m_impl) {
                if (ptr) {
                    std::allocator_traits<Allocator>::destroy(m_alloc, ptr);
                    m_alloc.deallocate(ptr, 1);
                }
            }
            m_impl.clear();
        }

    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

        template <typename ValueType>
        class stable_iterator {
        private:
            using base_iter_t = typename std::vector<T*, ptr_allocator_t>::iterator;
            using const_base_iter_t = typename std::vector<T*, ptr_allocator_t>::const_iterator;

            std::conditional_t<std::is_const_v<ValueType>, const_base_iter_t, base_iter_t> m_it;
            friend class stable_vector;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;
            using pointer = ValueType*;
            using reference = ValueType&;

            stable_iterator() = default;
            stable_iterator(base_iter_t it) : m_it(it) {}
            stable_iterator(const_base_iter_t it) : m_it(it) {}

            reference operator*() const { return **m_it; }
            pointer operator->() const { return *m_it; }

            stable_iterator& operator++() { ++m_it; return *this; }
            stable_iterator operator++(int) { stable_iterator tmp = *this; ++m_it; return tmp; }
            stable_iterator& operator--() { --m_it; return *this; }
            stable_iterator operator--(int) { stable_iterator tmp = *this; --m_it; return tmp; }

            stable_iterator& operator+=(difference_type n) { m_it += n; return *this; }
            stable_iterator& operator-=(difference_type n) { m_it -= n; return *this; }

            friend stable_iterator operator+(stable_iterator it, difference_type n) { it += n; return it; }
            friend stable_iterator operator+(difference_type n, stable_iterator it) { it += n; return it; }
            friend stable_iterator operator-(stable_iterator it, difference_type n) { it -= n; return it; }
            friend difference_type operator-(const stable_iterator& lhs, const stable_iterator& rhs) { return lhs.m_it - rhs.m_it; }

            bool operator==(const stable_iterator& other) const { return m_it == other.m_it; }
            bool operator!=(const stable_iterator& other) const { return m_it != other.m_it; }
            bool operator<(const stable_iterator& other) const { return m_it < other.m_it; }
            bool operator<=(const stable_iterator& other) const { return m_it <= other.m_it; }
            bool operator>(const stable_iterator& other) const { return m_it > other.m_it; }
            bool operator>=(const stable_iterator& other) const { return m_it >= other.m_it; }
        };

        using iterator = stable_iterator<T>;
        using const_iterator = stable_iterator<const T>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        stable_vector() noexcept(noexcept(Allocator())) = default;
        explicit stable_vector(const Allocator& alloc) noexcept : m_alloc(alloc) {}
        explicit stable_vector(size_type count, const Allocator& alloc = Allocator()) : m_alloc(alloc) { resize(count); }
        stable_vector(size_type count, const T& value, const Allocator& alloc = Allocator()) : m_alloc(alloc) {
            for (size_type i = 0; i < count; ++i) {
                push_back(value);
            }
        }
        stable_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : m_alloc(alloc) {
            for (const auto& item : init) {
                push_back(item);
            }
        }
        ~stable_vector() { clear_internal(); }

        stable_vector(const stable_vector& other) : m_alloc(other.m_alloc) {
            for (const auto& item : other) {
                push_back(item);
            }
        }

        stable_vector& operator=(const stable_vector& other) {
            if (this != &other) {
                clear_internal();
                m_alloc = other.m_alloc;
                for (const auto& item : other) {
                    push_back(item);
                }
            }
            return *this;
        }

        stable_vector(stable_vector&& other) noexcept
            : m_impl(std::move(other.m_impl)), m_alloc(std::move(other.m_alloc)) {
        }

        stable_vector& operator=(stable_vector&& other) noexcept {
            if (this != &other) {
                clear_internal();
                m_impl = std::move(other.m_impl);
                m_alloc = std::move(other.m_alloc);
            }
            return *this;
        }

        pointer at(size_type pos) {
            if (pos >= size()) return nullptr;
            return m_impl[pos];
        }

        const_pointer at(size_type pos) const {
            if (pos >= size()) return nullptr;
            return m_impl[pos];
        }

        reference operator[](size_type pos) { return *m_impl[pos]; }
        const_reference operator[](size_type pos) const { return *m_impl[pos]; }

        reference front() { return *m_impl.front(); }
        const_reference front() const { return *m_impl.front(); }
        reference back() { return *m_impl.back(); }
        const_reference back() const { return *m_impl.back(); }

        [[nodiscard]] bool empty() const noexcept { return m_impl.empty(); }
        [[nodiscard]] size_type size() const noexcept { return m_impl.size(); }
        [[nodiscard]] size_type max_size() const noexcept { return m_impl.max_size(); }
        void reserve(size_type new_cap) { m_impl.reserve(new_cap); }
        [[nodiscard]] size_type capacity() const noexcept { return m_impl.capacity(); }
        void shrink_to_fit() { m_impl.shrink_to_fit(); }

        void clear() noexcept { clear_internal(); }

        template <typename... Args>
        pointer emplace_back(Args&&... args) {
            T* ptr = m_alloc.allocate(1);
            try {
                std::allocator_traits<Allocator>::construct(m_alloc, ptr, std::forward<Args>(args)...);
                try {
                    m_impl.push_back(ptr);
                }
                catch (...) {
                    std::allocator_traits<Allocator>::destroy(m_alloc, ptr);
                    throw;
                }
            }
            catch (...) {
                m_alloc.deallocate(ptr, 1);
                return nullptr;
            }
            return ptr;
        }

        void push_back(const T& value) { emplace_back(value); }
        void push_back(T&& value) { emplace_back(std::move(value)); }

        void pop_back() {
            if (!empty()) {
                T* ptr = m_impl.back();
                std::allocator_traits<Allocator>::destroy(m_alloc, ptr);
                m_alloc.deallocate(ptr, 1);
                m_impl.pop_back();
            }
        }

        void resize(size_type count) {
            if (count < size()) {
                while (size() > count) pop_back();
            }
            else {
                while (size() < count) emplace_back();
            }
        }

        void resize(size_type count, const T& value) {
            if (count < size()) {
                while (size() > count) pop_back();
            }
            else {
                while (size() < count) push_back(value);
            }
        }

        void swap(stable_vector& other) noexcept {
            using std::swap;
            swap(m_impl, other.m_impl);
            swap(m_alloc, other.m_alloc);
        }

        iterator begin() noexcept { return iterator(m_impl.begin()); }
        iterator end() noexcept { return iterator(m_impl.end()); }
        const_iterator begin() const noexcept { return const_iterator(m_impl.begin()); }
        const_iterator end() const noexcept { return const_iterator(m_impl.end()); }
        const_iterator cbegin() const noexcept { return const_iterator(m_impl.cbegin()); }
        const_iterator cend() const noexcept { return const_iterator(m_impl.cend()); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        // ==========================================
        // 순수 ASCII 기반 Stable Vector Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            if (!title.empty()) {
                std::cout << "=== " << title << " (Size: " << size() << ", Cap: " << capacity() << ") ===\n";
            }
            else {
                std::cout << "=== Stable Vector Dump (Size: " << size() << ", Cap: " << capacity() << ") ===\n";
            }

            if (empty()) {
                std::cout << "  \\-- <Empty Stable Vector>\n\n";
                return;
            }

            for (size_type i = 0; i < m_impl.size(); ++i) {
                bool is_last = (i == m_impl.size() - 1);
                std::string connector = is_last ? "\\-- " : "|-- ";
                std::cout << connector << "[" << i << "] (NodePtr: " << static_cast<const void*>(m_impl[i])
                    << ") => " << *m_impl[i] << "\n";
            }
            std::cout << "\n";
        }
    };

    template <typename T, typename Alloc>
    void swap(stable_vector<T, Alloc>& lhs, stable_vector<T, Alloc>& rhs) noexcept {
        lhs.swap(rhs);
    }

} // namespace mino::core::container
