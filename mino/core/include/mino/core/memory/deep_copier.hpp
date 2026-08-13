#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <type_traits>
#include <cstdint>

namespace mino::core::memory {

    // --- Deep Copy를 위한 직렬화 스트림 클래스 ---
    class serializer {
    public:
        std::vector<uint8_t> buffer;

        // 기본 타입 및 POD 데이터 직렬화
        template <typename T>
        void serialize(const T& data) {
            if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
                buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
            }
            else {
                data.serialize(*this);
            }
        }

        // std::string 특수화
        void serialize(const std::string& str) {
            size_t size = str.size();
            serialize(size);
            buffer.insert(buffer.end(), str.begin(), str.end());
        }

        // std::vector 특수화
        template <typename T>
        void serialize(const std::vector<T>& vec) {
            size_t size = vec.size();
            serialize(size);
            for (const auto& item : vec) {
                serialize(item);
            }
        }
    };

    class deserializer {
    private:
        const std::vector<uint8_t>& buffer_;
        size_t offset_ = 0;

    public:
        deserializer(const std::vector<uint8_t>& buf) : buffer_(buf) {}

        size_t offset() const { return offset_; }
        size_t remaining() const { return buffer_.size() - offset_; }

        // 기본 타입 및 POD 데이터 역직렬화 (성공 여부 bool 반환)
        template <typename T>
        bool deserialize(T& data) {
            if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
                if (offset_ + sizeof(T) > buffer_.size()) {
                    return false;
                }
                std::memcpy(&data, &buffer_[offset_], sizeof(T));
                offset_ += sizeof(T);
                return true;
            }
            else {
                return data.deserialize(*this);
            }
        }

        // std::string 특수화
        bool deserialize(std::string& str) {
            size_t size = 0;
            if (!deserialize(size)) {
                return false;
            }
            if (offset_ + size > buffer_.size()) {
                return false;
            }
            str.assign(reinterpret_cast<const char*>(&buffer_[offset_]), size);
            offset_ += size;
            return true;
        }

        // std::vector 특수화
        template <typename T>
        bool deserialize(std::vector<T>& vec) {
            size_t size = 0;
            if (!deserialize(size)) {
                return false;
            }

            // 손상된 데이터로 인한 과도한 메모리 할당 방지
            if (size > buffer_.size() - offset_) {
                return false;
            }

            vec.resize(size);
            for (size_t i = 0; i < size; ++i) {
                if (!deserialize(vec[i])) {
                    return false;
                }
            }
            return true;
        }
    };

    // --- Deep Copier 유틸리티 ---
    class deep_copier {
    public:
        // 성공 여부를 확인하고 결과를 출력 파라미터에 담는 방식
        template <typename T>
        static bool copy(const T& src, T& dst) {
            serializer s;
            s.serialize(src);

            deserializer d(s.buffer);
            return d.deserialize(dst);
        }

        // 기존 인터페이스 호환용 (복사 실패 시 기본 생성된 객체 반환)
        template <typename T>
        static T copy(const T& obj) {
            T new_obj{};
            copy(obj, new_obj);
            return new_obj;
        }
    };

} // namespace mino::core::memory
