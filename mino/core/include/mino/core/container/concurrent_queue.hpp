#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <utility>

namespace mino::core::container
{
    // 큐가 가득 찼을 때 처리 방법
    enum class overflow_policy
    {
        drop_oldest, // 가장 오래된 항목을 버림
        reject_new   // 새로 들어오는 항목을 거부
    };

    template <typename T>
    class  concurrent_queue
    {
    public:
        // max_size == 0 이면 무제한 큐
        explicit concurrent_queue(
            std::size_t max_size = 0,
            overflow_policy policy = overflow_policy::reject_new)
            : max_size_(max_size)
            , policy_(policy)
        {
        }

        concurrent_queue(const concurrent_queue&) = delete;
        concurrent_queue& operator=(const concurrent_queue&) = delete;

        // 복사로 enqueue
        bool enqueue(const T& value)
        {
            return emplace_impl(value);
        }

        // 이동으로 enqueue
        bool enqueue(T&& value)
        {
            return emplace_impl(std::move(value));
        }

        // 큐 내부에서 객체를 직접 생성
        template <typename... Args>
        bool emplace(Args&&... args)
        {
            return emplace_impl(T(std::forward<Args>(args)...));
        }

        // 비차단 dequeue
        // - 성공 시 true, out 에 값이 들어감
        // - 큐가 비어 있으면 false
        bool try_dequeue(T& out)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (queue_.empty())
                return false;

            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        // 대기형 dequeue
        // - 큐가 비어 있으면 항목이 들어올 때까지 대기
        void wait_dequeue(T& out)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_cv_.wait(lock, [this] { return !queue_.empty(); });

            out = std::move(queue_.front());
            queue_.pop_front();
        }

        // 조건이 만족될 때만 헤드를 pop
        // pred(head, size) -> bool
        template <typename Predicate>
        bool dequeue_if(T& out, Predicate pred)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (queue_.empty())
                return false;

            const std::size_t current_size = queue_.size();
            const T& head = queue_.front();

            // 조건이 맞지 않으면 pop 하지 않음
            if (!pred(head, current_size))
                return false;

            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        // 현재 큐 크기
        std::size_t size() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        // 비어 있는지 여부
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        // 크기 제한 여부 (무제한이면 false)
        bool is_bounded() const
        {
            return max_size_ != 0;
        }

        // 설정된 최대 크기 (0 이면 무제한)
        std::size_t capacity() const
        {
            return max_size_;
        }

        // 오버플로우 정책 조회
        overflow_policy get_overflow_policy() const
        {
            return policy_;
        }

        // 멀티쓰레드에서 안전하게 큐의 모든 항목을 제거.
        // 반환값: 제거된 항목의 개수
        std::size_t clear()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            const std::size_t removed = queue_.size();
            queue_.clear();
            return removed;
        }

    private:
        // 내부 공통 enqueue 구현
        template <typename U>
        bool emplace_impl(U&& value)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            // 무제한 큐
            if (max_size_ == 0)
            {
                queue_.emplace_back(std::forward<U>(value));
                lock.unlock();
                not_empty_cv_.notify_one();
                return true;
            }

            // 크기 제한 큐
            if (queue_.size() >= max_size_)
            {
                if (policy_ == overflow_policy::drop_oldest)
                {
                    queue_.pop_front();
                    queue_.emplace_back(std::forward<U>(value));
                    lock.unlock();
                    not_empty_cv_.notify_one();
                    return true;
                }
                else // overflow_policy::reject_new
                {
                    return false;
                }
            }

            // 아직 여유 공간
            queue_.emplace_back(std::forward<U>(value));
            lock.unlock();
            not_empty_cv_.notify_one();
            return true;
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable not_empty_cv_;
        std::deque<T> queue_;
        std::size_t max_size_;
        overflow_policy policy_;
    };

}  
