
#include "mino/core/bit/bit_array.hpp"

namespace mino::core::bit {

//-------------------------------------
// 기본 생성자
bit_array::bit_array() {
    clear();
}  

//-------------------------------------
// 초기 생성자
bit_array::bit_array(const std::vector<uint8_t>& bytes, size_t bits) {
    from(bytes, bits);
}  

//-------------------------------------
// 초기화
void bit_array::clear() {
    _bit_size = 0;
    _data.clear();
}

//-------------------------------------
// 비트 크기 반환
size_t bit_array::size() const {
    return _bit_size;
}

//-------------------------------------
// 바이트 배열로 반환
std::vector<uint8_t> bit_array::data() const {
    return _data;
}

//-------------------------------------
// 특정 비트 오프셋과 길이로 하위 비트 어레이 추출
bit_array bit_array::get(size_t bitOffset, size_t bitLength) const {

    if (bitOffset + bitLength > _bit_size) {
        bitLength = (bitOffset < _bit_size) ? _bit_size - bitOffset : 0; // Adjust to valid range
    }

    std::vector<uint8_t> subArray((bitLength + 7) / 8, 0);

    for (size_t i = 0; i < bitLength; ++i) {
        size_t sourceBitIndex = bitOffset + i;
        size_t sourceByteIndex = sourceBitIndex / 8;
        size_t sourceBitOffset = sourceBitIndex % 8;

        size_t targetByteIndex = i / 8;
        size_t targetBitOffset = i % 8;

        bool bit = (_data[sourceByteIndex] >> (7 - sourceBitOffset)) & 1;
        subArray[targetByteIndex] |= (bit << (7 - targetBitOffset));
    }

    return bit_array(subArray, bitLength);
}

//-------------------------------------
// 특정 비트 오프셋과 길이로 값을 병합
bool bit_array::merge(const bit_array& input, size_t bitOffset) {

    if (bitOffset >= _bit_size) {
        return false; // Out of bounds, do nothing
    }

    size_t mergeLength = std::min(input._bit_size, _bit_size - bitOffset);

    for (size_t i = 0; i < mergeLength; ++i) {
        size_t targetBitIndex = bitOffset + i;
        size_t targetByteIndex = targetBitIndex / 8;
        size_t targetBitOffset = targetBitIndex % 8;

        bool bit = (input._data[i / 8] >> (7 - (i % 8))) & 1;

        if (bit) {
            _data[targetByteIndex] |= (1 << (7 - targetBitOffset));
        } else {
            _data[targetByteIndex] &= ~(1 << (7 - targetBitOffset));
        }
    }

    return true;
}

//-------------------------------------
// 연산자 +를 통해 비트 어레이 병합
bit_array bit_array::operator+(const bit_array& other) const {

    size_t totalBits = _bit_size + other._bit_size;
    std::vector<uint8_t> combinedData((totalBits + 7) / 8, 0);

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t targetByteIndex = i / 8;
        size_t targetBitOffset = i % 8;

        bool bit = (_data[i / 8] >> (7 - (i % 8))) & 1;
        combinedData[targetByteIndex] |= (bit << (7 - targetBitOffset));
    }

    for (size_t i = 0; i < other._bit_size; ++i) {
        size_t targetBitIndex = _bit_size + i;
        size_t targetByteIndex = targetBitIndex / 8;
        size_t targetBitOffset = targetBitIndex % 8;

        bool bit = (other._data[i / 8] >> (7 - (i % 8))) & 1;
        combinedData[targetByteIndex] |= (bit << (7 - targetBitOffset));
    }

    return bit_array(combinedData, totalBits);
}

//-------------------------------------
// 바이트 배열로 초기화
void bit_array::from(const std::vector<uint8_t>& bytes, size_t bits) {
    _data = bytes;
    _bit_size = bits ? bits : bytes.size() * 8;
}

//-------------------------------------
// 지정된 바이트 크기로 0으로 채운 배열을 설정
void bit_array::set_bytes(size_t byteSize) {
    _data = std::vector<uint8_t>(byteSize, 0); // 0으로 초기화된 바이트 벡터 생성
    _bit_size = byteSize * 8;                  // 비트 크기 설정
}

//-------------------------------------
// 지정된 비트 크기로 0으로 채운 배열을 설정
void bit_array::set_bits(size_t bitSize) {
    _bit_size = bitSize;
    size_t byteSize = (bitSize + 7) / 8; // 비트 크기를 바이트 크기로 변환 (올림 처리)
    _data = std::vector<uint8_t>(byteSize, 0); // 0으로 초기화된 바이트 벡터 생성
}

//-------------------------------------
// 비트 배열을 boolean 배열(std::vector<bool>)로 반환 (0:false, 1:true)
std::vector<bool> bit_array::to_array() const {
    std::vector<bool> booleanArray;
    booleanArray.reserve(_bit_size); // 크기 예약

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t byteIndex = i / 8;
        size_t bitOffset = i % 8;
        bool bit = (_data[byteIndex] >> (7 - bitOffset)) & 1;
        booleanArray.push_back(bit);
    }

    return booleanArray;
}

