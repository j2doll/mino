#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <array>

namespace mino::core::zip {

    namespace fs = std::filesystem;

    // 압축 레벨 정의 (0: 무압축, 1: 가장 빠름, 3: 빠름, 6: 기본 밸런스, 9: 최대 압축)
    enum class compression_level {
        store = 0,
        fastest = 1,
        fast = 3,
        default_level = 6,
        maximum = 9
    };

    // 아카이브 파일명/경로 인코딩 정책
    enum class path_encoding {
        utf8,   // Bit 11 활성화 (Linux, 최신 압축기, UTF-8 표준)
        cp949   // Bit 11 비활성화 (레거시 Windows 기본 CP949 인식용)
    };

    // 진행률 콜백에 전달되는 정보 구조체
    struct progress_info {
        std::string current_entry_name;
        uint64_t bytes_processed;
        uint64_t total_bytes;
        int percentage; // 현재 퍼센트 (0 ~ 100)
    };

    // 진행률 콜백 함수 타입 정의 (false 반환 시 작업 취소 및 롤백)
    using progress_callback = std::function<bool(const progress_info&)>;

#pragma pack(push, 1)
    // ZIP 로컬 파일 헤더
    struct local_file_header {
        uint32_t signature = 0x04034b50;
        uint16_t min_version = 20;
        uint16_t flags = 0;
        uint16_t compression_method = 8;
        uint16_t last_mod_time = 0;
        uint16_t last_mod_date = 0;
        uint32_t crc32_val = 0;
        uint32_t compressed_size = 0;
        uint32_t uncompressed_size = 0;
        uint16_t filename_length = 0;
        uint16_t extra_field_length = 0;
    };

    // ZIP 센트럴 디렉터리 헤더
    struct central_directory_header {
        uint32_t signature = 0x02014b50;
        uint16_t version_made_by = 20;
        uint16_t min_version = 20;
        uint16_t flags = 0;
        uint16_t compression_method = 8;
        uint16_t last_mod_time = 0;
        uint16_t last_mod_date = 0;
        uint32_t crc32_val = 0;
        uint32_t compressed_size = 0;
        uint32_t uncompressed_size = 0;
        uint16_t filename_length = 0;
        uint16_t extra_field_length = 0;
        uint16_t comment_length = 0;
        uint16_t disk_number_start = 0;
        uint16_t internal_attr = 0;
        uint32_t external_attr = 0;
        uint32_t local_header_offset = 0;
    };

    // ZIP EOCD (End of Central Directory)
    struct eocd_record {
        uint32_t signature = 0x06054b50;
        uint16_t disk_number = 0;
        uint16_t start_disk = 0;
        uint16_t num_entries_this_disk = 0;
        uint16_t total_entries = 0;
        uint32_t central_directory_size = 0;
        uint32_t central_directory_offset = 0;
        uint16_t comment_length = 0;
    };
#pragma pack(pop)

    namespace detail {

        // DEFLATE 알고리즘 파라미터 구조체 선언
        struct deflate_params {
            size_t window_size;
            size_t max_chain;
            size_t good_length;
        };

        // DEFLATE 비트 단위 쓰기 클래스 선언
        class bit_writer {
        public:
            void write_bits(uint32_t bits, int count);
            void flush();
            std::vector<uint8_t>& get_bytes();

        private:
            std::vector<uint8_t> out_bytes;
            uint32_t bit_buffer = 0;
            int bit_count = 0;
        };

        // 내부 헬퍼 함수 선언
        uint32_t calculate_crc32(const std::vector<uint8_t>& data);
        void write_huffman_code(bit_writer& bw, uint32_t code, int length);
        void write_fixed_literal(bit_writer& bw, uint32_t lit);
        void write_fixed_distance(bit_writer& bw, uint32_t dist_code);
        deflate_params get_params_for_level(compression_level level);
        std::vector<uint8_t> compress_deflate(const std::vector<uint8_t>& input,
            compression_level level,
            const std::string& entry_name,
            const progress_callback& callback,
            int percent_step);

    } // namespace detail

    // ZIP 아카이버 클래스 선언
    class zip_archiver {
    public:
        zip_archiver();
        ~zip_archiver();

        zip_archiver(const zip_archiver&) = delete;
        zip_archiver& operator=(const zip_archiver&) = delete;

        zip_archiver(zip_archiver&&) noexcept;
        zip_archiver& operator=(zip_archiver&&) noexcept;

        void init(const fs::path& zip_path,
            bool overwrite = true,
            compression_level default_lvl = compression_level::default_level,
            path_encoding default_enc = path_encoding::utf8);

        void set_progress_callback(progress_callback callback, int percent_step = 1);

        void add_file_data(const std::string& raw_archive_path,
            const std::vector<uint8_t>& content);
        void add_file_data(const std::string& raw_archive_path,
            const std::vector<uint8_t>& content,
            compression_level level);

        void add_file_from_disk(const fs::path& disk_file_path,
            const std::string& raw_archive_path);
        void add_file_from_disk(const fs::path& disk_file_path,
            const std::string& raw_archive_path,
            compression_level level);

        void finish();

    private:
        void ensure_initialized_and_active() const;
        std::string sanitize_archive_path(const std::string& input_path) const;

        fs::path target_path;
        std::ofstream zip_out;
        std::vector<central_directory_header> cd_headers;
        std::vector<std::string> filenames;
        compression_level default_level = compression_level::default_level;
        path_encoding archive_encoding = path_encoding::utf8;
        progress_callback on_progress;
        int progress_step = 1;
        bool is_initialized = false;
        bool is_finished = false;
    };

} // namespace mino::core::zip
