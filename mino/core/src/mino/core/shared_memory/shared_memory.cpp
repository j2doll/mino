#include <utility>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "mino/core/shared_memory/shared_memory.hpp"

namespace mino::core::shared_memory {
 
    shared_memory::~shared_memory() noexcept {
        close();
    }

    bool shared_memory::initialize(std::string_view name, std::size_t size) noexcept {
        close();
        if (name.empty() || size == 0) return false;

        try {
            name_ = name;
#if defined(OS_WINDOWS)
            size_ = size;
            wname_ = to_wstring(name_);
#elif defined(OS_LINUX)
            size_ = size + sizeof(shm_header);
            clean_shm_name_ = normalize_name(name_, "/shm_");
#endif
            return true;
        }
        catch (...) {
            return false;
        }
    }

    shared_memory::shared_memory(shared_memory&& other) noexcept
        : name_(std::move(other.name_)), size_(other.size_), mapped_address_(other.mapped_address_) {
        other.mapped_address_ = nullptr;
#if defined(OS_WINDOWS)
        shm_handle_ = other.shm_handle_;
        mutex_exclusive_ = other.mutex_exclusive_;
        wname_ = std::move(other.wname_);
        other.shm_handle_ = nullptr;
        other.mutex_exclusive_ = nullptr;
#elif defined(OS_LINUX)
        shm_fd_ = other.shm_fd_;
        clean_shm_name_ = std::move(other.clean_shm_name_);
        header_ = other.header_;
        other.shm_fd_ = -1;
        other.header_ = nullptr;
#endif
    }

    shared_memory& shared_memory::operator=(shared_memory&& other) noexcept {
        if (this != &other) {
            close();
            try {
                name_ = std::move(other.name_);
                size_ = other.size_;
                mapped_address_ = other.mapped_address_;
                other.mapped_address_ = nullptr;
#if defined(OS_WINDOWS)
                shm_handle_ = other.shm_handle_;
                mutex_exclusive_ = other.mutex_exclusive_;
                wname_ = std::move(other.wname_);
                other.shm_handle_ = nullptr;
                other.mutex_exclusive_ = nullptr;
#elif defined(OS_LINUX)
                shm_fd_ = other.shm_fd_;
                clean_shm_name_ = std::move(other.clean_shm_name_);
                header_ = other.header_;
                other.shm_fd_ = -1;
                other.header_ = nullptr;
#endif
            }
            catch (...) {}
        }
        return *this;
    }

    bool shared_memory::create() noexcept {
        if (name_.empty() || size_ == 0) return false;
        close();

#if defined(OS_WINDOWS)
        std::wstring win_shm_name = L"Local\\SHM_" + wname_;
        std::wstring win_ex_name = L"Local\\EX_" + wname_;

        mutex_exclusive_ = CreateMutexW(nullptr, FALSE, win_ex_name.c_str());
        if (!mutex_exclusive_) return false;

        shm_handle_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            static_cast<DWORD>((size_ >> 32) & 0xFFFFFFFF),
            static_cast<DWORD>(size_ & 0xFFFFFFFF), win_shm_name.c_str());
        if (!shm_handle_) {
            CloseHandle(mutex_exclusive_); mutex_exclusive_ = nullptr;
            return false;
        }
        mapped_address_ = MapViewOfFile(shm_handle_, FILE_MAP_ALL_ACCESS, 0, 0, size_);
        if (!mapped_address_) {
            close();
            return false;
        }
        return true;

#elif defined(OS_LINUX)
        shm_fd_ = shm_open(clean_shm_name_.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if (shm_fd_ == -1) return false;

        if (ftruncate(shm_fd_, static_cast<off_t>(size_)) == -1) {
            ::close(shm_fd_); shm_fd_ = -1;
            return false;
        }
        mapped_address_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
        if (mapped_address_ == MAP_FAILED) {
            mapped_address_ = nullptr; ::close(shm_fd_); shm_fd_ = -1;
            return false;
        }

        header_ = static_cast<shm_header*>(mapped_address_);

        pthread_rwlockattr_t attr;
        pthread_rwlockattr_init(&attr);
        pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_rwlock_init(&(header_->rwlock), &attr);
        pthread_rwlockattr_destroy(&attr);
        return true;
#endif
    }

