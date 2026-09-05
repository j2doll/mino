#include "mino/core/zip/zip_archiver.hpp"

#include <algorithm>
#include <stdexcept>
#include <system_error>

namespace mino::core::zip {

    namespace detail {

        // -------------------------------------------------------------
        // CRC32 계산 정의
        // -------------------------------------------------------------
        uint32_t calculate_crc32(const std::vector<uint8_t>& data) {
            static const auto crc_table = []() {
                std::array<uint32_t, 256> table{};
                for (uint32_t i = 0; i < 256; ++i) {
                    uint32_t c = i;
                    for (int j = 0; j < 8; ++j) {
                        c = (c & 1) ? (0xEDB88320L ^ (c >> 1)) : (c >> 1);
                    }
                    table[i] = c;
                }
                return table;
                }();

            uint32_t crc = 0xFFFFFFFF;
            for (uint8_t byte : data) {
                crc = crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
            }
            return crc ^ 0xFFFFFFFF;
        }

        // -------------------------------------------------------------
        // bit_writer 멤버 함수 정의
        // -------------------------------------------------------------
        void bit_writer::write_bits(uint32_t bits, int count) {
            bit_buffer |= (bits << bit_count);
            bit_count += count;
            while (bit_count >= 8) {
                out_bytes.push_back(static_cast<uint8_t>(bit_buffer & 0xFF));
                bit_buffer >>= 8;
                bit_count -= 8;
            }
        }

        void bit_writer::flush() {
            if (bit_count > 0) {
                out_bytes.push_back(static_cast<uint8_t>(bit_buffer & 0xFF));
                bit_buffer = 0;
                bit_count = 0;
            }
        }

        std::vector<uint8_t>& bit_writer::get_bytes() {
            return out_bytes;
        }

        // -------------------------------------------------------------
        // 허프만 및 DEFLATE 헬퍼 함수 정의
        // -------------------------------------------------------------
        void write_huffman_code(bit_writer& bw, uint32_t code, int length) {
            uint32_t reversed = 0;
            for (int i = 0; i < length; ++i) {
                if ((code >> (length - 1 - i)) & 1) {
                    reversed |= (1 << i);
                }
            }
            bw.write_bits(reversed, length);
        }

        void write_fixed_literal(bit_writer& bw, uint32_t lit) {
            if (lit <= 143) {
                write_huffman_code(bw, 0x030 + lit, 8);
            }
            else if (lit <= 255) {
                write_huffman_code(bw, 0x190 + (lit - 144), 9);
            }
            else if (lit <= 279) {
                write_huffman_code(bw, 0x000 + (lit - 256), 7);
            }
            else {
                write_huffman_code(bw, 0x0C0 + (lit - 280), 8);
            }
        }

        void write_fixed_distance(bit_writer& bw, uint32_t dist_code) {
            write_huffman_code(bw, dist_code, 5);
        }

        deflate_params get_params_for_level(compression_level level) {
            switch (level) {
            case compression_level::fastest:
                return { 4096, 4, 8 };
            case compression_level::fast:
                return { 8192, 16, 16 };
            case compression_level::default_level:
                return { 32768, 64, 32 };
            case compression_level::maximum:
                return { 32768, 4096, 258 };
            default:
                return { 32768, 64, 32 };
            }
        }

