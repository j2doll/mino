#include "mino/core/string/mutex_string.hpp"

namespace mino::core::string {

#ifndef NDEBUG
    namespace {
        // 클래스 밖 내부 전역 TLS 저장소
        // - 클래스에 DLL 인터페이스가 붙어도 이 변수는 내보내지 않음
        thread_local const mino::core::string::mutex_string* g_tls_owner = nullptr;
    }

    const mutex_string* mutex_string::tls_owner() noexcept {
        return g_tls_owner;
    }
    void mutex_string::set_tls_owner(const mutex_string* self) noexcept {
        g_tls_owner = self;
    }

    mutex_string::ReentrancyMark::ReentrancyMark(const mutex_string* self)
        : prev(g_tls_owner) {
        // 동일 객체에 대한 재진입 방지
        assert(g_tls_owner != self && "no reentrancy for same object");
        g_tls_owner = self;
    }
    mutex_string::ReentrancyMark::~ReentrancyMark() {
        g_tls_owner = prev;
    }
#endif // NDEBUG

    // ================= Locked 구현 =================
    mutex_string::Locked::Locked(std::string& s,
        std::mutex& m,
        const mutex_string* owner)
        : s_(&s)
        , lock_(m)                 // 잠금을 먼저 보유
#ifndef NDEBUG
        , owner_(owner)
#endif
    {
#ifndef NDEBUG
        // guard 생존 기간 동안 동일 객체의 다른 멤버 호출을 금지
        assert(mutex_string::tls_owner() != owner_ &&
            "no reentrancy on same object (calling ms.* while guard "
            "is held)");
        mutex_string::set_tls_owner(owner_);
        mark_set_ = true;
#endif
    }

    mutex_string::Locked::Locked(const std::string& s,
        std::mutex& m,
        const mutex_string* owner)
        : cs_(&s)
        , lock_(m)
#ifndef NDEBUG
        , owner_(owner)
#endif
    {
#ifndef NDEBUG
        assert(mutex_string::tls_owner() != owner_ &&
            "no reentrancy on same object (calling ms.* while guard "
            "is held)");
        mutex_string::set_tls_owner(owner_);
        mark_set_ = true;
#endif
    }

    mutex_string::Locked::~Locked() {
#ifndef NDEBUG
        if (mark_set_ && mutex_string::tls_owner() == owner_) {
            mutex_string::set_tls_owner(nullptr);
        }
#endif
    }

    std::string* mutex_string::Locked::operator->() { return s_; }
    const std::string* mutex_string::Locked::operator->() const {
        return cs_ ? cs_ : s_;
    }
    std::string& mutex_string::Locked::operator*() { return *s_; }
    const std::string& mutex_string::Locked::operator*() const {
        return cs_ ? *cs_ : *s_;
    }

    void mutex_string::Locked::unlock() {
        lock_.unlock();
#ifndef NDEBUG
        // 조기 해제 시, 더 이상 소유 마크 유지하지 않음
        if (mark_set_ && mutex_string::tls_owner() == owner_) {
            mutex_string::set_tls_owner(nullptr);
            mark_set_ = false;
        }
#endif
    }
    bool mutex_string::Locked::owns_lock() const { return lock_.owns_lock(); }

    // 보호 메서드: guard 생존 기간 동안에만 안전
    const char* mutex_string::Locked::guard_cstr() const {
        return (cs_ ? cs_ : s_)->c_str();
    }

    // ================= CStrGuard 구현 =================
    mutex_string::CStrGuard::CStrGuard(const std::string& s, std::mutex& m)
        : lock_(m), p_(s.c_str()) {
        // 주의: p_는 내부 버퍼 포인터이므로, 잠금이 살아있는
        // CStrGuard 생존 기간 동안에만 안전합니다.
    }

    // ================= mutex_string 핵심 구현 =================
    mutex_string::mutex_string(std::string s) : s_(std::move(s)) {
        // 플랫폼/라이브러리 차이로 초기 capacity가 크게 나오는 경우가 있어서
        // 일관된 동작을 위해 내부 버퍼를 임시 std::string과 스왑해 최소 용량으로 만듭니다.
        std::string(s_).swap(s_);
    }
    mutex_string::mutex_string(const char* s) : s_(s ? s : "") {
        // 동일한 이유로 최소 용량 확보
        std::string(s_).swap(s_);
    }

