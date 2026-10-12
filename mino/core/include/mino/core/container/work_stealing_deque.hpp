#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace mino::core::container {

    template <typename T>
    class work_stealing_deque {
    public:
        work_stealing_deque() = default;
        ~work_stealing_deque() = default;

        work_stealing_deque(const work_stealing_deque&) = delete;
        work_stealing_deque& operator=(const work_stealing_deque&) = delete;

        // 소유자 워커가 하단(Back)에 작업 푸시 (LIFO)
        void push(T item) {
            std::lock_guard<std::mutex> lock(mtx_);
            deque_.push_back(std::move(item));
        }

        // 소유자 워커가 하단(Back)에서 작업 꺼냄 (LIFO)
        std::optional<T> pop() {
            std::lock_guard<std::mutex> lock(mtx_);
            if (deque_.empty()) {
                return std::nullopt;
            }
            T item = std::move(deque_.back());
            deque_.pop_back();
            return item;
        }

        // 다른 유휴 워커가 상단(Front)에서 작업을 훔쳐감 (FIFO, Non-blocking try_lock)
        std::optional<T> steal() {
            std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
            if (!lock.owns_lock() || deque_.empty()) {
                return std::nullopt;
            }
            T item = std::move(deque_.front());
            deque_.pop_front();
            return item;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mtx_);
            return deque_.empty();
        }

        std::size_t size() const {
            std::lock_guard<std::mutex> lock(mtx_);
            return deque_.size();
        }

    private:
        mutable std::mutex mtx_;
        std::deque<T> deque_;
    };

} // namespace mino::core::container