        std::vector<uint8_t> compress_deflate(const std::vector<uint8_t>& input,
            compression_level level,
            const std::string& entry_name,
            const progress_callback& callback,
            int percent_step) {
            const size_t in_size = input.size();
            int last_reported_pct = -1;

            auto report = [&](size_t current_pos, bool force = false) {
                if (!callback) return;

                int current_pct = (in_size == 0) ? 100 : static_cast<int>((current_pos * 100) / in_size);

                if (last_reported_pct == -1 || (current_pct - last_reported_pct >= percent_step) || force) {
                    if (force && current_pct == last_reported_pct) return;

                    last_reported_pct = current_pct;
                    progress_info info{ entry_name, current_pos, in_size, current_pct };
                    if (!callback(info)) {
                        throw std::runtime_error("Compression cancelled by user progress callback.");
                    }
                }
                };

            report(0);

            if (level == compression_level::store) {
                bit_writer bw;
                size_t offset = 0;

                if (in_size == 0) {
                    bw.write_bits(1, 1);
                    bw.write_bits(0, 2);
                    bw.flush();
                    auto& bytes = bw.get_bytes();
                    bytes.push_back(0); bytes.push_back(0);
                    bytes.push_back(0xFF); bytes.push_back(0xFF);
                    report(0, true);
                    return bytes;
                }

                while (offset < in_size) {
                    size_t block_len = std::min<size_t>(65535, in_size - offset);
                    bool is_final = (offset + block_len >= in_size);

                    bw.write_bits(is_final ? 1 : 0, 1);
                    bw.write_bits(0, 2);
                    bw.flush();

                    uint16_t len = static_cast<uint16_t>(block_len);
                    uint16_t nlen = ~len;

                    auto& bytes = bw.get_bytes();
                    bytes.push_back(len & 0xFF);
                    bytes.push_back((len >> 8) & 0xFF);
                    bytes.push_back(nlen & 0xFF);
                    bytes.push_back((nlen >> 8) & 0xFF);

                    bytes.insert(bytes.end(), input.begin() + offset, input.begin() + offset + block_len);
                    offset += block_len;

                    report(offset);
                }
                report(in_size, true);
                return bw.get_bytes();
            }

            deflate_params params = get_params_for_level(level);
            bit_writer bw;

            bw.write_bits(1, 1);
            bw.write_bits(1, 2);

            size_t pos = 0;
            const size_t min_match = 3;
            const size_t max_match = 258;

            while (pos < in_size) {
                report(pos);

                size_t best_len = 0;
                size_t best_dist = 0;

                size_t window_start = (pos > params.window_size) ? (pos - params.window_size) : 0;
                size_t max_len_possible = std::min(max_match, in_size - pos);

                if (max_len_possible >= min_match) {
                    size_t chain_count = 0;
                    for (size_t match_pos = pos; match_pos > window_start; --match_pos) {
                        if (++chain_count > params.max_chain) break;

                        size_t cand_pos = match_pos - 1;
                        size_t curr_len = 0;
                        while (curr_len < max_len_possible && input[cand_pos + curr_len] == input[pos + curr_len]) {
                            curr_len++;
                        }
                        if (curr_len > best_len) {
                            best_len = curr_len;
                            best_dist = pos - cand_pos;
                            if (best_len >= params.good_length || best_len == max_match) break;
                        }
                    }
                }

                if (best_len >= min_match) {
                    static const uint16_t length_bases[] = {
                        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
                    };
                    static const uint8_t length_extra_bits[] = {
                        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
                    };

                    int len_idx = 0;
                    for (int i = 0; i < 29; ++i) {
                        if (best_len >= length_bases[i]) {
                            len_idx = i;
                        }
                        else {
                            break;
                        }
                    }

                    uint32_t length_symbol = 257 + len_idx;
                    write_fixed_literal(bw, length_symbol);
                    if (length_extra_bits[len_idx] > 0) {
                        bw.write_bits(best_len - length_bases[len_idx], length_extra_bits[len_idx]);
                    }

                    static const uint16_t dist_bases[] = {
                        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
                    };
                    static const uint8_t dist_extra_bits[] = {
                        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
                    };

                    int dist_idx = 0;
                    for (int i = 0; i < 30; ++i) {
                        if (best_dist >= dist_bases[i]) {
                            dist_idx = i;
                        }
                        else {
                            break;
                        }
                    }

                    write_fixed_distance(bw, dist_idx);
                    if (dist_extra_bits[dist_idx] > 0) {
                        bw.write_bits(best_dist - dist_bases[dist_idx], dist_extra_bits[dist_idx]);
                    }

                    pos += best_len;
                }
                else {
                    write_fixed_literal(bw, input[pos]);
                    pos++;
                }
            }

            write_fixed_literal(bw, 256);
            bw.flush();

            report(in_size, true);

            return bw.get_bytes();
        }

    } // namespace detail

    // -------------------------------------------------------------
    // zip_archiver 멤버 함수 정의
    // -------------------------------------------------------------
    zip_archiver::zip_archiver() = default;

    zip_archiver::~zip_archiver() {
        if (is_initialized && !is_finished) {
            try {
                if (zip_out.is_open()) {
                    zip_out.close();
                }
                std::error_code ec;
                fs::remove(target_path, ec);
            }
            catch (...) {}
        }
    }

