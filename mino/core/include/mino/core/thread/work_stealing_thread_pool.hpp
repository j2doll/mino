#pragma once

#include "mino/core/container/work_stealing_deque.hpp"
#include "mino/core/task/move_task.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace mino::core::thread {

    // work_stealing_thread_pool 는 작업을 모두 마친 유휴(놀고 있는) 스레드가
    // 다른 바쁜 스레드의 큐에서 남은 작업을 '훔쳐와서(Steal)'
    // 대신 처리하는 Work-Stealing(작업 훔치기) 스케줄링 알고리즘을 사용
    class work_stealing_thread_pool {
    public:
        explicit work_stealing_thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
            : stop_(false), thread_count_(thread_count ? thread_count : 1) {

            queues_.reserve(thread_count_);
            for (std::size_t i = 0; i < thread_count_; ++i) {
                queues_.push_back(std::make_unique<container::work_stealing_deque<task::move_task>>());
            }

            workers_.reserve(thread_count_);
            for (std::size_t i = 0; i < thread_count_; ++i) {
                workers_.emplace_back(&work_stealing_thread_pool::worker_loop, this, i);
            }
        }

        ~work_stealing_thread_pool() {
            stop();
        }

        work_stealing_thread_pool(const work_stealing_thread_pool&) = delete;
        work_stealing_thread_pool& operator=(const work_stealing_thread_pool&) = delete;

        template <typename F, typename... Args>
        auto submit(F&& fn, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>> {
            using return_type = std::invoke_result_t<F, Args...>;

            auto task_pkg = std::make_shared<std::packaged_task<return_type()>>(
                [task_fn = std::decay_t<F>(std::forward<F>(fn)),
                task_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                    return std::apply(std::move(task_fn), std::move(task_args));
                }
            );

            std::future<return_type> future_result = task_pkg->get_future();
            task::move_task task([task_pkg]() { (*task_pkg)(); });

            // 작업 분배: 라운드로빈 방식으로 큐에 고르게 적재
            static std::atomic<std::size_t> next_queue{ 0 };
            std::size_t target = next_queue.fetch_add(1, std::memory_order_relaxed) % thread_count_;
            queues_[target]->push(std::move(task));

            cv_.notify_all();
            return future_result;
        }

        void stop() noexcept {
            if (!stop_.exchange(true, std::memory_order_acq_rel)) {
                cv_.notify_all();
                for (auto& worker : workers_) {
                    if (worker.joinable()) {
                        worker.join();
                    }
                }
            }
        }

        std::size_t thread_count() const noexcept {
            return thread_count_;
        }

    private:
        void worker_loop(std::size_t worker_id) {
            this_worker_id_ = static_cast<int>(worker_id);
            std::mt19937 rng(static_cast<unsigned int>(worker_id + 100));
            std::uniform_int_distribution<std::size_t> dist(0, thread_count_ - 1);

            while (true) {
                // 1. 자신의 덱에서 작업 확인 (LIFO)
                auto current_task = queues_[worker_id]->pop();

                // 2. 자신의 작업이 없으면 무작위로 다른 워커로부터 훔치기 시도 (Steal, FIFO)
                if (!current_task) {
                    for (std::size_t i = 0; i < thread_count_; ++i) {
                        std::size_t victim_id = dist(rng);
                        if (victim_id != worker_id) {
                            current_task = queues_[victim_id]->steal();
                            if (current_task) {
                                break;
                            }
                        }
                    }
                }

                // 3. 작업 실행 또는 종료 조건/대기 처리
                if (current_task) {
                    (*current_task)();
                }
                else {
                    if (stop_.load(std::memory_order_acquire)) {
                        // stop 플래그가 켜졌어도 모든 큐가 완전히 빌 때까지 처리 (Drain)
                        bool all_empty = true;
                        for (const auto& q : queues_) {
                            if (!q->empty()) {
                                all_empty = false;
                                break;
                            }
                        }
                        if (all_empty) {
                            break;
                        }
                    }

                    std::unique_lock<std::mutex> lock(cv_mtx_);
                    cv_.wait_for(lock, std::chrono::milliseconds(1), [this, worker_id]() {
                        return stop_.load(std::memory_order_acquire) || !queues_[worker_id]->empty();
                        });
                }
            }
        }

        std::vector<std::unique_ptr<container::work_stealing_deque<task::move_task>>> queues_;
        std::vector<std::thread> workers_;
        std::atomic<bool> stop_;
        const std::size_t thread_count_;

        std::mutex cv_mtx_;
        std::condition_variable cv_;

        static inline thread_local int this_worker_id_ = -1;
    };

} // namespace mino::core::thread
