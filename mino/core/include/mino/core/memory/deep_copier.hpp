#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <stdexcept>

// 요청하신 네임스페이스 적용
namespace mino::core::memory {

    // --- Deep Copy를 위한 직렬화 스트림 클래스 ---
    class  serializer {
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

    class  deserializer {
    private:
        const std::vector<uint8_t>& buffer_;
        size_t offset_ = 0;

    public:
        deserializer(const std::vector<uint8_t>& buf) : buffer_(buf) {}

        // 기본 타입 및 POD 데이터 역직렬화
        template <typename T>
        void deserialize(T& data) {
            if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
                if (offset_ + sizeof(T) > buffer_.size()) {
                    throw std::runtime_error("Buffer overflow during deserialization");
                }
                std::memcpy(&data, &buffer_[offset_], sizeof(T));
                offset_ += sizeof(T);
            }
            else {
                data.deserialize(*this);
            }
        }

        // std::string 특수화
        void deserialize(std::string& str) {
            size_t size = 0;
            deserialize(size);
            if (offset_ + size > buffer_.size()) {
                throw std::runtime_error("Buffer overflow during deserialization");
            }
            str.assign(reinterpret_cast<const char*>(&buffer_[offset_]), size);
            offset_ += size;
        }

        // std::vector 특수화
        template <typename T>
        void deserialize(std::vector<T>& vec) {
            size_t size = 0;
            deserialize(size);
            vec.resize(size);
            for (size_t i = 0; i < size; ++i) {
                deserialize(vec[i]);
            }
        }
    };

    // --- Deep Copier 유틸리티 ---
    class  deep_copier {
    public:
        template <typename T>
        static T copy(const T& obj) {
            serializer s;
            const_cast<T&>(obj).serialize(s);

            deserializer d(s.buffer);
            T new_obj;
            new_obj.deserialize(d);

            return new_obj;
        }
    };

} // namespace mino::core::memory