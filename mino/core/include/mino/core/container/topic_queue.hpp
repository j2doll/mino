#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <any>
#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>

#include "mino/core/log/tinylog/tinylog_fwd.hpp"
#include "mino/core/log/tinylog/tinylog.hpp"

// ============+====================+====================+==================
// 항목        |  concurrent_queue  |  topic_queue       |  Message Broker
// ------------+--------------------+--------------------+------------------
// 통신 모델   | 점대점 (P2P Queue) | 발행-구독 (Pub-Sub)| 분산 네트워크 Pub-Sub
// 스레드 구조 | 외부 작업자 풀     | 구독자별 전용 워커 | 외부 브로커 프로세스
// 데이터 타입 | 단일 고정 타입 (T) | 이기종 데이터(Any) | 바이트 스트림 직렬화
// ------------+--------------------+--------------------+------------------
// 토픽 필터링 | 미지원             | 토픽 문자열 라우팅 | 토픽 와일드카드 매칭
// 동기화 방식 | 뮤텍스/CV 동기화   | 채널별 독립 락/CV  | 소켓/IPC 비동기 I/O
// ------------+--------------------+--------------------+------------------
// 실무 추천   | 단일 생산자-소비자 | 프로세스 내부 모듈 | 프로세스/네트워크 간
//             | 작업 큐            | 간 비동기 이벤트   | 대규모 메시지 버스
// ============+====================+====================+==================
//
// topic_queue는 토픽(Topic) 기반의 비동기 1:N 메시지 발행/구독(Pub-Sub) 시스템으로,
// 각 구독자마다 독립된 작업 큐와 전용 워커 스레드를 할당하여 병렬 처리를 보장합니다.
//
// auto& tq = topic_queue::get_instance();
// 
// // [1] 구독자 등록 (Stopped 상태에서만 등록 가능)
// tq.subscribe("sensor/temp", [](const message_context& ctx) {
//     int temp = std::any_cast<int>(ctx.data);
//     std::cout << "[Temp Subscriber] " << temp << " C" << std::endl;
// });
// 
// // [2] 큐 시스템 시작 (각 구독자별 워커 스레드 생성)
// tq.start();
// 
// // [3] 메시지 발행 (비동기 병렬 분배)
// tq.publish("sensor/temp", 25);
// tq.publish("sensor/temp", 28);
// 
// // [4] 상태 확인 및 덤프
// tq.dump("Running State");
// 
// // [5] 큐 시스템 종료 (대기 메시지 처리 후 스레드 안전 종료)
// tq.stop();
// 

namespace mino::core::container {

    struct message_context {
        std::any data;
        std::chrono::system_clock::time_point published_at;
    };

    enum class queue_state {
        stopped,
        stopping,
        running
    };

    class topic_queue {
    public:
        using callback_t = std::function<void(const message_context&)>;

    protected:
        struct subscriber_channel {
            callback_t callback;
            std::queue<message_context> item_queue;
            std::mutex queue_mutex;
            std::condition_variable cv;
            std::unique_ptr<std::thread> worker_thread;

            explicit subscriber_channel(callback_t cb)
                : callback(std::move(cb)) {
            }
        };

    public:
        static topic_queue& get_instance() {
            static topic_queue instance;
            return instance;
        }

        void set_logger(std::shared_ptr<mino::core::log::tinylog::logger> logger) {
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            if (logger) {
                logger_ = logger;
            }
        }

        queue_state get_state() const {
            if (is_running_.load(std::memory_order_acquire))
                return queue_state::running;
            if (!is_cleanup_finished_.load(std::memory_order_acquire))
                return queue_state::stopping;
            return queue_state::stopped;
        }

        bool subscribe(const std::string& topic, callback_t callback) {
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            if (is_running_.load(std::memory_order_relaxed) || !is_cleanup_finished_.load(std::memory_order_relaxed)) {
                log_warn("Subscription failed: Queue must be in STOPPED state to add a subscriber.");
                return false;
            }

            if (!callback)
                return false;

            subscribers_[topic].push_back(std::make_unique<subscriber_channel>(std::move(callback)));
            return true;
        }

        void start() {
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            if (is_running_.load(std::memory_order_relaxed))
                return;

            is_running_.store(true, std::memory_order_release);
            is_cleanup_finished_.store(false, std::memory_order_release);

            int thread_count = 0;
            for (auto& [topic, channel_list] : subscribers_) {
                for (auto& channel : channel_list) {
                    channel->worker_thread = std::make_unique<std::thread>(
                        &topic_queue::worker_loop, this, channel.get()
                    );
                    thread_count++;
                }
            }

            log_info(">>> Topic Queue System Started (Total worker threads: " + std::to_string(thread_count) + ") <<<");
        }

        void stop() {
            {
                std::unique_lock<std::shared_mutex> lock(state_mutex_);
                if (!is_running_.load(std::memory_order_relaxed)) return;
                is_running_.store(false, std::memory_order_release);
            }

            for (auto& [topic, channel_list] : subscribers_) {
                for (auto& channel : channel_list) {
                    channel->cv.notify_all();
                }
            }

            for (auto& [topic, channel_list] : subscribers_) {
                for (auto& channel : channel_list) {
                    if (channel->worker_thread && channel->worker_thread->joinable()) {
                        channel->worker_thread->join();
                        channel->worker_thread.reset();
                    }
                }
            }

            {
                std::unique_lock<std::shared_mutex> lock(state_mutex_);
                is_cleanup_finished_.store(true, std::memory_order_release);
            }

            log_info(">>> Topic Queue System Stopped Successfully <<<");
        }

