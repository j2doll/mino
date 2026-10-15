#pragma once

#include <utility>

namespace mino::core::scope {

    template <typename F>
    class scope_exit {
    public:
        explicit scope_exit(F f) : fn_(std::move(f)), active_(true) {}

        ~scope_exit() {
            if (active_) {
                fn_();
            }
        }

        // 복사 방지
        scope_exit(const scope_exit&) = delete;
        scope_exit& operator=(const scope_exit&) = delete;

        // 이동 허용
        scope_exit(scope_exit&& other) noexcept
            : fn_(std::move(other.fn_)), active_(other.active_) {
            other.active_ = false;
        }
        scope_exit& operator=(scope_exit&&) = delete;

        // 자원 해제 취소 기능
        void release() noexcept {
            active_ = false;
        }

    private:
        F fn_;
        bool active_;
    };

    // C++17 CTAD 유도 가이드 (동일 네임스페이스 내에 위치해야 합니다)
    template <typename F>
    scope_exit(F) -> scope_exit<F>;

    // 헬퍼 함수
    template <typename F>
    auto make_scope_exit(F&& f) {
        return scope_exit<std::decay_t<F>>(std::forward<F>(f));
    }

} // namespace mino::core::scope
