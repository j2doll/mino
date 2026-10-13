#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "mino/core/type_traits/concepts.hpp"
#include "mino/core/container/span.hpp"
#include "mino/core/string/string.hpp"

namespace mcsp = mino::core::string::print;
namespace mct = mino::core::traits;
namespace mcc = mino::core::container;

// 1. 함수 템플릿 제약: 정수형 전용 처리
template <typename T MINO_REQUIRES(mct::Integral<T>)>
void process_number(T val) {
    mcsp::println("[정수 처리] 값: {}, 타입 크기: {} bytes", val, sizeof(T));
}

// 2. 함수 템플릿 제약: 부동소수점형 전용 처리 (SFINAE 오버로딩)
template <typename T MINO_REQUIRES(mct::FloatingPoint<T>)>
void process_number(T val) {
    mcsp::println("[실수 처리] 값: {}, 타입 크기: {} bytes", val, sizeof(T));
}

// 3. 복합 표현식 제약: .data()와 .size()를 보유한 연속 버퍼 전용 전송 함수
template <typename Buffer MINO_REQUIRES(mct::ContiguousContainer<Buffer>)>
void transmit_packet(std::string_view label, const Buffer& buf) {
    mcsp::printlnss("[패킷 전송] 대상: {target}, 원소 수: {count}, 총 크기: {bytes} bytes", {
        {"target", label},
        {"count", buf.size()},
        {"bytes", buf.size() * sizeof(typename Buffer::value_type)}
    });
}

// 4. 클래스 템플릿 제약: POD 구조체만 수용 가능한 홀더
template <typename T, typename = std::enable_if_t<mct::TriviallyCopyable<T>>>
class NetworkPayloadHolder {
public:
    T payload{};
};

// 테스트용 데이터 구조체
struct HeaderPacket {
    std::uint32_t seq_id{ 101 };
    std::uint16_t op_code{ 0x01 };
};

