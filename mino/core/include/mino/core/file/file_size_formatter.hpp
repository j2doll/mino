#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace mino::core::file {

    enum class metric_type {
        iec = 1024, // IEC 단위계 (1024 기반) (디폴트)
        si = 1000   // SI 단위계 (1000 기반)
    };

    class file_size_formatter {
    public:
        // 바이트를 문자열로 변환 (소수점 아래는 디폴트 2자리, metric_type은 디폴트 IEC)
        static std::string byte_to_string(uint64_t bytes, metric_type metric = metric_type::iec, int precision = 2) noexcept;

        // 문자열을 바이트 단위로 변환
        static std::optional<uint64_t> string_to_bytes(std::string_view input_str, metric_type metric = metric_type::iec) noexcept;

        // 파일 경로를 받아 파일 크기를 문자열로 반환 (실패시 std::nullopt 반환)
        static std::optional<std::string> get_filesize_string(const std::filesystem::path& file_path, metric_type metric = metric_type::iec, int precision = 2) noexcept;
    };

} // namespace mino::core::file