    zip_archiver::zip_archiver(zip_archiver&& other) noexcept
        : target_path(std::move(other.target_path)),
        zip_out(std::move(other.zip_out)),
        cd_headers(std::move(other.cd_headers)),
        filenames(std::move(other.filenames)),
        default_level(other.default_level),
        archive_encoding(other.archive_encoding),
        on_progress(std::move(other.on_progress)),
        progress_step(other.progress_step),
        is_initialized(other.is_initialized),
        is_finished(other.is_finished) {
        other.is_finished = true;
        other.is_initialized = false;
    }

    zip_archiver& zip_archiver::operator=(zip_archiver&& other) noexcept {
        if (this != &other) {
            if (is_initialized && !is_finished && zip_out.is_open()) {
                zip_out.close();
                std::error_code ec;
                fs::remove(target_path, ec);
            }

            target_path = std::move(other.target_path);
            zip_out = std::move(other.zip_out);
            cd_headers = std::move(other.cd_headers);
            filenames = std::move(other.filenames);
            default_level = other.default_level;
            archive_encoding = other.archive_encoding;
            on_progress = std::move(other.on_progress);
            progress_step = other.progress_step;
            is_initialized = other.is_initialized;
            is_finished = other.is_finished;

            other.is_finished = true;
            other.is_initialized = false;
        }
        return *this;
    }

    void zip_archiver::init(const fs::path& zip_path,
        bool overwrite,
        compression_level default_lvl,
        path_encoding default_enc) {
        if (is_initialized) {
            throw std::logic_error("Archiver is already initialized.");
        }
        if (zip_path.empty()) {
            throw std::invalid_argument("Destination zip file path cannot be empty.");
        }

        std::error_code ec;

        if (fs::exists(zip_path, ec) && fs::is_directory(zip_path, ec)) {
            throw std::runtime_error("Destination path points to an existing directory: " + zip_path.string());
        }

        if (!overwrite && fs::exists(zip_path, ec)) {
            throw std::runtime_error("Destination zip file already exists and overwrite is set to false: " + zip_path.string());
        }

        if (zip_path.has_parent_path()) {
            fs::path parent_dir = zip_path.parent_path();
            if (!fs::exists(parent_dir, ec)) {
                fs::create_directories(parent_dir, ec);
                if (ec) {
                    throw std::runtime_error("Failed to create destination directories: " + parent_dir.string() + " (" + ec.message() + ")");
                }
            }
        }

        target_path = zip_path;
        default_level = default_lvl;
        archive_encoding = default_enc;
        is_finished = false;
        cd_headers.clear();
        filenames.clear();

        zip_out.exceptions(std::ios::badbit | std::ios::failbit);

        try {
            zip_out.open(target_path, std::ios::binary | std::ios::trunc);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to open output ZIP file: " + target_path.string() + " (" + e.what() + ")");
        }

        is_initialized = true;
    }

    void zip_archiver::set_progress_callback(progress_callback callback, int percent_step) {
        on_progress = std::move(callback);
        progress_step = std::clamp(percent_step, 1, 100);
    }

    void zip_archiver::ensure_initialized_and_active() const {
        if (!is_initialized) {
            throw std::logic_error("Archiver is not initialized. Call init() before performing operations.");
        }
        if (is_finished) {
            throw std::logic_error("Cannot modify a finished ZIP archive.");
        }
    }

    std::string zip_archiver::sanitize_archive_path(const std::string& input_path) const {
        if (input_path.empty()) {
            throw std::invalid_argument("Archive entry path cannot be empty.");
        }

        fs::path p(input_path);

        if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) {
            throw std::invalid_argument("Absolute paths are not allowed in ZIP entry: " + input_path);
        }

        for (const auto& part : p) {
            if (part == "..") {
                throw std::invalid_argument("Relative directory traversal ('..') is not allowed: " + input_path);
            }
        }