int main() {
    auto to_console_encoding = mino::core::string::to_console_encoding;
    mcsp::println("=== Mino C++17 Concepts & Print 검증 시작 ===\n");

    ////////////////////////////////////////////////////
    // [1] 정수/실수 타입 제약 및 println 포맷팅 검증

    process_number(42);  // Integral<T> 분기
    // [정수 처리] 값: 42, 타입 크기 : 4 bytes

    process_number(10000000000LL); // Integral<T> 분기
    // [정수 처리] 값 : 10000000000, 타입 크기 : 8 bytes

    process_number(3.14159265); // FloatingPoint<T> 분기
    // [실수 처리] 값 : 3.14159, 타입 크기 : 8 bytes

    process_number(2.718f); // FloatingPoint<T> 분기
    // [실수 처리] 값 : 2.718, 타입 크기 : 4 bytes

    mcsp::println("");

    ////////////////////////////////////////////////////
    // [2] 표현식 제약 (ContiguousContainer)  

    std::vector<std::uint8_t> byte_vec = { 0x10, 0x20, 0x30, 0x40 };
    transmit_packet("std::vector 버퍼", byte_vec);
    // [패킷 전송] 대상: std::vector 버퍼, 원소 수 : 4, 총 크기 : 4 bytes

    std::string text_msg = "Hello Mino Core";
    transmit_packet("std::string 버퍼", text_msg);
    // [패킷 전송] 대상 : std::string 버퍼, 원소 수 : 15, 총 크기 : 15 bytes

    int raw_arr[] = { 1, 2, 3, 4, 5 };
    mcc::span<int> span_view(raw_arr);
    transmit_packet("mcc::span 버퍼   ", span_view);
    // [패킷 전송] 대상 : mcc::span 버퍼, 원소 수 : 5, 총 크기 : 20 bytes

    mcsp::println("");

    ////////////////////////////////////////////////////
    // [3] 클래스 TriviallyCopyable 제약 검증

    NetworkPayloadHolder<HeaderPacket> valid_holder;
    valid_holder.payload.seq_id = 999; // seq_id 는 uint32_t 타입이며, TriviallyCopyable 제약을 만족함.
    mcsp::println("[성공] TriviallyCopyable 제약 통과 (Seq ID: {})", valid_holder.payload.seq_id);
    // [성공] TriviallyCopyable 제약 통과 (Seq ID: 999)

    ////////////////////////////////////////////////////
    // [4] 런타임 출력 대신 컴파일 타임 Concepts 검증

    {
        // 검증용 샘플 타입 정의
        struct TrivialPacket {
            std::uint32_t id;
            float value;
        };

        struct NonTrivialPacket {
            std::string name; // non-trivial 소멸자/복사 생성자를 가지므로 TriviallyCopyable 위반
        };

        // =============================================================================
        // 1. 기존 요청 케이스 (영문 변환)
        // =============================================================================

        static_assert(mct::Integral<int>,
            "int must satisfy Integral concept.");
        static_assert(!mct::Integral<std::string>,
            "std::string must not satisfy Integral concept.");

        static_assert(mct::ContiguousContainer<std::vector<int>>,
            "std::vector must satisfy ContiguousContainer concept.");
        static_assert(!mct::ContiguousContainer<int>,
            "Primitive scalar type (int) must not satisfy ContiguousContainer concept.");


        // =============================================================================
        // 2. 추가 검증 케이스: 기본 타입 및 한정자(const/reference) 처리
        // =============================================================================

        // const 한정자 및 참조자가 붙었을 때도 decay되어 정상 인식되는지 검증
        static_assert(mct::Integral<const int&>,
            "const int& must satisfy Integral concept via std::decay_t.");
        static_assert(mct::Integral<std::uint64_t>,
            "Fixed-width integer types must satisfy Integral concept.");

        // 부동소수점 검증 및 정수/실수 상호 배타성 검증
        static_assert(mct::FloatingPoint<double>,
            "double must satisfy FloatingPoint concept.");
        static_assert(mct::FloatingPoint<float>,
            "float must satisfy FloatingPoint concept.");
        static_assert(!mct::FloatingPoint<int>,
            "int must not satisfy FloatingPoint concept.");
        static_assert(!mct::Integral<double>,
            "double must not satisfy Integral concept.");


        // =============================================================================
        // 3. 추가 검증 케이스: TriviallyCopyable (직렬화 및 메모리 복사 가능 여부)
        // =============================================================================

        // 기본 산술 타입 및 포인터는 항상 TriviallyCopyable
        static_assert(mct::TriviallyCopyable<int>,
            "Scalar types must be TriviallyCopyable.");
        static_assert(mct::TriviallyCopyable<void*>,
            "Pointer types must be TriviallyCopyable.");

        // POD 구조체 vs 동적 메모리/객체를 가진 복합 클래스
        static_assert(mct::TriviallyCopyable<TrivialPacket>,
            "Plain POD struct must satisfy TriviallyCopyable concept.");
        static_assert(!mct::TriviallyCopyable<NonTrivialPacket>,
            "Struct containing non-trivial member (std::string) must not satisfy TriviallyCopyable concept.");
        static_assert(!mct::TriviallyCopyable<std::string>,
            "std::string must not satisfy TriviallyCopyable concept.");


        // =============================================================================
        // 4. 추가 검증 케이스: 표현식 탐지 (HasSize / HasData / ContiguousContainer)
        // =============================================================================

        // std::list: .size()는 있지만 연속 메모리가 아니므로 .data()가 없음
        static_assert(mct::HasSize<std::list<int>>,
            "std::list has size() and must satisfy HasSize concept.");
        static_assert(!mct::HasData<std::list<int>>,
            "std::list does not provide contiguous data() and must fail HasData concept.");
        static_assert(!mct::ContiguousContainer<std::list<int>>,
            "Non-contiguous container (std::list) must fail ContiguousContainer concept.");

        // std::array 및 std::string (연속 메모리 지원 컨테이너)
        static_assert(mct::ContiguousContainer<std::array<int, 4>>,
            "std::array must satisfy ContiguousContainer concept.");
        static_assert(mct::ContiguousContainer<std::string>,
            "std::string must satisfy ContiguousContainer concept.");

        // mino::core::container::span 자체의 연속 버퍼 규격 만족 여부
        static_assert(mct::ContiguousContainer<mcc::span<int>>,
            "mcc::span must satisfy ContiguousContainer concept.");
        static_assert(mct::ContiguousContainer<mcc::span<const std::byte>>,
            "Byte-view span must satisfy ContiguousContainer concept.");
    }

    mcsp::println("\n=== 모든 검증 완료 ===");
    return 0;
}