    bool shared_memory::open() noexcept {
        if (name_.empty() || size_ == 0) return false;
        close();

#if defined(OS_WINDOWS)
        std::wstring win_shm_name = L"Local\\SHM_" + wname_;
        std::wstring win_ex_name = L"Local\\EX_" + wname_;

        mutex_exclusive_ = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, win_ex_name.c_str());
        if (!mutex_exclusive_) return false;

        shm_handle_ = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, win_shm_name.c_str());
        if (!shm_handle_) { CloseHandle(mutex_exclusive_); mutex_exclusive_ = nullptr; return false; }
        mapped_address_ = MapViewOfFile(shm_handle_, FILE_MAP_ALL_ACCESS, 0, 0, size_);
        return mapped_address_ != nullptr;

#elif defined(OS_LINUX)
        shm_fd_ = shm_open(clean_shm_name_.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
        if (shm_fd_ == -1) return false;

        mapped_address_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
        if (mapped_address_ == MAP_FAILED) { mapped_address_ = nullptr; ::close(shm_fd_); shm_fd_ = -1; return false; }
        header_ = static_cast<shm_header*>(mapped_address_);
        return true;
#endif
    }

    void shared_memory::close() noexcept {
        if (mapped_address_) {
#if defined(OS_WINDOWS)
            UnmapViewOfFile(mapped_address_);
#elif defined(OS_LINUX)
            munmap(mapped_address_, size_);
#endif
            mapped_address_ = nullptr;
        }
#if defined(OS_WINDOWS)
        if (shm_handle_) { CloseHandle(shm_handle_); shm_handle_ = nullptr; }
        if (mutex_exclusive_) { CloseHandle(mutex_exclusive_); mutex_exclusive_ = nullptr; }
#elif defined(OS_LINUX)
        if (shm_fd_ != -1) { ::close(shm_fd_); shm_fd_ = -1; shm_unlink(clean_shm_name_.c_str()); }
#endif
    }

    bool shared_memory::lock_shared() noexcept {
#if defined(OS_WINDOWS)
        if (!mutex_exclusive_) return false;
        return WaitForSingleObject(mutex_exclusive_, INFINITE) == WAIT_OBJECT_0;
#elif defined(OS_LINUX)
        if (!header_) return false;
        return pthread_rwlock_rdlock(&(header_->rwlock)) == 0;
#endif
    }

    void shared_memory::unlock_shared() noexcept {
#if defined(OS_WINDOWS)
        if (mutex_exclusive_) ReleaseMutex(mutex_exclusive_);
#elif defined(OS_LINUX)
        if (header_) pthread_rwlock_unlock(&(header_->rwlock));
#endif
    }

    bool shared_memory::lock_exclusive() noexcept {
#if defined(OS_WINDOWS)
        if (!mutex_exclusive_) return false;
        return WaitForSingleObject(mutex_exclusive_, INFINITE) == WAIT_OBJECT_0;
#elif defined(OS_LINUX)
        if (!header_) return false;
        return pthread_rwlock_wrlock(&(header_->rwlock)) == 0;
#endif
    }

    void shared_memory::unlock_exclusive() noexcept {
        unlock_shared();
    }

    void* shared_memory::get_address() const noexcept {
#if defined(OS_WINDOWS)
        return mapped_address_;
#elif defined(OS_LINUX)
        if (!mapped_address_) return nullptr;
        return static_cast<char*>(mapped_address_) + sizeof(shm_header);
#endif
    }

#if defined(OS_WINDOWS)
    std::wstring shared_memory::to_wstring(std::string_view str) noexcept {
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(
            CP_UTF8,
            0,
            str.data(),
            static_cast<int>(str.size()),
            nullptr,
            0);
        try {
            std::wstring wstr_to(size_needed, 0);
            MultiByteToWideChar(
                CP_UTF8,
                0,
                str.data(),
                static_cast<int>(str.size()),
                &wstr_to[0],
                size_needed);
            return wstr_to;
        }
        catch (...) {
            return L"";
        }
    }
#elif defined(OS_LINUX)
    std::string shared_memory::normalize_name(std::string_view str, std::string_view prefix) noexcept {
        try {
            std::string res(prefix);
            if (!str.empty() && str[0] == '/') res.append(str.substr(1));
            else res.append(str);
            return res;
        }
        catch (...) {
            return "/shm_default_fallback";
        }
    }
#endif

}