        template <typename T>
        bool publish(const std::string& topic, T&& data) {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);

            if (!is_running_.load(std::memory_order_relaxed)) return false;

            auto it = subscribers_.find(topic);
            if (it == subscribers_.end() || it->second.empty()) {
                return true;
            }

            message_context ctx;
            ctx.data = std::forward<T>(data);
            ctx.published_at = std::chrono::system_clock::now();

            for (auto& channel : it->second) {
                {
                    std::unique_lock<std::mutex> lock(channel->queue_mutex);
                    channel->item_queue.push(ctx);
                }
                channel->cv.notify_one();
            }

            return true;
        }

        // ==========================================
        // 순수 ASCII 기반 Topic Queue Dump
        // ==========================================
        void dump(const std::string& title = "") const {
            std::shared_lock<std::shared_mutex> lock(state_mutex_);

            std::string state_str;
            switch (get_state()) {
            case queue_state::running:  state_str = "RUNNING"; break;
            case queue_state::stopping: state_str = "STOPPING"; break;
            case queue_state::stopped:  state_str = "STOPPED"; break;
            }

            if (!title.empty()) {
                std::cout << "=== " << title << " (State: " << state_str << ", Topics: " << subscribers_.size() << ") ===\n";
            }
            else {
                std::cout << "=== Topic Queue Dump (State: " << state_str << ", Topics: " << subscribers_.size() << ") ===\n";
            }

            if (subscribers_.empty()) {
                std::cout << "  \\-- <No Subscribed Topics>\n\n";
                return;
            }

            size_t topic_idx = 0;
            size_t total_topics = subscribers_.size();
            for (const auto& [topic, channel_list] : subscribers_) {
                bool is_last_topic = (++topic_idx == total_topics);
                std::string t_connector = is_last_topic ? "\\-- " : "|-- ";
                std::cout << t_connector << "Topic: [" << topic << "] (Subscribers: " << channel_list.size() << ")\n";

                std::string sub_prefix = is_last_topic ? "    " : "|   ";
                for (size_t i = 0; i < channel_list.size(); ++i) {
                    bool is_last_sub = (i == channel_list.size() - 1);
                    std::string s_connector = is_last_sub ? "\\-- " : "|-- ";

                    size_t pending = 0;
                    {
                        std::unique_lock<std::mutex> q_lock(channel_list[i]->queue_mutex);
                        pending = channel_list[i]->item_queue.size();
                    }

                    bool active = (channel_list[i]->worker_thread != nullptr);
                    std::cout << sub_prefix << s_connector << "Subscriber #" << (i + 1)
                        << " | Pending Items: " << pending
                        << " | Worker: " << (active ? "ACTIVE" : "IDLE") << "\n";
                }
            }
            std::cout << "\n";
        }

    protected:
        topic_queue()
            : is_running_(false)
            , is_cleanup_finished_(true)
            , logger_(nullptr) {
        }

        ~topic_queue() {
            stop();
        }

        topic_queue(const topic_queue&) = delete;
        topic_queue& operator=(const topic_queue&) = delete;

        void worker_loop(subscriber_channel* channel) {
            while (true) {
                message_context ctx;
                bool has_data = false;

                {
                    std::unique_lock<std::mutex> lock(channel->queue_mutex);

                    channel->cv.wait(lock, [this, channel]() {
                        return !is_running_.load(std::memory_order_relaxed) || !channel->item_queue.empty();
                        });

                    if (!is_running_.load(std::memory_order_relaxed) && channel->item_queue.empty()) {
                        break;
                    }

                    if (!channel->item_queue.empty()) {
                        ctx = std::move(channel->item_queue.front());
                        channel->item_queue.pop();
                        has_data = true;
                    }
                }

                if (has_data) {
                    try {
                        channel->callback(ctx);
                    }
                    catch (const std::exception& e) {
                        log_error("Exception in subscriber callback: " + std::string(e.what()));
                    }
                    catch (...) {
                        log_error("Unknown exception in subscriber callback.");
                    }
                }
            }
        }

        void log_trace(const std::string& msg) {
            if (logger_) logger_->trace(msg);
        }
        void log_debug(const std::string& msg) {
            if (logger_) logger_->debug(msg);
        }
        void log_info(const std::string& msg) {
            if (logger_) logger_->info(msg);
        }
        void log_warn(const std::string& msg) {
            if (logger_) logger_->warn(msg);
        }
        void log_error(const std::string& msg) {
            if (logger_) logger_->error(msg);
        }
        void log_critical(const std::string& msg) {
            if (logger_) logger_->critical(msg);
        }

    private:
        std::unordered_map<std::string, std::vector<std::unique_ptr<subscriber_channel>>> subscribers_;
        mutable std::shared_mutex state_mutex_;
        std::atomic<bool> is_running_;
        std::atomic<bool> is_cleanup_finished_;
        std::shared_ptr<mino::core::log::tinylog::logger> logger_;
    };

} // namespace mino::core::container

