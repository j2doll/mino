#pragma once

#include <string>
#include <string_view>
#include <cstddef>

#if defined(_WIN32) || defined(_WIN64)
#   define OS_WINDOWS
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#elif defined(__linux__)
#   define OS_LINUX
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <pthread.h>
#endif

namespace mino::core::shared_memory {

    class  shared_memory {
    public:
        shared_memory() = default;
        ~shared_memory() noexcept;

        shared_memory(const shared_memory&) = delete;
        shared_memory& operator=(const shared_memory&) = delete;

        shared_memory(shared_memory&& other) noexcept;
        shared_memory& operator=(shared_memory&& other) noexcept;

        bool initialize(std::string_view name, std::size_t size) noexcept;

        bool create() noexcept;
        bool open() noexcept;
        void close() noexcept;

        bool lock_shared() noexcept;
        void unlock_shared() noexcept;

        bool lock_exclusive() noexcept;
        void unlock_exclusive() noexcept;

        [[nodiscard]] void* get_address() const noexcept;
        [[nodiscard]] std::size_t get_size() const noexcept { return size_; }
        [[nodiscard]] bool is_attached() const noexcept { return mapped_address_ != nullptr; }

    private:
        std::string name_;
        std::size_t size_ = 0;
        void* mapped_address_ = nullptr;

#if defined(OS_WINDOWS)
        HANDLE shm_handle_ = nullptr;
        HANDLE mutex_exclusive_ = nullptr;
        std::wstring wname_;
        [[nodiscard]] std::wstring to_wstring(std::string_view str) noexcept;
#elif defined(OS_LINUX)
        int shm_fd_ = -1;
        std::string clean_shm_name_;

        struct shm_header {
            pthread_rwlock_t rwlock;
        };
        shm_header* header_ = nullptr;
        [[nodiscard]] std::string normalize_name(std::string_view str, std::string_view prefix) noexcept;
#endif
    };

    class  shm_shared_guard {
    public:
        explicit shm_shared_guard(shared_memory& shm) noexcept : shm_(shm), is_locked_(shm.lock_shared()) {}
        ~shm_shared_guard() noexcept { if (is_locked_) shm_.unlock_shared(); }
        [[nodiscard]] bool owns_lock() const noexcept { return is_locked_; }
    private:
        shared_memory& shm_;
        bool is_locked_;
    };

    class  shm_exclusive_guard {
    public:
        explicit shm_exclusive_guard(shared_memory& shm) noexcept : shm_(shm), is_locked_(shm.lock_exclusive()) {}
        ~shm_exclusive_guard() noexcept { if (is_locked_) shm_.unlock_exclusive(); }
        [[nodiscard]] bool owns_lock() const noexcept { return is_locked_; }
    private:
        shared_memory& shm_;
        bool is_locked_;
    };

} 