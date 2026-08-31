#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <utility>
#include <optional>
#include <chrono>
#include <iostream>
#include <string>

// ============+====================+====================+==================
// 항목        |  std::queue        |  concurrent_queue  |  boost::lockfree::queue
// ------------+--------------------+--------------------+------------------
// 쓰레드 안전 | 안전하지 않음      | 뮤텍스/CV 기반 안전| 락프리(CAS) 안전
// 크기 제한   | 무제한             | 무제한/고정 선택   | 컴파일/런타임 고정
// 오버플로우  | 동적 확장          | 거부(Reject) 또는  | 항상 거부
//             |                    | 덮어쓰기(Drop)     |
// ------------+--------------------+--------------------+------------------
// Enqueue     | O(1)               | O(1) (경합 시 대기)| O(1) (비차단)
// Dequeue     | O(1)               | O(1) (비차단/대기) | O(1) (비차단)
// ------------+--------------------+--------------------+------------------
// 주요 용도   | 단일 쓰레드 작업   | 범용 생산자-소비자 | 초고성능 실시간 시스템
//             |                    | 패턴 (가독성/안정성)| (T가 TriviallyCopyable)
// ============+====================+====================+==================
//
// concurrent_queue는 멀티쓰레드 환경에서 안전하게 사용할 수 있는 동시성 큐입니다.
//
// concurrent_queue<int> queue(10, overflow_policy::reject_new);
// 
// // Producer 쓰레드
// std::thread producer([&queue]() {
//     for (int i = 0; i < 5; ++i) {
//         queue.enqueue(i * 10);
//     }
// });
// 
// // Consumer 쓰레드 (대기형)
// std::thread consumer([&queue]() {
//     int value;
//     for (int i = 0; i < 5; ++i) {
//         queue.wait_dequeue(value);
//         std::cout << "Dequeued: " << value << std::endl;
//     }
// });
// 
// producer.join();
// consumer.join();
// 
// // 조건부 dequeue 예제
// queue.enqueue(50);
// int result;
// bool ok = queue.dequeue_if(result, [](const int& val, std::size_t size) {
//     return val > 40; // 50 > 40 이므로 dequeue 성공
// });
// 

namespace mino::core::container
{
    enum class overflow_policy
    {
        drop_oldest, // 가득 찼을 때 가장 오래된 항목을 버림
        reject_new   // 가득 찼을 때 새로운 항목을 거부 (기본값)
    };

    template <typename T>
    class concurrent_queue
    {
    public:
        using value_type = T;
        using size_type = std::size_t;

        explicit concurrent_queue(
            size_type max_size = 0,
            overflow_policy policy = overflow_policy::reject_new)
            : max_size_(max_size)
            , policy_(policy)
        {
        }

        concurrent_queue(const concurrent_queue&) = delete;
        concurrent_queue& operator=(const concurrent_queue&) = delete;

        // --------------------------------------------------------------------
        // 1. Enqueue 연산
        // --------------------------------------------------------------------
        bool enqueue(const T& value)
        {
            return emplace(value);
        }

        bool enqueue(T&& value)
        {
            return emplace(std::move(value));
        }

        template <typename... Args>
        bool emplace(Args&&... args)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (max_size_ != 0 && queue_.size() >= max_size_)
            {
                if (policy_ == overflow_policy::drop_oldest)
                {
                    queue_.pop_front();
                }
                else
                {
                    return false;
                }
            }

            queue_.emplace_back(std::forward<Args>(args)...);
            lock.unlock();
            not_empty_cv_.notify_one();
            return true;
        }

        // --------------------------------------------------------------------
        // 2. Dequeue 연산
        // --------------------------------------------------------------------
        // 비차단 Dequeue
        bool try_dequeue(T& out)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (queue_.empty())
                return false;

            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        std::optional<T> try_dequeue()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (queue_.empty())
                return std::nullopt;

            T item = std::move(queue_.front());
            queue_.pop_front();
            return item;
        }

        // 대기형 Dequeue (무한 대기)
        void wait_dequeue(T& out)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_cv_.wait(lock, [this] { return !queue_.empty(); });

            out = std::move(queue_.front());
            queue_.pop_front();
        }

        // 타임아웃 대기형 Dequeue
        template <typename Rep, typename Period>
        bool wait_dequeue_for(T& out, const std::chrono::duration<Rep, Period>& rel_time)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!not_empty_cv_.wait_for(lock, rel_time, [this] { return !queue_.empty(); }))
            {
                return false;
            }

            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        // 조건부 Dequeue
        template <typename Predicate>
        bool dequeue_if(T& out, Predicate pred)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (queue_.empty())
                return false;

            const size_type current_size = queue_.size();
            const T& head = queue_.front();

            if (!pred(head, current_size))
                return false;

            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        // --------------------------------------------------------------------
        // 3. 상태 확인 및 제어
        // --------------------------------------------------------------------
        [[nodiscard]] size_type size() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        [[nodiscard]] bool empty() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        [[nodiscard]] bool is_bounded() const noexcept
        {
            return max_size_ != 0;
        }

        [[nodiscard]] size_type capacity() const noexcept
        {
            return max_size_;
        }

        [[nodiscard]] overflow_policy get_overflow_policy() const noexcept
        {
            return policy_;
        }

        size_type clear()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            const size_type removed = queue_.size();
            queue_.clear();
            return removed;
        }

        // --------------------------------------------------------------------
        // 4. 순수 ASCII 기반 Queue Dump
        // --------------------------------------------------------------------
        void dump(const std::string& title = "") const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::string cap_str = (max_size_ == 0) ? "Inf" : std::to_string(max_size_);

            if (!title.empty())
            {
                std::cout << "=== " << title << " (Size: " << queue_.size() << "/" << cap_str << ") ===\n";
            }
            else
            {
                std::cout << "=== Concurrent Queue Dump (Size: " << queue_.size() << "/" << cap_str << ") ===\n";
            }

            if (queue_.empty())
            {
                std::cout << "  \\-- <Empty Queue>\n\n";
                return;
            }

            std::cout << "Front -> ";
            for (size_type i = 0; i < queue_.size(); ++i)
            {
                std::cout << "[" << queue_[i] << "]" << (i + 1 == queue_.size() ? "" : " -> ");
            }
            std::cout << " -> Back\n\n";
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable not_empty_cv_;
        std::deque<T> queue_;
        size_type max_size_;
        overflow_policy policy_;
    };

} // namespace mino::core::container

