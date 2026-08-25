#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <cctype>

#include "mino/core/string/string.hpp"
#include "mino/core/encoding/encoding.hpp"

#include "mino/core/reflect/reflect.hpp"

//----------------------------------------------
// 직렬화/역직렬화 대상(REFLECT)인 구조체 정의

struct point {
    double x;
    double y;

    MINO_REFLECT(x, y)
};
// NOTE: 구조체에 MINO_REFLECT() 매크로를 설정하면,
// binary_writer와 binary_reader가 해당 구조체를 자동으로 직렬화/역직렬화할 수 있습니다.   

// MINO_REFLECT 적용 대상인 구조체의 멤버 타입
// - `int`, `uint8_t`, `uint32_t`, `uint64_t`, `float`, `double`, `bool`, `char` 등의 기본 타입(POD)
// - `enum` 및 `enum class`
// - `MINO_REFLECT` 매크로가 정의된 구조체 및 클래스
// - `std::string`
// - `std::vector<T>`
// - `std::deque<T>`
// - `std::list<T>`
// - Trivially Copyable한 C 스타일 구조체 및 기본형 고정 크기 배열

struct sensor_unit {
    uint32_t id;
    std::string model;
    point location;
    std::vector<double> history;

    MINO_REFLECT(id, model, location, history)
};

struct telemetry_packet {
    uint64_t timestamp;
    std::string device_name;
    std::vector<sensor_unit> sensors;

    MINO_REFLECT(timestamp, device_name, sensors)
};

// Dump data for debugging
void dump_hex_range(const uint8_t* data, size_t len, std::ostream& os = std::cout);
void dump_hex(const std::vector<uint8_t>& buf, std::ostream& os = std::cout);
void dump_reader(const mino::core::reflect::binary_reader& reader, const std::vector<uint8_t>& full_buffer, std::ostream& os = std::cout);

int main() {
    namespace mcr = mino::core::reflect;
    using binary_writer = mcr::binary_writer;
    using binary_reader = mcr::binary_reader;
    auto tce = mino::core::string::to_console_encoding;

    // 1. 원본 패킷 구성
    telemetry_packet original; // 원본 구조체
    original.timestamp = 1771800000;
    original.device_name = "edge_node_seoul_01";
    original.sensors = {
      //{id,  model,            location(x,y),        history}
        {101, "temp_sensor",     {37.5665, 126.9780}, {21.5, 22.0, 21.8}},
        {102, "pressure_sensor", {37.5665, 126.9780}, {1013.25, 1012.90}}
    };

    // 원본 패킷 정보 출력
    std::cout << tce("[ 원본 패킷 정보 ]\n");
    std::cout << " - timestamp: " << original.timestamp << std::endl;
    std::cout << " - device_name: " << original.device_name << std::endl;
    std::cout << " - sensors count: " << original.sensors.size() << std::endl;
    for (const auto& sensor : original.sensors) {
        std::cout << "   * sensor ID: " << sensor.id
            << " | model: " << sensor.model
            << " | location: (" << sensor.location.x << ", " << sensor.location.y << ")"
            << " | history count: " << sensor.history.size()
            << std::endl;
    }
    std::cout << std::endl;

    // 2. 직렬화 (구조체 -> 바이트 배열)
    binary_writer writer;
    try {
        writer(original);
    }
    catch (const std::bad_alloc& e) {
        std::cerr << tce("직렬화 실패 (메모리 부족): ") << e.what() << '\n';
        return 1; 
    }

    std::cout << tce("[ 직렬화 완료 ]\n");
    std::cout << tce("- 생성된 바이트 크기: ") << writer.size() << " bytes\n\n";

    // Dump the binary buffer (hex + ASCII)
    std::cout << tce("[ writer buffer dump ]\n");
    dump_hex(writer.get_buffer());
    std::cout << std::endl;

    // Base64 인코딩된 정보
    // NOTE: json, xml 등으로 전달하는 경우, 바이너리 어레이 보다 base64 인코딩된 문자열을
    //  전달하는 것이 일반적입니다.
    auto base64_encoded = mino::core::encoding::base64_encode(writer.get_buffer());
    std::cout << tce("[ Base64 인코딩 ]\n") << base64_encoded << std::endl << std::endl;

    // 3. 역직렬화 (바이트 배열 -> 구조체)
    binary_reader reader(writer.get_buffer().data(), writer.size()); // 바이트 배열을 읽기 위한 reader 생성
    telemetry_packet restored; // 구조체
    reader(restored); // 역직렬화 수행

    // 4. 복원 검증
    if (reader.has_error()) {
        // 역직렬화 중 오류 발생
        std::cerr << tce("역직렬화 중 오류 발생!") << std::endl;
        return 2;
    }

    std::cout << tce("[ 역직렬화 검증 성공 ]\n");
    std::cout << tce("- timestamp: ") << restored.timestamp << std::endl;
    std::cout << tce("- device_name: ") << restored.device_name << std::endl;
    std::cout << tce("- sensors count: ") << restored.sensors.size() << std::endl;

    for (const auto& sensor : restored.sensors) {
        std::cout
            << tce("  * sensor ID: ") << sensor.id
            << tce(" | model: ") << sensor.model
            << tce(" | location: (") << sensor.location.x << ", " << sensor.location.y << ")"
            << tce(" | history count: ") << sensor.history.size()
            << std::endl;
    }

    return 0;
}

// Hex + ASCII dump for a buffer range
void dump_hex_range(const uint8_t* data, size_t len, std::ostream& os) {
    const size_t width = 16;
    for (size_t off = 0; off < len; off += width) {
        os << std::hex << std::setw(8) << std::setfill('0') << off << ": ";

        for (size_t i = 0; i < width; ++i) {
            if (off + i < len) {
                os << std::setw(2) << std::setfill('0') << std::uppercase
                    << static_cast<int>(data[off + i]) << ' ';
            }
            else {
                os << "   ";
            }
        }

        os << " ";

        for (size_t i = 0; i < width && off + i < len; ++i) {
            unsigned char c = data[off + i];
            os << (std::isprint(c) ? static_cast<char>(c) : '.');
        }

        os << std::endl;
    }

    os << std::dec << std::setfill(' ');
}

void dump_hex(const std::vector<uint8_t>& buf, std::ostream& os) {
    dump_hex_range(buf.data(), buf.size(), os);
}

// Dump reader state: bytes read, remaining bytes, error flag, and unread bytes hex dump.
void dump_reader(
    const mino::core::reflect::binary_reader& reader,
    const std::vector<uint8_t>& full_buffer,
    std::ostream& os)
{
    const auto read = reader.bytes_read();
    const auto total = full_buffer.size();
    const auto remaining = (total > read) ? (total - read) : 0;

    os << "[ reader dump ]\n";
    os << "  bytes_read: " << read << "\n";
    os << "  total_size: " << total << "\n";
    os << "  remaining : " << remaining << "\n";
    os << "  has_error : " << (reader.has_error() ? "true" : "false") << "\n";

    if (remaining > 0) {
        os << "  unread bytes (hex):\n";
        dump_hex_range(full_buffer.data() + read, remaining, os);
    }
    else {
        os << "  no unread bytes\n";
    }

    os << std::endl;
}

