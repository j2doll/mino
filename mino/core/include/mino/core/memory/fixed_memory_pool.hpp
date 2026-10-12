#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

namespace mino::core::memory {

    class fixed_memory_pool {
    private:
        struct free_node {
            free_node* next{ nullptr };
        };

        struct memory_chunk {
            std::byte* data{ nullptr };
            explicit memory_chunk(std::size_t size) : data(new std::byte[size]) {}
            ~memory_chunk() { delete[] data; }

            memory_chunk(memory_chunk&& other) noexcept : data(other.data) {
                other.data = nullptr;
            }
            memory_chunk& operator=(memory_chunk&& other) noexcept {
                if (this != &other) {
                    delete[] data;
                    data = other.data;
                    other.data = nullptr;
                }
                return *this;
            }

            memory_chunk(const memory_chunk&) = delete;
            memory_chunk& operator=(const memory_chunk&) = delete;
        };

    public:
        /**
         * @brief 고정 블록 메모리 풀 생성자
         * @param block_size 블록 1개의 크기 (bytes)
         * @param blocks_per_chunk 청크 1개당 생성할 블록 개수
         * @param max_chunks 최대 생성 가능한 청크 개수 (0이면 무제한 동적 확장)
         */
        explicit fixed_memory_pool(std::size_t block_size,
            std::size_t blocks_per_chunk = 64,
            std::size_t max_chunks = 0)
            : blocks_per_chunk_(std::max<std::size_t>(1, blocks_per_chunk)),
            max_chunks_(max_chunks),
            free_list_(nullptr),
            allocated_count_(0) {
            // 프리 리스트 포인터 크기 및 기본 정렬 요건 충족
            constexpr std::size_t min_block_size = sizeof(free_node);
            constexpr std::size_t alignment = alignof(std::max_align_t);

            std::size_t raw_size = std::max(block_size, min_block_size);
            aligned_block_size_ = (raw_size + alignment - 1) & ~(alignment - 1);
        }

        ~fixed_memory_pool() = default;

        fixed_memory_pool(const fixed_memory_pool&) = delete;
        fixed_memory_pool& operator=(const fixed_memory_pool&) = delete;
        fixed_memory_pool(fixed_memory_pool&&) = delete;
        fixed_memory_pool& operator=(fixed_memory_pool&&) = delete;

        /**
         * @brief 블록 1개 할당 (O(1))
         * @return 할당된 메모리 포인터. 최대 청크 제한에 도달하여 할당 불가 시 nullptr 반환
         */
        void* allocate() {
            std::lock_guard<std::mutex> lock(mtx_);
            if (free_list_ == nullptr) {
                // 최대 청크 제한이 설정되어 있고 이미 최대치에 도달한 경우 할당 거부
                if (max_chunks_ > 0 && chunks_.size() >= max_chunks_) {
                    return nullptr;
                }
                allocate_new_chunk();
            }

            free_node* head = free_list_;
            free_list_ = head->next;
            ++allocated_count_;
            return static_cast<void*>(head);
        }

        /**
         * @brief 블록 1개 반환 (O(1))
         */
        void deallocate(void* ptr) noexcept {
            if (ptr == nullptr) {
                return;
            }

            std::lock_guard<std::mutex> lock(mtx_);
            auto* node = static_cast<free_node*>(ptr);
            node->next = free_list_;
            free_list_ = node;
            if (allocated_count_ > 0) {
                --allocated_count_;
            }
        }

        // 정렬된 개별 블록 크기 (bytes)
        std::size_t block_size() const noexcept {
            return aligned_block_size_;
        }

        // 청크당 블록 개수 단위
        std::size_t blocks_per_chunk() const noexcept {
            return blocks_per_chunk_;
        }

        // 최대 허용 청크 개수 (0이면 무제한)
        std::size_t max_chunks() const noexcept {
            return max_chunks_;
        }

        // 현재 생성된 총 청크 개수
        std::size_t chunk_count() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            return chunks_.size();
        }

        // 현재 확보되어 있는 총 블록 용량 (현재 생성된 청크 기준 슬롯 수)
        std::size_t capacity() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            return chunks_.size() * blocks_per_chunk_;
        }

        // 풀이 도달할 수 있는 최대 블록 용량 (0이면 무제한)
        std::size_t max_capacity() const noexcept {
            return max_chunks_ * blocks_per_chunk_;
        }

        // 현재 사용(할당) 중인 활성 블록 수
        std::size_t allocated_count() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            return allocated_count_;
        }

        // 현재 확보된 청크 내에서 새 청크 생성 없이 즉시 할당 가능한 여유 블록 수
        std::size_t available_count() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            return (chunks_.size() * blocks_per_chunk_) - allocated_count_;
        }

        // OS 힙 할당(새 청크 생성) 없이 기존 프리 리스트에서 즉시 O(1)로 할당 가능한지 검사
        bool can_allocate_immediately() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            return free_list_ != nullptr;
        }

        /**
         * @brief 추가 할당이 가능한 상태인지 사전 검사
         * @return true: 빈 블록이 남아있거나 청크 확장이 가능함 / false: 최대 한도 도달로 할당 불가
         */
        bool can_allocate() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            if (free_list_ != nullptr) {
                return true;
            }
            // 프리 리스트가 비어 있을 때: 청크를 더 생성할 수 있는지 여부 확인
            return (max_chunks_ == 0) || (chunks_.size() < max_chunks_);
        }

        /**
         * @brief 풀이 최대 용량 한도까지 도달하여 더 이상 할당할 수 없는 만석 상태인지 검사
         * @note max_chunks가 0(무제한)인 경우 항상 false 반환
         */
        bool is_full() const noexcept {
            std::lock_guard<std::mutex> lock(mtx_);
            if (max_chunks_ == 0) {
                return false;
            }
            return (chunks_.size() >= max_chunks_) && (free_list_ == nullptr);
        }

    private:
        void allocate_new_chunk() {
            std::size_t chunk_bytes = aligned_block_size_ * blocks_per_chunk_;
            chunks_.emplace_back(chunk_bytes);

            std::byte* chunk_data = chunks_.back().data;
            for (std::size_t i = 0; i < blocks_per_chunk_; ++i) {
                auto* node = reinterpret_cast<free_node*>(chunk_data + (i * aligned_block_size_));
                node->next = free_list_;
                free_list_ = node;
            }
        }

        std::size_t aligned_block_size_{ 0 };
        std::size_t blocks_per_chunk_{ 64 };
        std::size_t max_chunks_{ 0 };
        std::size_t allocated_count_{ 0 };
        free_node* free_list_{ nullptr };
        std::vector<memory_chunk> chunks_;
        mutable std::mutex mtx_;
    };

} // namespace mino::core::memory
