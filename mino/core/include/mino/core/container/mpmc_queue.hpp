#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>
#include <functional>

namespace mino::core::container {

    template <typename T>
    class mpmc_queue {
    private:
        mutable std::mutex mtx_;
        std::queue<T> data_queue_;
        std::condition_variable data_cond_;

    public:
        mpmc_queue() = default;
        ~mpmc_queue() = default;

        // 멀티스레드 환경에서 복사는 위험하므로 복사 생성자 및 대입 연산자 삭제
        mpmc_queue(const mpmc_queue&) = delete;
        mpmc_queue& operator=(const mpmc_queue&) = delete;

        // 1. 큐에 데이터 삽입 (Multi-In)
        void push(T new_value) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_queue_.push(std::move(new_value));
            data_cond_.notify_one(); // 대기 중인 소비자 스레드 중 하나를 깨움
        }

        // 2. 데이터가 들어올 때까지 대기 후 추출 (Multi-Out)
        void wait_and_pop(T& value) {
            std::unique_lock<std::mutex> lock(mtx_);
            data_cond_.wait(lock, [this] { return !data_queue_.empty(); });
            value = std::move(data_queue_.front());
            data_queue_.pop();
        }

        // 3. 블로킹 없이 데이터를 즉시 추출 시도
        std::optional<T> pop() {
            std::lock_guard<std::mutex> lock(mtx_);
            if (data_queue_.empty()) {
                return std::nullopt;
            }
            T value = std::move(data_queue_.front());
            data_queue_.pop();
            return value;
        }

        // 4. 값을 제거하지 않고 맨 앞의 요소를 살짝 들여다보기 (Pre-test / Peek)
        std::optional<T> peek() const {
            std::lock_guard<std::mutex> lock(mtx_);
            if (data_queue_.empty()) {
                return std::nullopt;
            }
            return data_queue_.front();
        }

        // 5. 특정 조건을 만족하는 데이터가 있을 때만 안전하게 꺼내오기 (조건부 팝)
        bool pop_if(const std::function<bool(const T&)>& predicate, T& value) {
            std::lock_guard<std::mutex> lock(mtx_);
            if (data_queue_.empty()) {
                return false;
            }

            // 맨 앞의 데이터가 조건을 만족하는지 확인
            if (predicate(data_queue_.front())) {
                value = std::move(data_queue_.front());
                data_queue_.pop();
                return true;
            }

            return false; // 조건 불일치
        }

        // 6. 큐가 비어있는지 확인
        bool empty() const {
            std::lock_guard<std::mutex> lock(mtx_);
            return data_queue_.empty();
        }

        // 7. 현재 큐에 들어있는 데이터 개수 반환
        size_t size() const {
            std::lock_guard<std::mutex> lock(mtx_);
            return data_queue_.size();
        }
    };

} // namespace mino::core::container