        return p.generic_string();
    }

    void zip_archiver::add_file_data(const std::string& raw_archive_path, const std::vector<uint8_t>& content) {
        add_file_data(raw_archive_path, content, default_level);
    }

    void zip_archiver::add_file_data(const std::string& raw_archive_path,
        const std::vector<uint8_t>& content,
        compression_level level) {
        ensure_initialized_and_active();

        std::string archive_path = sanitize_archive_path(raw_archive_path);
        if (archive_path.size() > 0xFFFF) {
            throw std::length_error("ZIP entry path is too long: " + archive_path);
        }

        uint32_t local_offset = static_cast<uint32_t>(zip_out.tellp());
        uint32_t file_crc = detail::calculate_crc32(content);
        std::vector<uint8_t> compressed = detail::compress_deflate(content, level, archive_path, on_progress, progress_step);

        uint16_t header_flags = (archive_encoding == path_encoding::utf8) ? 0x0800 : 0x0000;

        local_file_header lfh{};
        lfh.flags = header_flags;
        lfh.crc32_val = file_crc;
        lfh.compression_method = (level == compression_level::store) ? 0 : 8;
        lfh.compressed_size = static_cast<uint32_t>(compressed.size());
        lfh.uncompressed_size = static_cast<uint32_t>(content.size());
        lfh.filename_length = static_cast<uint16_t>(archive_path.size());

        try {
            zip_out.write(reinterpret_cast<const char*>(&lfh), sizeof(lfh));
            zip_out.write(archive_path.data(), archive_path.size());
            if (!compressed.empty()) {
                zip_out.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
            }
            zip_out.flush();
        }
        catch (const std::ios_base::failure& e) {
            throw std::runtime_error("Write failure during entry write: " + std::string(e.what()));
        }

        central_directory_header cdh{};
        cdh.flags = header_flags;
        cdh.crc32_val = file_crc;
        cdh.compression_method = lfh.compression_method;
        cdh.compressed_size = static_cast<uint32_t>(compressed.size());
        cdh.uncompressed_size = static_cast<uint32_t>(content.size());
        cdh.filename_length = static_cast<uint16_t>(archive_path.size());
        cdh.local_header_offset = local_offset;

        cd_headers.push_back(cdh);
        filenames.push_back(archive_path);
    }

    void zip_archiver::add_file_from_disk(const fs::path& disk_file_path, const std::string& raw_archive_path) {
        add_file_from_disk(disk_file_path, raw_archive_path, default_level);
    }

    void zip_archiver::add_file_from_disk(const fs::path& disk_file_path,
        const std::string& raw_archive_path,
        compression_level level) {
        ensure_initialized_and_active();

        std::error_code ec;
        if (!fs::exists(disk_file_path, ec) || !fs::is_regular_file(disk_file_path, ec)) {
            throw std::runtime_error("Target file does not exist or is not a regular file: " + disk_file_path.string());
        }

        uintmax_t file_size = fs::file_size(disk_file_path, ec);
        if (ec) {
            throw std::runtime_error("Failed to query file size: " + disk_file_path.string());
        }

        if (file_size >= 0xFFFFFFFFULL) {
            throw std::overflow_error("File size exceeds 4GB standard ZIP limit: " + disk_file_path.string());
        }

        std::ifstream file(disk_file_path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open source file: " + disk_file_path.string());
        }

        std::vector<uint8_t> buffer(file_size);
        if (file_size > 0) {
            file.read(reinterpret_cast<char*>(buffer.data()), file_size);
            if (!file) {
                throw std::runtime_error("Failed to read complete file content: " + disk_file_path.string());
            }
        }

        add_file_data(raw_archive_path, buffer, level);
    }

    void zip_archiver::finish() {
        ensure_initialized_and_active();

        try {
            uint32_t cd_offset = static_cast<uint32_t>(zip_out.tellp());

            for (size_t i = 0; i < cd_headers.size(); ++i) {
                zip_out.write(reinterpret_cast<const char*>(&cd_headers[i]), sizeof(central_directory_header));
                zip_out.write(filenames[i].data(), filenames[i].size());
            }

            uint32_t cd_size = static_cast<uint32_t>(zip_out.tellp()) - cd_offset;

            eocd_record eocd{};
            eocd.num_entries_this_disk = static_cast<uint16_t>(cd_headers.size());
            eocd.total_entries = static_cast<uint16_t>(cd_headers.size());
            eocd.central_directory_size = cd_size;
            eocd.central_directory_offset = cd_offset;

            zip_out.write(reinterpret_cast<const char*>(&eocd), sizeof(eocd));
            zip_out.flush();
            zip_out.close();

            is_finished = true;
        }
        catch (const std::ios_base::failure& e) {
            throw std::runtime_error("Write failure during finalize (EOCD): " + std::string(e.what()));
        }
    }

} // namespace mino::core::zip
