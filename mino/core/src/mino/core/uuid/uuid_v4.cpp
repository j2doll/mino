
#include "mino/core/uuid/uuid_v4.hpp"

//
// 구현 상세
// - thread_local 엔진(mt19937_64)을 각 스레드에서 1회 시드
// - 시드 재료: random_device, 고해상도 시계, 스레드ID 해시, 원자 카운터
// - 16바이트 생성 후 버전/바리언트 비트 설정, 하이픈 포함 문자열 변환
//

namespace mino::core::uuid {

    // 전역 카운터: 프로세스 내 추가적인 엔트로피로 활용
    static std::atomic<unsigned long long> g_counter{ 0 };

    unsigned long long& uuid_v4::counter() {
        // 원자값 자체를 참조로 노출하지 않고, 카운트 목적에만 사용
        static unsigned long long dummy = 0; // 사용하지 않음(인터페이스 형태 보존)
        (void)dummy;
        return dummy;
    }

    // 64비트 난수를 반환한다. thread_local 엔진을 사용한다.
    unsigned long long uuid_v4::rng_next64() {
        // 각 스레드마다 독립적인 PRNG 인스턴스를 보유한다.
        thread_local std::mt19937_64 rng = [] {
            // random_device가 충분한 엔트로피를 제공하지 않더라도
            // 다양한 재료를 섞어 seed_seq로 초기화한다.
            std::random_device rd;
            const auto now = static_cast<unsigned long long>(
                std::chrono::high_resolution_clock::now()
                .time_since_epoch().count());
            const auto tid_hash =
                static_cast<unsigned long long>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
            const auto cnt = g_counter.fetch_add(1, std::memory_order_relaxed);

            std::array<std::uint32_t, 8> seeds{
                static_cast<std::uint32_t>(rd()),
                static_cast<std::uint32_t>(rd()),
                static_cast<std::uint32_t>(now),
                static_cast<std::uint32_t>(now >> 32),
                static_cast<std::uint32_t>(tid_hash),
                static_cast<std::uint32_t>(tid_hash >> 32),
                static_cast<std::uint32_t>(cnt),
                static_cast<std::uint32_t>((cnt ^ now) + 0x9e3779b9u)
            };
            std::seed_seq seq(seeds.begin(), seeds.end());
            return std::mt19937_64(seq);
            }();

        return rng();
    }

    // 16바이트 랜덤 값을 채운다.
    void uuid_v4::generate_bytes(unsigned char(&b)[16]) {
        const auto r1 = rng_next64();
        const auto r2 = rng_next64();

        for (int i = 0; i < 8; ++i) {
            b[i] = static_cast<unsigned char>((r1 >> (i * 8)) & 0xFFu);
            b[i + 8] = static_cast<unsigned char>((r2 >> (i * 8)) & 0xFFu);
        }
    }

    // RFC 4122: 버전(4) 및 바리언트(10xx xxxx) 비트를 설정한다.
    void uuid_v4::set_version_and_variant(unsigned char(&b)[16]) {
        // 버전 4: 바이트 인덱스 6의 상위 4비트를 0100으로 설정
        b[6] = static_cast<unsigned char>((b[6] & 0x0Fu) | 0x40u);
        // 바리언트(2비트): 바이트 인덱스 8의 상위 2비트를 10로 설정
        b[8] = static_cast<unsigned char>((b[8] & 0x3Fu) | 0x80u);
    }

    // 16바이트를 하이픈 포함 문자열(8-4-4-4-12)로 변환한다.
    std::string uuid_v4::format_bytes(const unsigned char(&b)[16]) {
        std::ostringstream oss;
        oss << std::hex << std::nouppercase << std::setfill('0');

        auto emit2 = [&](int idx) {
            oss << std::setw(2) << static_cast<unsigned>(b[idx]);
            };

        // 8-4-4-4-12
        for (int i = 0; i < 4; ++i) emit2(i);
        oss << '-';
        for (int i = 4; i < 6; ++i) emit2(i);
        oss << '-';
        for (int i = 6; i < 8; ++i) emit2(i);   // 버전 비트 포함
        oss << '-';
        for (int i = 8; i < 10; ++i) emit2(i);  // 바리언트 비트 포함
        oss << '-';
        for (int i = 10; i < 16; ++i) emit2(i);

        return oss.str();
    }

    // 공개 API: UUID v4 문자열 생성
    std::string uuid_v4::generate() {
        unsigned char bytes[16]{};
        generate_bytes(bytes);
        set_version_and_variant(bytes);
        return format_bytes(bytes);
    }

} 