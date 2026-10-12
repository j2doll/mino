#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace mino::core::task {

    class move_task {
    private:
        struct task_concept {
            virtual ~task_concept() = default;
            virtual void execute() = 0;
        };

        template <typename F>
        struct task_model final : task_concept {
            F callable;

            explicit task_model(F&& fn) : callable(std::forward<F>(fn)) {}
            void execute() override { callable(); }
        };

        std::unique_ptr<task_concept> impl_;

    public:
        move_task() noexcept = default;

        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, move_task>>>
        move_task(F&& fn) : impl_(std::make_unique<task_model<std::decay_t<F>>>(std::forward<F>(fn))) {}

        move_task(move_task&& other) noexcept = default;
        move_task& operator=(move_task&& other) noexcept = default;

        move_task(const move_task&) = delete;
        move_task& operator=(const move_task&) = delete;

        void operator()() {
            if (impl_) {
                impl_->execute();
            }
        }

        explicit operator bool() const noexcept {
            return static_cast<bool>(impl_);
        }

        void reset() noexcept {
            impl_.reset();
        }
    };

} // namespace mino::core::task