    mutex_string::mutex_string(const mutex_string& other) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(other.m_);
        s_ = other.s_;
    }
    mutex_string::mutex_string(mutex_string&& other) noexcept {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(other.m_);
        s_ = std::move(other.s_);
    }

    mutex_string& mutex_string::operator=(const mutex_string& other) {
        if (this != &other) {
#ifndef NDEBUG
            assert_not_reentrant_();
#endif
            std::scoped_lock lock(m_, other.m_);
            s_ = other.s_;
        }
        return *this;
    }

    mutex_string& mutex_string::operator=(mutex_string&& other) noexcept {
        if (this != &other) {
#ifndef NDEBUG
            assert_not_reentrant_();
#endif
            std::scoped_lock lock(m_, other.m_);
            s_ = std::move(other.s_);
        }
        return *this;
    }

    // ===== std::string/char* 대입 =====
    mutex_string& mutex_string::operator=(const std::string& rhs) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ = rhs;
        return *this;
    }
    mutex_string& mutex_string::operator=(const char* rhs) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ = (rhs ? rhs : "");
        return *this;
    }

    // ===== 비교 =====
    bool mutex_string::operator==(const std::string& rhs) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_ == rhs;
    }
    bool mutex_string::operator==(const char* rhs) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_ == (rhs ? rhs : "");
    }

    // ===== 용량/상태 =====
    std::size_t mutex_string::size() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.size();
    }
    std::size_t mutex_string::length() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.length();
    }
    bool mutex_string::empty() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.empty();
    }
    std::size_t mutex_string::capacity() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.capacity();
    }
    std::size_t mutex_string::max_size() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.max_size();
    }
    void mutex_string::reserve(std::size_t n) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.reserve(n);
    }
    void mutex_string::shrink_to_fit() {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        // std::string::shrink_to_fit()는 구현마다 동작이 다르므로,
        // 일관된 최소화 동작을 위해 임시 문자열과 스왑합니다.
        std::string(s_).swap(s_);
    }

    // ===== 원소 접근(값 반환) + setter =====
    char mutex_string::at(std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.at(pos);
    }
    char mutex_string::operator[](std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_[pos];
    }
    char mutex_string::front() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.front();
    }
    char mutex_string::back() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.back();
    }
    void mutex_string::set(std::size_t pos, char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.at(pos) = ch;
    }
    void mutex_string::front(char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.front() = ch;
    }
    void mutex_string::back(char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.back() = ch;
    }

    // ===== 수정자 =====
    void mutex_string::clear() {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.clear();
    }
    void mutex_string::push_back(char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.push_back(ch);
    }
    void mutex_string::pop_back() {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.pop_back();
    }

    void mutex_string::assign(const std::string& s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ = s;
    }
    void mutex_string::assign(const char* s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ = (s ? s : "");
    }
    void mutex_string::assign(std::size_t count, char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.assign(count, ch);
    }

    mutex_string& mutex_string::append(const std::string& s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.append(s);
        return *this;
    }
    mutex_string& mutex_string::append(const char* s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.append(s ? s : "");
        return *this;
    }
    mutex_string& mutex_string::append(std::size_t count, char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.append(count, ch);
        return *this;
    }

    mutex_string& mutex_string::operator+=(const std::string& s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ += s;
        return *this;
    }
    mutex_string& mutex_string::operator+=(const char* s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ += (s ? s : "");
        return *this;
    }
    mutex_string& mutex_string::operator+=(char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_ += ch;
        return *this;
    }

    mutex_string& mutex_string::insert(std::size_t pos, const std::string& s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.insert(pos, s);
        return *this;
    }
    mutex_string& mutex_string::insert(std::size_t pos, const char* s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.insert(pos, s ? s : "");
        return *this;
    }
    mutex_string& mutex_string::insert(std::size_t pos,
        std::size_t count,
        char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.insert(pos, count, ch);
        return *this;
    }

    mutex_string& mutex_string::erase(std::size_t pos, std::size_t count) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.erase(pos, count);
        return *this;
    }

    mutex_string& mutex_string::replace(std::size_t pos, std::size_t count,
        const std::string& s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.replace(pos, count, s);
        return *this;
    }
    mutex_string& mutex_string::replace(std::size_t pos, std::size_t count,
        const char* s) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.replace(pos, count, s ? s : "");
        return *this;
    }
    mutex_string& mutex_string::replace(std::size_t pos, std::size_t count,
        std::size_t n, char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.replace(pos, count, n, ch);
        return *this;
    }

    void mutex_string::resize(std::size_t n) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.resize(n);
    }
    void mutex_string::resize(std::size_t n, char ch) {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        s_.resize(n, ch);
    }

    void mutex_string::swap(mutex_string& other) {
        if (this == &other) return;
#ifndef NDEBUG
        assert_not_reentrant_();
        other.assert_not_reentrant_();
#endif
        mutex_string* first = this < &other ? this : &other;
        mutex_string* second = this < &other ? &other : this;
        std::scoped_lock lock(first->m_, second->m_);
        s_.swap(other.s_);
    }
    void mutex_string::swap(std::string& other_str) {
#ifndef NDEBUG
        assert_not_reentrant_(); // 공유 문자열이면 별도 동기화 필요
#endif
        std::scoped_lock lock(m_);
        s_.swap(other_str);
    }

    // ===== 문자열 연산 =====
    std::string mutex_string::substr(std::size_t pos,
        std::size_t count) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.substr(pos, count);
    }
    std::size_t mutex_string::copy(char* dest, std::size_t count,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.copy(dest, count, pos);
    }
    int mutex_string::compare(const std::string& s) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.compare(s);
    }
    int mutex_string::compare(std::size_t pos, std::size_t count,
        const std::string& s) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.compare(pos, count, s);
    }

    std::size_t mutex_string::find(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find(s, pos);
    }
    std::size_t mutex_string::find(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find(s ? s : "", pos);
    }
    std::size_t mutex_string::find(char ch, std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find(ch, pos);
    }

    std::size_t mutex_string::rfind(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.rfind(s, pos);
    }
    std::size_t mutex_string::rfind(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.rfind(s ? s : "", pos);
    }
    std::size_t mutex_string::rfind(char ch, std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.rfind(ch, pos);
    }

    std::size_t mutex_string::find_first_of(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_of(s, pos);
    }
    std::size_t mutex_string::find_first_of(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_of(s ? s : "", pos);
    }
    std::size_t mutex_string::find_first_of(char ch,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_of(std::string(1, ch), pos);
    }

    std::size_t mutex_string::find_last_of(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_of(s, pos);
    }
    std::size_t mutex_string::find_last_of(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_of(s ? s : "", pos);
    }
    std::size_t mutex_string::find_last_of(char ch,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_of(std::string(1, ch), pos);
    }

    std::size_t mutex_string::find_first_not_of(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_not_of(s, pos);
    }
    std::size_t mutex_string::find_first_not_of(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_not_of(s ? s : "", pos);
    }
    std::size_t mutex_string::find_first_not_of(char ch,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_first_not_of(std::string(1, ch), pos);
    }

    std::size_t mutex_string::find_last_not_of(const std::string& s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_not_of(s, pos);
    }
    std::size_t mutex_string::find_last_not_of(const char* s,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_not_of(s ? s : "", pos);
    }
    std::size_t mutex_string::find_last_not_of(char ch,
        std::size_t pos) const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_.find_last_not_of(std::string(1, ch), pos);
    }

    // ===== 안전 편의 =====
    std::string mutex_string::str() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        std::scoped_lock lock(m_);
        return s_;
    }

    // ===== 전체 API 접근 =====
    mutex_string::Locked mutex_string::synchronize() {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        return Locked{ s_, m_, this };
    }
    mutex_string::Locked mutex_string::synchronize() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        return Locked{ s_, m_, this };
    }

    // ===== 보호: RAII c_str() (외부 미노출) =====
    mutex_string::CStrGuard mutex_string::c_str() const {
#ifndef NDEBUG
        assert_not_reentrant_();
#endif
        return CStrGuard{ s_, m_ };
    }

} // namespace mino::core::string
