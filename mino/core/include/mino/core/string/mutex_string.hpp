#pragma once

#include <string>
#include <mutex>
#include <utility>
#include <type_traits>
#include <cassert>

#include "mino/core/string/u8.hpp"

namespace mino::core::string {

    // 스레드 안전 문자열 래퍼 클래스
    // - 멤버는 std::string 과 std::mutex 두 가지 뿐입니다.
    // - std::string API를 최대한 동일/유사 시그니처로 제공합니다.
    // - 포인터/이터레이터/참조를 직접 반환하면 위험하므로 guard()나
    //   값 복사(snap: str())를 통해서만 접근하게 합니다.
    class  mutex_string {
    public:
        // 잠금 뷰(guard): 생존 기간 동안 뮤텍스를 보유하며 내부 문자열에
        // 직접 접근할 수 있게 합니다.
        class Locked {
        public:
            // 재진입 검출을 위해 소유자 포인터(대상 객체)를 받습니다.
            Locked(std::string& s, std::mutex& m, const mutex_string* owner);
            Locked(const std::string& s, std::mutex& m, const mutex_string* owner);
            ~Locked(); // 디버그 모드에서 재진입 마크 해제

            // 내부 std::string API 전체를 guard 생존 기간 동안 사용 가능
            std::string* operator->();
            const std::string* operator->() const;
            std::string& operator*();
            const std::string& operator*() const;

            // 조기 unlock 과 보유 여부 확인 (가급적 사용을 지양)
            [[deprecated("특수한 경우 외 unlock() 사용을 지양하세요. "
                "guard 생존 기간을 좁히는 방식으로 구현하세요.")]]
            void unlock();
            bool owns_lock() const;

        protected:
            // 외부에서 내부 포인터를 뽑아가는 오남용 방지용 헬퍼
            const char* guard_cstr() const; // == (cs_ ? cs_ : s_)->c_str()

            // 내부 상태
            std::string* s_ = nullptr;
            const std::string* cs_ = nullptr;
            std::unique_lock<std::mutex> lock_;

#ifndef NDEBUG
            // 디버그 전용 재진입 제어
            const mutex_string* owner_ = nullptr;
            bool mark_set_ = false;
#endif

            friend class mutex_string; // mutex_string 이 내부 접근 허용
        };

        // 내부용 RAII c_str() 가드
        // - get()/operator const char*() 제공
        // - 생존 기간 동안 잠금을 유지하여 안전하게 전달 가능
        class CStrGuard {
        public:
            CStrGuard(const std::string& s, std::mutex& m);
            const char* get() const { return p_; }
            operator const char* () const { return p_; } // 직접 인자로 전달 허용
            CStrGuard(const CStrGuard&) = delete;
            CStrGuard& operator=(const CStrGuard&) = delete;
        private:
            std::unique_lock<std::mutex> lock_;
            const char* p_ = nullptr;
        };

    public:
        // ===== 생성/대입 =====
        mutex_string() = default; // 빈 문자열

        // explicit 제거: "mutex_string ms = \"start\";" 허용
        mutex_string(std::string s);
        mutex_string(const char* s);

        mutex_string(const mutex_string& other);
        mutex_string(mutex_string&& other) noexcept;
        mutex_string& operator=(const mutex_string& other);
        mutex_string& operator=(mutex_string&& other) noexcept;

        // std::string/char* 에서의 대입(쓰기)
        mutex_string& operator=(const std::string& rhs);
        mutex_string& operator=(const char* rhs);

        // 비교(읽기)
        bool operator==(const std::string& rhs) const;
        bool operator==(const char* rhs) const;
        bool operator!=(const std::string& rhs) const { return !(*this == rhs); }
        bool operator!=(const char* rhs) const { return !(*this == rhs); }
        friend inline bool operator==(const std::string& lhs,
            const mutex_string& rhs) {
            return rhs == lhs;
        }
        friend inline bool operator==(const char* lhs,
            const mutex_string& rhs) {
            return rhs == lhs;
        }
        friend inline bool operator!=(const std::string& lhs,
            const mutex_string& rhs) {
            return !(rhs == lhs);
        }
        friend inline bool operator!=(const char* lhs,
            const mutex_string& rhs) {
            return !(rhs == lhs);
        }

        // 사용 예: std::string s2 = static_cast<std::string>(ms);
        explicit operator std::string() const { return str(); }

        // ===== 용량/상태 =====
        std::size_t size() const;
        std::size_t length() const;
        bool empty() const;
        std::size_t capacity() const;
        std::size_t max_size() const;
        void reserve(std::size_t n);
        void shrink_to_fit();

        // ===== 원소 접근(값 반환) + 짧은 setter =====
        // 참고: 참조 반환(operator[], at with char&)은 안전성 문제로 미제공
        char at(std::size_t pos) const;
        char operator[](std::size_t pos) const; // 값 반환
        char front() const;                     // getter
        char back() const;                      // getter

        void set(std::size_t pos, char ch);    // set_at 대체(짧은 이름)
        void front(char ch);                    // setter
        void back(char ch);                     // setter

        // ===== 수정자 =====
        void clear();
        void push_back(char ch);
        void pop_back();

        void assign(const std::string& s);
        void assign(const char* s);
        void assign(std::size_t count, char ch);

        // append (체이닝: mutex_string& 반환)
        mutex_string& append(const std::string& s);
        mutex_string& append(const char* s);
        mutex_string& append(std::size_t count, char ch);

        // operator+=
        mutex_string& operator+=(const std::string& s);
        mutex_string& operator+=(const char* s);
        mutex_string& operator+=(char ch);

