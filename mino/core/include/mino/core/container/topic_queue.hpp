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

// #include <spdlog/spdlog.h>

namespace mino::core::container {

    struct  message_context {
        std::any data;
        std::chrono::system_clock::time_point published_at;
    };

    enum class queue_state {
        stopped,
        stopping,
        running
    };

    class  topic_queue {
    public:
        using callback_t = std::function<void(const message_context&)>;

    private:
        struct subscriber_channel {
            callback_t callback;
            std::queue<message_context> item_queue;
            std::mutex queue_mutex;
            std::unique_ptr<std::condition_variable> cv;
            std::unique_ptr<std::thread> worker_thread;

            subscriber_channel(callback_t cb);
        };

    public:
        static topic_queue& get_instance();

        // void set_logger(std::shared_ptr<spdlog::logger> logger);

        queue_state get_state() const;
        bool subscribe(const std::string& topic, callback_t callback);
        void start();
        void stop();

        template <typename T>
        bool publish(const std::string& topic, T&& data) {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);

            if (!is_running_) return false;

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
                channel->cv->notify_one();
            }

            return true;
        }

    private:
        topic_queue();
        ~topic_queue();
        topic_queue(const topic_queue&) = delete;
        topic_queue& operator=(const topic_queue&) = delete;

        void worker_loop(subscriber_channel* channel);

        /*
        void log_trace(const std::string& msg);
        void log_debug(const std::string& msg);
        void log_info(const std::string& msg);
        void log_warn(const std::string& msg);
        void log_error(const std::string& msg);
        void log_critical(const std::string& msg);
        //*/

        std::unordered_map<std::string, std::vector<std::unique_ptr<subscriber_channel>>> subscribers_;
        mutable std::shared_mutex state_mutex_;
        std::condition_variable cv_;
        bool is_running_;
        bool is_cleanup_finished_;

        // std::shared_ptr<spdlog::logger> logger_;
    };

} 
