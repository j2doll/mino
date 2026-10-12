#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

#include "mino/core/container/span.hpp"
#include "mino/core/log/log.hpp"

namespace tinylog = mino::core::log::tinylog;
namespace container = mino::core::container;

namespace {

    // span을 인자로 받아 데이터를 복사 없이 출력하는 헬퍼 함수
    void print_span(
        const std::shared_ptr<tinylog::logger>& log,
        std::string_view label,
        container::span<const int> view)
    {
        std::string elements;
        for (std::size_t i = 0; i < view.size(); ++i) {
            elements += std::to_string(view[i]);
            if (i + 1 < view.size()) {
                elements += ", ";
            }
        }
        log->info("{0}: [ {1} ] (크기: {2}, 바이트: {3})",
            label, elements, view.size(), view.size_bytes());
    }

    // 네트워크 패킷 구조체 예시 (바이트 뷰 검증용)
    struct PacketHeader {
        std::uint32_t packet_id{ 1001 };  // 4 바이트 
        std::uint16_t body_size{ 128 };   // 2 바이트
        std::uint16_t checksum{ 0xABCD }; // 2 바이트
    };

} // anonymous namespace

void test_span() {
    using console_sink = tinylog::console_sink;
    using console_sink_config = tinylog::console_sink_config;
    using encoding_type = tinylog::encoding_type;
    using eol_type = tinylog::eol_type;
    using log_level = tinylog::log_level;

    std::cout << std::endl;

    //--------------------------------------------------
    // 1. tinylog 초기화
    console_sink_config config;
#ifdef _WIN32
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::crlf;
#else
    config.encoding = encoding_type::utf8;
    config.eol = eol_type::lf;
#endif
    auto console = std::make_shared<console_sink>("console", config);
    assert(console != nullptr);
    auto log = std::make_shared<tinylog::logger>("span_test");
    assert(log != nullptr);
    log->add_sink(console);
    log->set_level(log_level::trace);

    log->info("<bold><cyan>=== Mino C++17 span 검증 테스트 시작 ===</cyan></bold>");

    //--------------------------------------------------
    // 2. 다양한 컨테이너와의 제로 카피 연동 검증
    int raw_array[] = { 10, 20, 30, 40, 50 };
    std::vector<int> dynamic_vec = { 100, 200, 300 };
    std::array<int, 4> std_arr = { 1, 2, 3, 4 };

    print_span(log, "C 스타일 정적 배열", raw_array);
    print_span(log, "std::vector 연동   ", dynamic_vec);
    print_span(log, "std::array 연동    ", std_arr);

    //--------------------------------------------------
    // 3. span을 통한 원본 데이터 수정 검증

    // span은 소유권이 없는(Non-owning) 뷰(View) 컨테이너로,
    // 연속 메모리 영역을 가리키며
    // 고정 크기(Static Extent) 또는 동적 크기(Dynamic Extent)를 지원합니다.

    container::span<int> writable_view(dynamic_vec);
    writable_view[1] = 999; // writable_view의 두번째 요소를 999로 수정.
    // span은 원본 vector인 dynamic_vec를 직접 참조하므로 dynamic_vec의 두번째 요소도 999로 변경됨.
    log->info(
        "<bright_yellow>[수정 검증]</bright_yellow> span을 통해 vector[1] 수정"
        " -> vector[1] = {0}",
        dynamic_vec[1]); // dynamic_vec의 두 번째 요소가 999로 변경되었는지 확인.

    //--------------------------------------------------
    // 4. 슬라이싱(Subviews) 검증: first, last, subspan
    container::span<const int> full_view(raw_array);
    auto first_two = full_view.first(2); // {10, 20}
    auto last_two = full_view.last(2); // {40, 50}
    auto middle = full_view.subspan(1, 3); // {20, 30, 40}

    print_span(log, "first(2)   슬라이스", first_two); // first(2) 슬라이스: [10, 20] (크기:2, 바이트:8)
    print_span(log, "last(2)    슬라이스", last_two); // last(2) 슬라이스: [40, 50] (크기:2, 바이트:8)
    print_span(log, "subspan(1, 3)      ", middle); // subspan(1, 3): [20, 30, 40] (크기:3, 바이트:12)

    //--------------------------------------------------
    // 5. 제로 오버헤드 (메모리 크기) 검증
    container::span<int, 5> fixed_span(raw_array); // 고정 크기 (Static Extent)
    container::span<int> dynamic_span(raw_array);  // 동적 크기 (Dynamic Extent)

    log->info(
        "<bright_green>[메모리 검증]</bright_green> 고정 크기 span 크기"
        " : {0} bytes",
        sizeof(fixed_span)); // 16 bytes = 
        // pointer 1개(64-bit OS에서 8 바이트) +
        // size_t 1개(64-bit OS에서 8 바이트. unsigned integer와 같은 크기)

    log->info(
        "<bright_green>[메모리 검증]</bright_green> 동적 크기 span 크기"
        " : {0} bytes",
        sizeof(dynamic_span)); // 16 bytes: pointer 1개(8B) + size_t 1개(8B) (64-bit OS)

    //--------------------------------------------------
    // 6. 바이트 뷰(as_bytes) 변환 및 버퍼 직렬화 검증
    PacketHeader header;
    container::span<const PacketHeader, 1> header_view(&header, 1); // PacketHeader 구조체를 span으로 감싸기
    auto byte_view = container::as_bytes(header_view); // PacketHeader를 바이트 뷰로 변환

    log->info(
        "<cyan>[바이트 뷰]</cyan> PacketHeader 구조체 크기:"
        " {0} bytes,"
        " as_bytes 크기: {1} bytes",
        sizeof(PacketHeader), // 8 bytes
        byte_view.size_bytes() // 8 bytes
    );

    log->info("<bold><bright_green>=== 모든 span 테스트 통과 ===</bright_green></bold>");
}

