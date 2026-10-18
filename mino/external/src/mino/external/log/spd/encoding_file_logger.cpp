#include "mino/external/log/spd/encoding_file_logger.hpp"
#include <cstdio>

namespace mino::external::log::spd {

    // ------------------------------------------------------------------------
    // UTF-8 -> CP949 인코딩 변환 함수 (Windows)
    // ------------------------------------------------------------------------
    std::string convert_utf8_to_cp949(const std::string& utf8_str) {
#ifdef _WIN32
        if (utf8_str.empty()) {
            return {};
        }

        // 1. UTF-8 -> UTF-16 (WideChar)
        int wide_len = ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
        if (wide_len <= 0) {
            return utf8_str;
        }

        std::wstring wide_str(wide_len, 0);
        ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), &wide_str[0], wide_len);

        // 2. UTF-16 -> CP949 (EUC-KR)
        int cp949_len = ::WideCharToMultiByte(949, 0, wide_str.data(), wide_len, nullptr, 0, nullptr, nullptr);
        if (cp949_len <= 0) {
            return utf8_str;
        }

        std::string cp949_str(cp949_len, 0);
        ::WideCharToMultiByte(949, 0, wide_str.data(), wide_len, &cp949_str[0], cp949_len, nullptr, nullptr);

        return cp949_str;
#else
        return utf8_str;
#endif
    }

    // ------------------------------------------------------------------------
    // encoding_file_sink<Mutex> 구현
    // ------------------------------------------------------------------------
    template<typename Mutex>
    encoding_file_sink<Mutex>::encoding_file_sink(
        const std::filesystem::path& filename,
        log_encoding encoding,
        line_ending ending,
        bool write_bom)
        : encoding_(encoding), line_ending_(ending)
    {
        // 1. 디렉터리 생성 (예외 없이 std::error_code 사용)
        if (filename.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(filename.parent_path(), ec);
            if (ec) {
                std::fprintf(stderr, "[spdlog error] Failed to create directory %s: %s\n",
                    filename.parent_path().string().c_str(), ec.message().c_str());
            }
        }

        // 2. 파일 열기 시도  
        file_stream_.open(filename, std::ios::out | std::ios::binary | std::ios::app);
        if (!file_stream_) {
            std::fprintf(stderr, "[spdlog error] Failed to open file: %s\n", filename.string().c_str());
            return;
        }

        // 3. 파일 열기에 성공한 경우에만 BOM 기록
        if (write_bom && file_stream_.tellp() == 0) {
            if (encoding_ == log_encoding::utf8) {
                const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
                file_stream_.write(reinterpret_cast<const char*>(utf8_bom), sizeof(utf8_bom));
            }
        }

        this->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%+"));
    }

    template<typename Mutex>
    void encoding_file_sink<Mutex>::sink_it_(const ::spdlog::details::log_msg& msg) {
        if (!file_stream_.is_open()) {
            return;
        }

        // 포맷팅된 메시지 획득
        ::spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        std::string text(formatted.data(), formatted.size());

        // 줄바꿈 문자 변환
        if (line_ending_ == line_ending::crlf) {
            std::string crlf_text;
            crlf_text.reserve(text.size() + 16);
            for (size_t i = 0; i < text.size(); ++i) {
                if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) {
                    crlf_text += "\r\n";
                }
                else {
                    crlf_text += text[i];
                }
            }
            text = std::move(crlf_text);
        }
        else if (line_ending_ == line_ending::cr) {
            for (char& c : text) {
                if (c == '\n') c = '\r';
            }
        }

        // 인코딩 변환
        if (encoding_ == log_encoding::cp949) {
            text = convert_utf8_to_cp949(text);
        }

        file_stream_.write(text.data(), text.size());
    }

    template<typename Mutex>
    void encoding_file_sink<Mutex>::flush_() {
        if (file_stream_.is_open()) {
            file_stream_.flush();
        }
    }

    // ------------------------------------------------------------------------
    // 명시적 템플릿 인스턴스화
    // ------------------------------------------------------------------------
    template class encoding_file_sink<std::mutex>;
    template class encoding_file_sink<::spdlog::details::null_mutex>;

} // namespace mino::external::log::spd
