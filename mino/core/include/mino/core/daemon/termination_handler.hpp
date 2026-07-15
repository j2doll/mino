#pragma once

#include <functional>

namespace mino::core::daemon {

    class  termination_handler {
    public:
        using callback_t = std::function<void()>;

        // Returns the singleton instance
        static termination_handler& get_instance();

        // Registers the callback function to be executed upon termination
        void set_callback(callback_t callback);

        // Initializes and registers the OS-specific signal/console handlers
        void initialize();

        // Delete copy and move operations
        termination_handler(const termination_handler&) = delete;
        termination_handler& operator=(const termination_handler&) = delete;

    private:
        termination_handler() = default;
        ~termination_handler() = default;

        static void execute_callback();

        callback_t user_callback_ = nullptr;

#if defined(_WIN32) || defined(_WIN64)
        static int __stdcall windows_handler(unsigned long ctrl_type);
#else
        static void linux_handler(int signum);
#endif
    };

} // namespace mino::core::daemon