        // insert
        mutex_string& insert(std::size_t pos, const std::string& s);
        mutex_string& insert(std::size_t pos, const char* s);
        mutex_string& insert(std::size_t pos, std::size_t count, char ch);

        // erase
        mutex_string& erase(std::size_t pos = 0,
            std::size_t count = std::string::npos);

        // replace
        mutex_string& replace(std::size_t pos, std::size_t count,
            const std::string& s);
        mutex_string& replace(std::size_t pos, std::size_t count,
            const char* s);
        mutex_string& replace(std::size_t pos, std::size_t count,
            std::size_t n, char ch);

        // resize
        void resize(std::size_t n);
        void resize(std::size_t n, char ch);

        // swap
        void swap(mutex_string& other);     // mutex_string 간
        void swap(std::string& other_str);  // 내부 string 과

        // ===== 문자열 연산 =====
        std::string substr(std::size_t pos = 0,
            std::size_t count = std::string::npos) const;
        std::size_t copy(char* dest, std::size_t count,
            std::size_t pos = 0) const;

        int compare(const std::string& s) const;
        int compare(std::size_t pos, std::size_t count,
            const std::string& s) const;

        std::size_t find(const std::string& s, std::size_t pos = 0) const;
        std::size_t find(const char* s, std::size_t pos = 0) const;
        std::size_t find(char ch, std::size_t pos = 0) const;

        std::size_t rfind(const std::string& s,
            std::size_t pos = std::string::npos) const;
        std::size_t rfind(const char* s,
            std::size_t pos = std::string::npos) const;
        std::size_t rfind(char ch,
            std::size_t pos = std::string::npos) const;

        std::size_t find_first_of(const std::string& s,
            std::size_t pos = 0) const;
        std::size_t find_first_of(const char* s,
            std::size_t pos = 0) const;
        std::size_t find_first_of(char ch,
            std::size_t pos = 0) const;

        std::size_t find_last_of(const std::string& s,
            std::size_t pos = std::string::npos) const;
        std::size_t find_last_of(const char* s,
            std::size_t pos = std::string::npos) const;
        std::size_t find_last_of(char ch,
            std::size_t pos = std::string::npos) const;

        std::size_t find_first_not_of(const std::string& s,
            std::size_t pos = 0) const;
        std::size_t find_first_not_of(const char* s,
            std::size_t pos = 0) const;
        std::size_t find_first_not_of(char ch,
            std::size_t pos = 0) const;

        std::size_t find_last_not_of(const std::string& s,
            std::size_t pos =
            std::string::npos) const;
        std::size_t find_last_not_of(const char* s,
            std::size_t pos =
            std::string::npos) const;
        std::size_t find_last_not_of(char ch,
            std::size_t pos =
            std::string::npos) const;

        // ===== 안전 편의 기능 =====
        std::string str() const;

        // with_lock()/with(): 람다를 잠금 범위 내에서 실행
        // 디버그 모드: 동일 객체의 다른 멤버 호출 시 assert 발생
        template <typename Fn>
        auto with_lock(Fn&& f)
            -> decltype(std::forward<Fn>(f)(std::declval<std::string&>())) {
#ifndef NDEBUG
            assert_not_reentrant_();        // 동일 객체 재진입 금지
            ReentrancyMark _rmk{ this };    // with() 생존 기간 동안 마크
#endif
            std::scoped_lock lock(m_);
            return std::forward<Fn>(f)(s_);
        }
        template <typename Fn>
        auto with_lock(Fn&& f) const
            -> decltype(std::forward<Fn>(f)(
                std::declval<const std::string&>())) {
#ifndef NDEBUG
            assert_not_reentrant_();
            ReentrancyMark _rmk{ this };
#endif
            std::scoped_lock lock(m_);
            return std::forward<Fn>(f)(s_);
        }
        template <typename Fn>
        auto with(Fn&& f)
            -> decltype(std::forward<Fn>(f)(std::declval<std::string&>())) {
            return with_lock(std::forward<Fn>(f));
        }
        template <typename Fn>
        auto with(Fn&& f) const
            -> decltype(std::forward<Fn>(f)(
                std::declval<const std::string&>())) {
            return with_lock(std::forward<Fn>(f));
        }

        // 전체 API 접근(이터레이터/포인터 포함)
        [[nodiscard]] Locked synchronize();
        [[nodiscard]] Locked synchronize() const;

        // 단축 별칭
        [[nodiscard]] Locked guard() { return synchronize(); }
        [[nodiscard]] Locked guard() const { return synchronize(); }

    protected:
        // 외부 노출 금지: 오남용 방지를 위해 보호 범위로 둡니다.
        CStrGuard c_str() const;

#ifndef NDEBUG
        // 디버그 전용 재진입 체크/마킹
        // - 정적 thread_local 멤버는 DLL 인터페이스 문제로 금지
        // - 대신 cpp 내부 전역 TLS를 함수로 감쌉니다.
        static const mutex_string* tls_owner() noexcept;
        static void set_tls_owner(const mutex_string* self) noexcept;

        void assert_not_reentrant_() const {
            // 동일 객체의 잠금 구간 내부에서 다시 멤버 호출 금지
            assert(tls_owner() != this &&
                "reentrancy detected: do not call ms.* again inside "
                "with()/guard() scope. Inside with(), only manipulate "
                "the provided std::string(s).");
        }

        struct ReentrancyMark {
            const mutex_string* prev;
            explicit ReentrancyMark(const mutex_string* self);
            ~ReentrancyMark();
        };
#endif

        // 파생 클래스에서 직접 접근 가능한 멤버
        std::string        s_;
        mutable std::mutex m_;
    };

    // 비멤버 swap (ADL)
    inline void swap(mutex_string& a, mutex_string& b) { a.swap(b); }

}  

