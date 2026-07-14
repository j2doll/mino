#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <initializer_list>
#include <utility>

namespace mino::core::container {

    template <typename T, typename Allocator = std::allocator<T>>
    class  stable_vector {
    private:
        using ptr_allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;

        // 실제 데이터를 가리키는 포인터들의 벡터
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
        // Types
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

        // Iterators
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

        // Constructors & Destructor
        stable_vector() noexcept(noexcept(Allocator())) = default;
        explicit stable_vector(const Allocator& alloc) noexcept : m_alloc(alloc) {}

        explicit stable_vector(size_type count, const Allocator& alloc = Allocator()) : m_alloc(alloc) {
            resize(count);
        }

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

        // Copy & Move
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

        stable_vector(stable_vector&& other) noexcept : m_impl(std::move(other.m_impl)), m_alloc(std::move(other.m_alloc)) {}

        stable_vector& operator=(stable_vector&& other) noexcept {
            if (this != &other) {
                clear_internal();
                m_impl = std::move(other.m_impl);
                m_alloc = std::move(other.m_alloc);
            }
            return *this;
        }

        // Element Access
        reference at(size_type pos) {
            if (pos >= size()) throw std::out_of_range("stable_vector::at out of range");
            return *m_impl[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size()) throw std::out_of_range("stable_vector::at out of range");
            return *m_impl[pos];
        }

        reference operator[](size_type pos) { return *m_impl[pos]; }
        const_reference operator[](size_type pos) const { return *m_impl[pos]; }

        reference front() { return *m_impl.front(); }
        const_reference front() const { return *m_impl.front(); }
        reference back() { return *m_impl.back(); }
        const_reference back() const { return *m_impl.back(); }

        // Capacity
        bool empty() const noexcept { return m_impl.empty(); }
        size_type size() const noexcept { return m_impl.size(); }
        size_type max_size() const noexcept { return m_impl.max_size(); }
        void reserve(size_type new_cap) { m_impl.reserve(new_cap); }
        size_type capacity() const noexcept { return m_impl.capacity(); }
        void shrink_to_fit() { m_impl.shrink_to_fit(); }

        // Modifiers
        void clear() noexcept { clear_internal(); }

        template <typename... Args>
        reference emplace_back(Args&&... args) {
            T* ptr = m_alloc.allocate(1);
            try {
                std::allocator_traits<Allocator>::construct(m_alloc, ptr, std::forward<Args>(args)...);
            }
            catch (...) {
                m_alloc.deallocate(ptr, 1);
                throw;
            }
            m_impl.push_back(ptr);
            return *ptr;
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

        // Iterators Support
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
    };

    template <typename T, typename Alloc>
    void swap(stable_vector<T, Alloc>& lhs, stable_vector<T, Alloc>& rhs) noexcept {
        lhs.swap(rhs);
    }

} 
