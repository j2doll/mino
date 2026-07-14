
#include "mino/core/container/topic_queue.hpp"

namespace mino::core::container {

    topic_queue::subscriber_channel::subscriber_channel(callback_t cb)
        : callback(std::move(cb)), cv(std::make_unique<std::condition_variable>()) {
    }

    topic_queue& topic_queue::get_instance() {
        static topic_queue instance;
        return instance;
    }

    topic_queue::topic_queue()
        : is_running_(false), is_cleanup_finished_(true)
        // , logger_(spdlog::default_logger())
    {
    }

    topic_queue::~topic_queue() {
        stop();
    }

    /*
    void topic_queue::set_logger(std::shared_ptr<spdlog::logger> logger) {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        if (logger) {
            logger_ = logger;
        }
    }

    void topic_queue::log_trace(const std::string& msg) {
        if (logger_) logger_->trace(msg);
    }

    void topic_queue::log_debug(const std::string& msg) {
        if (logger_) logger_->debug(msg);
    }

    void topic_queue::log_info(const std::string& msg) {
        if (logger_) logger_->info(msg);
    }

    void topic_queue::log_warn(const std::string& msg) {
        if (logger_) logger_->warn(msg);
    }

    void topic_queue::log_error(const std::string& msg) {
        if (logger_) logger_->error(msg);
    }

    void topic_queue::log_critical(const std::string& msg) {
        if (logger_) logger_->critical(msg);
    }
    //*/

    queue_state topic_queue::get_state() const {
        std::shared_lock<std::shared_mutex> lock(state_mutex_);
        if (is_running_) return queue_state::running;
        if (!is_cleanup_finished_) return queue_state::stopping;
        return queue_state::stopped;
    }

    bool topic_queue::subscribe(const std::string& topic, callback_t callback) {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        if (is_running_ || !is_cleanup_finished_) {
            // 💡 영어 로그로 변경
            // log_warn("Subscription failed: Queue must be in STOPPED state to add a subscriber.");
            return false;
        }

        if (!callback) return false;

        subscribers_[topic].push_back(std::make_unique<subscriber_channel>(callback));
        return true;
    }

    void topic_queue::start() {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        if (is_running_) return;

        this->is_running_ = true;
        this->is_cleanup_finished_ = false;

        int thread_count = 0;
        for (auto& [topic, channel_list] : subscribers_) {
            for (auto& channel : channel_list) {
                channel->worker_thread = std::make_unique<std::thread>(
                    &topic_queue::worker_loop, this, channel.get()
                );
                thread_count++;
            }
        }

        // log_info(">>> Topic Queue System Started (Total worker threads: " + std::to_string(thread_count) + ") <<<");
    }

    void topic_queue::stop() {
        {
            std::unique_lock<std::shared_mutex> lock(state_mutex_);
            if (!is_running_) return;
            is_running_ = false;
        }

        for (auto& [topic, channel_list] : subscribers_) {
            for (auto& channel : channel_list) {
                channel->cv->notify_all();
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
            is_cleanup_finished_ = true;
        }

        // log_info(">>> Topic Queue System Stopped Successfully <<<");
    }

    void topic_queue::worker_loop(subscriber_channel* channel) {
        while (true) {
            message_context ctx;
            bool has_data = false;

            {
                std::unique_lock<std::mutex> lock(channel->queue_mutex);

                channel->cv->wait(lock, [this, channel]() {
                    return !is_running_ || !channel->item_queue.empty();
                    });

                if (!is_running_ && channel->item_queue.empty()) {
                    break;
                }

                if (!channel->item_queue.empty()) {
                    ctx = std::move(channel->item_queue.front());
                    channel->item_queue.pop();
                    has_data = true;
                }
            }

            if (has_data) {
                channel->callback(ctx);
            }
        }
    }

} // namespace j2::container