//-------------------------------------
// 비트 이동 <<
bit_array bit_array::operator<<(size_t shift) const {
    bit_array result(_data, _bit_size);

    if (shift >= _bit_size) {
        result._data.assign((_bit_size + 7) / 8, 0); // 모두 0으로 설정
        return result;
    }

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t sourceIndex = i + shift;
        if (sourceIndex < _bit_size) {
            size_t sourceByte = sourceIndex / 8;
            size_t sourceBit = sourceIndex % 8;
            bool bit = (_data[sourceByte] >> (7 - sourceBit)) & 1;

            size_t targetByte = i / 8;
            size_t targetBit = i % 8;
            if (bit) {
                result._data[targetByte] |= (1 << (7 - targetBit));
            } else {
                result._data[targetByte] &= ~(1 << (7 - targetBit));
            }
        } else {
            // 초과된 비트는 0으로 채움
            size_t targetByte = i / 8;
            size_t targetBit = i % 8;
            result._data[targetByte] &= ~(1 << (7 - targetBit));
        }
    }
    return result;
}

//-------------------------------------
// 비트 이동 >>
bit_array bit_array::operator>>(size_t shift) const {
    bit_array result(_data, _bit_size);

    if (shift >= _bit_size) {
        result._data.assign((_bit_size + 7) / 8, 0); // 모두 0으로 설정
        return result;
    }

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t sourceIndex = i >= shift ? i - shift : _bit_size; // 이동 후 범위 초과 확인
        if (sourceIndex < _bit_size) {
            size_t sourceByte = sourceIndex / 8;
            size_t sourceBit = sourceIndex % 8;
            bool bit = (_data[sourceByte] >> (7 - sourceBit)) & 1;

            size_t targetByte = i / 8;
            size_t targetBit = i % 8;
            if (bit) {
                result._data[targetByte] |= (1 << (7 - targetBit));
            } else {
                result._data[targetByte] &= ~(1 << (7 - targetBit));
            }
        } else {
            // 초과된 비트는 0으로 채움
            size_t targetByte = i / 8;
            size_t targetBit = i % 8;
            result._data[targetByte] &= ~(1 << (7 - targetBit));
        }
    }
    return result;
}

//-------------------------------------
// 반전(reverse) byte array
void bit_array::reverser() {
    if (_data.empty() || _bit_size == 0) {
        return; // 데이터가 없거나 크기가 0이면 아무 작업도 하지 않음
    }

    // 유효한 바이트 크기 계산
    size_t byteSize = (_bit_size + 7) / 8;

    // 마지막 바이트의 유효하지 않은 비트를 보존하며 비트 전체를 반전
    std::vector<uint8_t> reversedData(byteSize, 0);

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t sourceIndex = i; // 현재 비트 위치
        size_t targetIndex = _bit_size - 1 - i; // 반전된 위치

        size_t sourceByte = sourceIndex / 8;
        size_t sourceBit = sourceIndex % 8;

        size_t targetByte = targetIndex / 8;
        size_t targetBit = targetIndex % 8;

        // 소스 비트 값 읽기
        bool bit = (_data[sourceByte] >> (7 - sourceBit)) & 1;

        // 타겟 위치에 비트 설정
        if (bit) {
            reversedData[targetByte] |= (1 << (7 - targetBit));
        }
    }

    // 결과를 저장
    _data = reversedData;
}


// bitwise NOT (각 비트의 0/1을 뒤집음)
bit_array bit_array::operator~() const {
    // 결과 버퍼 준비
    size_t byteSize = (_bit_size + 7) / 8;
    std::vector<uint8_t> invertedData(byteSize, 0);

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t byteIndex = i / 8;
        size_t bitOffset = i % 8;

        bool bit = (_data[byteIndex] >> (7 - bitOffset)) & 1;
        // 원래 비트를 뒤집어서 설정
        if (!bit) {
            invertedData[byteIndex] |= (1 << (7 - bitOffset));
        }
        // else: 이미 0으로 초기화되어 있으므로 따로 지울 필요 없음
    }

    return bit_array(invertedData, _bit_size);
}

//-------------------------------------
// 디버그용 출력
void bit_array::print(bool split4bit) const {

    if (!split4bit) {
        for (size_t i = 0; i < _bit_size; ++i) {
            size_t byteIndex = i / 8;
            size_t bitOffset = i % 8;
            std::cout << ((_data[byteIndex] >> (7 - bitOffset)) & 1);
        }
        std::cout << std::endl;
        return;
    }

    for (size_t i = 0; i < _bit_size; ++i) {
        size_t byteIndex = i / 8;
        size_t bitOffset = i % 8;

        // 각 비트를 출력
        std::cout << ((_data[byteIndex] >> (7 - bitOffset)) & 1);

        // 4자리마다 공백 추가
        if ((i + 1) % 4 == 0 && i + 1 != _bit_size) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

//-------------------------------------
// 헥스 덤프 출력
void bit_array::dump() const {

    // 출력 헤더
    std::cout << "--------+---------------------------------------" << std::endl;

    std::cout << "Offset  ";
    for (int i = 0; i < 8; ++i) {
        std::cout << "| " << std::setw(2) << std::setfill('0')
        << std::hex << std::uppercase << i << " ";
    }
    std::cout << std::endl;
    std::cout << "--------+---------------------------------------" << std::endl;

    size_t totalBytes = (_bit_size + 7) / 8; // 실제 사용하는 바이트 크기
    for (size_t i = 0; i < totalBytes; i += 8) {
        // Offset 출력
        std::cout << std::setw(6) << std::setfill('0') << std::hex << std::uppercase << i << "h ";

        // 각 바이트를 헥스 형식으로 출력
        for (size_t j = 0; j < 8; ++j) {
            if (i + j < totalBytes) {
                std::cout << "| " << std::setw(2) << std::setfill('0')
                << std::hex << std::uppercase << static_cast<int>(_data[i + j]) << " ";
            } else {
                std::cout << "|    "; // 빈 공간
            }
        }
        std::cout << std::endl;
    }

    std::cout << "--------+---------------------------------------" << std::endl;
}

 
} // namespace mino::core::bit
