#pragma once

#include <string>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

// * 랜덤 결과 형식: 8-4-4-4-12 (하이픈 포함 36자)
// * 버전/바리언트 비트 RFC 4122 준수
// * 스레드-세이프(generate() 재진입 가능)

namespace mino::core::uuid {

    class  uuid_v4 {
    public:
        // UUID v4 문자열을 생성한다.
        // 예) "f47ac10b-58cc-4372-a567-0e02b2c3d479"
        static std::string generate();

    private:
        // 아래 헬퍼들은 구현 파일(.cpp)에서 정의된다.
        static std::string format_bytes(const unsigned char(&b)[16]);
        static void set_version_and_variant(unsigned char(&b)[16]);
        static void generate_bytes(unsigned char(&b)[16]);

        // 전역 카운터 접근자(충돌 위험 추가 완화용)
        static unsigned long long& counter();

        // thread_local PRNG 접근자
        static unsigned long long rng_next64();
    };

}