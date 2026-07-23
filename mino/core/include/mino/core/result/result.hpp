#pragma once

#include <iostream>
#include <variant>
#include <string>
#include <type_traits>
#include <utility>

namespace mino::core::result {

    // 여러 람다를 하나로 묶어주는 헬퍼 구조체
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;

    // 범용 결과 컨테이너 클래스
    // - SuccessVariant: 성공 쪽의 std::variant 타입 (예: std::variant<int, std::string>)
    // - FailVariant:    실패 쪽의 std::variant 타입 (예: std::variant<std::error_code>)
    template <typename SuccessVariant, typename FailVariant>
    class result_container {
    public:
        using success_var = SuccessVariant;
        using fail_var = FailVariant;
        using data_var = std::variant<success_var, fail_var>;

        // 최종 데이터: 성공 variant 또는 실패 variant
        data_var data;

        // 성공 결과 생성
        template <typename T>
        static result_container success(T&& value) {
            return result_container{ success_var(std::forward<T>(value)) };
        }

        // 실패 결과 생성
        template <typename T>
        static result_container fail(T&& value) {
            return result_container{ fail_var(std::forward<T>(value)) };
        }

        // 성공 여부 확인
        bool is_success() const { return std::holds_alternative<success_var>(data); }

        // 결과 처리 매칭 함수
        template <typename SFunc, typename FFunc>
        void match(SFunc&& s_logic, FFunc&& f_logic) {
            if (is_success()) {
                std::visit(std::forward<SFunc>(s_logic), std::get<0>(data));
            }
            else {
                std::visit(std::forward<FFunc>(f_logic), std::get<1>(data));
            }
        }

    private:
        explicit result_container(success_var&& s) : data(std::move(s)) {}
        explicit result_container(fail_var&& f) : data(std::move(f)) {}
    };

}