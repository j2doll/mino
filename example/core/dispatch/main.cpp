#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/core/datetime/datetime.hpp"
#include "mino/core/dispatch/dispatch.hpp"

// =============================================================================
// 비-POD(Non-POD) 및 다양한 메모리 구조를 가진 이벤트 정의
// =============================================================================

extern void test_dynamic_event_dispatcher();
extern void test_any_event_dispatcher();
extern void test_static_dispatcher();
extern void test_collecting_event_dispatcher();

int main() {
    namespace mcd = mino::core::dispatch;

    test_dynamic_event_dispatcher(); // 1. dynamic_event_dispatcher 테스트
    test_any_event_dispatcher(); // 2. any_event_dispatcher 테스트
    test_static_dispatcher(); // 3. static_dispatcher 테스트
    test_collecting_event_dispatcher(); // 4. collecting_event_dispatcher 테스트

    return 0;
}
