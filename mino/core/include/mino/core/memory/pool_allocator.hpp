#pragma once

#include "mino/core/memory/fixed_memory_pool.hpp"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace mino::core::memory {

    template <typename T>
    class pool_allocator {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::false_type;

        template <typename U>
        struct rebind {
            using other = pool_allocator<U>;
        };

        explicit pool_allocator(fixed_memory_pool* pool = nullptr) noexcept
            : pool_(pool) {
        }

        template <typename U>
        pool_allocator(const pool_allocator<U>& other) noexcept
            : pool_(other.pool_) {
        }

        ~pool_allocator() = default;

        T* allocate(std::size_t n) {
            if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
                throw std::bad_array_new_length();
            }

            // 단일 객체 요청이고 메모리 풀의 블록 크기 범위 내인 경우 풀에서 할당
            if (pool_ != nullptr && n == 1 && sizeof(T) <= pool_->block_size()) {
                return static_cast<T*>(pool_->allocate());
            }

            // 연속 메모리 요청(n > 1) 또는 풀이 없는 경우 글로벌 힙으로 폴백
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void deallocate(T* ptr, std::size_t n) noexcept {
            if (ptr == nullptr) {
                return;
            }

            if (pool_ != nullptr && n == 1 && sizeof(T) <= pool_->block_size()) {
                pool_->deallocate(ptr);
                return;
            }

            ::operator delete(static_cast<void*>(ptr));
        }

        fixed_memory_pool* get_pool() const noexcept {
            return pool_;
        }

        template <typename U>
        bool operator==(const pool_allocator<U>& other) const noexcept {
            return pool_ == other.pool_;
        }

        template <typename U>
        bool operator!=(const pool_allocator<U>& other) const noexcept {
            return !(*this == other);
        }

    private:
        template <typename U>
        friend class pool_allocator;

        fixed_memory_pool* pool_{ nullptr };
    };

} // namespace mino::core::memory
