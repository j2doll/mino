#pragma once

#include <optional>
#include <utility>

// -----------------------------------------------------------------------------
// 매크로 유틸리티: 토큰 붙이기
// MINO_PP_CAT2: 두 토큰을 그대로 이어 붙입니다.
// MINO_PP_CAT : 매크로 치환 후 이어 붙일 수 있도록 2단계 구조를 사용합니다.
// MINO_UNIQUE_NAME: 현재 소스코드의 __LINE__ 값을 붙여서 고유 이름을 생성합니다.
// -----------------------------------------------------------------------------
#define MINO_PP_CAT2(a,b) a##b
#define MINO_PP_CAT(a,b)  MINO_PP_CAT2(a,b)
#define MINO_UNIQUE_NAME(prefix) MINO_PP_CAT(prefix, __LINE__)

// -----------------------------------------------------------------------------
// 내부 구현부: optional<T> 결과를 다룰 때 자주 쓰는 패턴을 매크로화했습니다.
// -----------------------------------------------------------------------------

// MINO_TRY_OPT_IMPL
// - std::optional<T> 타입을 반환하는 함수에서 사용.
// - EXPR이 값이 없으면 빈 optional을 리턴합니다.
// - 값이 있으면 VAR에 std::move로 바인딩합니다.
//
// 예시:
//   std::optional<int> foo() {
//       TRY_OPT(x, getValue()); // getValue() → std::optional<int>
//       return x + 10;
//   }
// -----------------------------------------------------------------------------
#define MINO_TRY_OPT_IMPL(VAR, EXPR, TMP) \
    auto TMP = (EXPR);                   \
    if (!TMP) return {};                 \
    auto VAR = std::move(*TMP)

// MINO_TRY_OPT_NO_BIND_IMPL
// - 바인딩 없이 결과만 확인하고 싶을 때 사용.
// - 값이 없으면 빈 optional을 리턴합니다.
// -----------------------------------------------------------------------------
#define MINO_TRY_OPT_NO_BIND_IMPL(EXPR, TMP) \
    { auto TMP = (EXPR); if (!TMP) return {}; }

// MINO_TRY_OPT_VOID_IMPL
// - 리턴 타입이 void인 함수에서 optional을 검사할 때 사용.
// - 값이 없으면 그냥 return;
// -----------------------------------------------------------------------------
#define MINO_TRY_OPT_VOID_IMPL(EXPR, TMP) \
    { auto TMP = (EXPR); if (!TMP) return; }

// MINO_TRY_OPT_BIND_VOID_IMPL
// - void 리턴 함수에서 optional의 값을 바인딩하고 싶을 때 사용.
// -----------------------------------------------------------------------------
#define MINO_TRY_OPT_BIND_VOID_IMPL(VAR, EXPR, TMP) \
    auto TMP = (EXPR);                             \
    if (!TMP) return;                              \
    auto VAR = std::move(*TMP)

// -----------------------------------------------------------------------------
// 공개 매크로 (optional 전용)
// -----------------------------------------------------------------------------
#define TRY_OPT(VAR, EXPR)               MINO_TRY_OPT_IMPL(VAR, EXPR, MINO_UNIQUE_NAME(_opt_))
#define TRY_OPT_NO_BIND(EXPR)            MINO_TRY_OPT_NO_BIND_IMPL(EXPR, MINO_UNIQUE_NAME(_opt_))
#define TRY_OPT_VOID(EXPR)               MINO_TRY_OPT_VOID_IMPL(EXPR, MINO_UNIQUE_NAME(_opt_))
#define TRY_OPT_BIND_VOID(VAR, EXPR)     MINO_TRY_OPT_BIND_VOID_IMPL(VAR, EXPR, MINO_UNIQUE_NAME(_opt_))

// -----------------------------------------------------------------------------
// (선택) 포인터 전용 매크로
// std::unique_ptr<T> 또는 생 포인터를 다룰 때 유사한 방식으로 활용.
// -----------------------------------------------------------------------------

// MINO_TRY_PTR_IMPL
// - 포인터가 nullptr인지 검사 후 nullptr이면 빈 optional 리턴.
// - nullptr이 아니면 VAR에 바인딩.
// 예시:
//   std::optional<int> bar() {
//       TRY_PTR(ptr, getPointer()); // getPointer() → int*
//       return *ptr + 5;
//   }
// -----------------------------------------------------------------------------
#define MINO_TRY_PTR_IMPL(VAR, EXPR, TMP) \
    auto TMP = (EXPR);                   \
    if (!(TMP)) return {};               \
    auto VAR = (TMP)

// MINO_TRY_PTR_NO_BIND_IMPL
// - 포인터 검사만 하고 바인딩은 필요 없을 때 사용.
// -----------------------------------------------------------------------------
#define MINO_TRY_PTR_NO_BIND_IMPL(EXPR, TMP) \
    { auto TMP = (EXPR); if (!(TMP)) return {}; }

// MINO_TRY_PTR_VOID_IMPL
// - void 함수에서 포인터가 nullptr이면 그냥 return;
// -----------------------------------------------------------------------------
#define MINO_TRY_PTR_VOID_IMPL(EXPR, TMP) \
    { auto TMP = (EXPR); if (!(TMP)) return; }

// MINO_TRY_PTR_BIND_VOID_IMPL
// - void 함수에서 포인터를 바인딩까지 하고 싶을 때 사용.
// -----------------------------------------------------------------------------
#define MINO_TRY_PTR_BIND_VOID_IMPL(VAR, EXPR, TMP) \
    auto TMP = (EXPR);                             \
    if (!(TMP)) return;                            \
    auto VAR = (TMP)

// -----------------------------------------------------------------------------
// 공개 매크로 (포인터 전용)
// -----------------------------------------------------------------------------
#define TRY_PTR(VAR, EXPR)               MINO_TRY_PTR_IMPL(VAR, EXPR, MINO_UNIQUE_NAME(_ptr_))
#define TRY_PTR_NO_BIND(EXPR)            MINO_TRY_PTR_NO_BIND_IMPL(EXPR, MINO_UNIQUE_NAME(_ptr_))
#define TRY_PTR_VOID(EXPR)               MINO_TRY_PTR_VOID_IMPL(EXPR, MINO_UNIQUE_NAME(_ptr_))
#define TRY_PTR_BIND_VOID(VAR, EXPR)     MINO_TRY_PTR_BIND_VOID_IMPL(VAR, EXPR, MINO_UNIQUE_NAME(_ptr_))

// -----------------------------------------------------------------------------
// 요약
// - TRY_OPT 계열: std::optional<T> 처리
// - TRY_PTR 계열: 포인터 처리
// 
// 장점:
// - if (!val) return {}; 같은 보일러플레이트 코드를 줄일 수 있음.
// - 코드 가독성과 안전성을 높임.
// -----------------------------------------------------------------------------
