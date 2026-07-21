#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <functional>
#include <locale>
#include <codecvt>
#include <regex>
#include <unordered_set>
#include <future>

#include "mino/external/log/spd/encoding_file_logger.hpp"

#define MINIZ_CPP_HEADER_ONLY
#include "mino/external/log/spd/zip_file.hpp"

// detect spdlog version at compile time and provide a fallback for older spdlog
#ifndef SPDLOG_VER_MAJOR
#  define MINO_SPDLOG_OLD 1
#elif (SPDLOG_VER_MAJOR * 10000 + SPDLOG_VER_MINOR * 100 + SPDLOG_VER_PATCH) < 10800
#  define MINO_SPDLOG_OLD 1
#else
#  define MINO_SPDLOG_NEW 1
#endif

namespace mino::external::log::spd {

    // ZIP 처리 워커: 시작/종료/예외 로깅 추가
    static std::mutex g_zip_mu;
    static std::unordered_set<std::string> g_zip_in_progress;

    static void process_zip_archive(
        const std::filesystem::path& target_file,
        bool delete_on_failure,
        int comp_level,
        std::size_t max_zip_count,
        time_zone_type tz,
        std::shared_ptr<::spdlog::logger> err_log,
        const std::string& original_source = std::string())
    {
        if (err_log) err_log->info("[ZipWorker][START] start for '{}'", target_file.string());

        namespace fs = std::filesystem;
        (void)comp_level;

        try {
            if (!fs::exists(target_file)) {
                if (err_log) err_log->warn("[ZipWorker][END] target not exists: '{}'", target_file.string());
                return;
            }
            fs::path log_dir = target_file.parent_path();

            // 대상 스템 기준으로 해당 그룹의 zip만 후보로 수집
            std::string target_stem = target_file.stem().string(); // e.g. "all.2" or "all.2.zip_tmp_..." or "all.2_YYYYMMDD_HHMMSS"
            std::string base_prefix = target_stem;
            {
                std::smatch m;
                // zip_tmp_<epoch> 형식이면 제거
                if (std::regex_match(target_stem, m, std::regex(R"(^(.+)\.zip_tmp_\d+$)"))) {
                    base_prefix = m[1].str();
                }
                // 날짜 접미사 _YYYYMMDD_HHMMSS 형식이면 제거
                else if (std::regex_match(target_stem, m, std::regex(R"(^(.+)_\d{8}_\d{6}$)"))) {
                    base_prefix = m[1].str();
                }
                // 그 외는 그대로 사용
            }

            std::vector<fs::directory_entry> zip_files;
            for (const auto& entry : fs::directory_iterator(log_dir)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                if (entry.path().extension() != ".zip") {
                    continue;
                }

                // 파일명(확장자 제거) 기준으로 base_prefix로 시작하는지 검사
                const std::string fname_no_ext = entry.path().stem().string();
                if (fname_no_ext.rfind(base_prefix, 0) != 0) {
                    continue;
                }

                zip_files.push_back(entry);
            }

            std::sort(zip_files.begin(), zip_files.end(), [](const auto& a, const auto& b) {
                return fs::last_write_time(a) < fs::last_write_time(b);
                });

            if (!zip_files.empty() && zip_files.size() >= max_zip_count) {
                std::size_t files_to_delete = (zip_files.size() - max_zip_count) + 1;
                for (std::size_t i = 0; i < files_to_delete; ++i) {
                    try {
                        fs::remove(zip_files[i].path());
                        if (err_log) err_log->info("[ZipWorker] removed old zip: '{}'", zip_files[i].path().string());
                    }
                    catch (const std::exception& e) {
                        if (err_log) err_log->error("[ZipWorker] failed remove old zip '{}': {}", zip_files[i].path().string(), e.what());
                    }
                }
            }

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm buf;

            if (tz == time_zone_type::utc) {
#if defined(_WIN32)
                gmtime_s(&buf, &in_time_t);
#else
                gmtime_r(&in_time_t, &buf);
#endif
            }
            else {
#if defined(_WIN32)
                localtime_s(&buf, &in_time_t);
#else
                localtime_r(&in_time_t, &buf);
#endif
            }

            char time_str[32];
            std::strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S", &buf);

            // 만약 target_file의 stem이 이미 _YYYYMMDD_HHMMSS 형식이면 그 stem 그대로 사용하여 zip 생성
            fs::path zip_file;
            {
                std::smatch m;
                if (std::regex_match(target_stem, m, std::regex(R"(^(.+)_\d{8}_\d{6}$)"))) {
                    // stem already contains date suffix -> use it directly
                    zip_file = log_dir / (target_stem + ".zip");
                }
                else {
                    // 기존 동작: stem + 현재 시간
                    zip_file = log_dir / (target_file.stem().string() + time_str + ".zip");
                }
            }

            // miniz-cpp 사용: 파일을 zip에 추가
            miniz_cpp::zip_file archive;
            if (err_log) err_log->info("[ZipWorker] archiving '{}' -> '{}'", target_file.string(), zip_file.string());

            auto target_file_name = target_file.string();
            archive.write(target_file_name);

            auto zip_file_name = zip_file.string();
            archive.save(zip_file_name);

            if (err_log) err_log->info("[ZipWorker] saved zip '{}'", zip_file_name);

            // 임시 복사본(target_file)은 삭제
            fs::remove(target_file);
            if (err_log) err_log->info("[ZipWorker] removed source '{}'", target_file.string());
        }
        catch (const std::exception& e) {
            if (err_log) {
                err_log->error("[ZipWorker] exception for '{}': {}", target_file.string(), e.what());
            }
            try {
                if (delete_on_failure && std::filesystem::exists(target_file)) {
                    std::filesystem::remove(target_file);
                    if (err_log) err_log->warn("[ZipWorker] deleted source on failure '{}'", target_file.string());
                }
            }
            catch (...) {
                if (err_log) err_log->error("[ZipWorker] failed to delete source after exception for '{}'", target_file.string());
            }
        }

        // 작업 완료시 original_source 가 주어졌으면 진행중 집합에서 제거
        if (!original_source.empty()) {
            std::lock_guard<std::mutex> lk(g_zip_mu);
            auto it = g_zip_in_progress.find(original_source);
            if (it != g_zip_in_progress.end()) {
                g_zip_in_progress.erase(it);
                if (err_log) err_log->info("[ZipWorker] removed '{}' from in-progress set", original_source);
            }
        }

        if (err_log) err_log->info("[ZipWorker][END] end for '{}'", target_file.string());
    }


    // encoding_file_sink 구현 (생략 없이 포함)
    template<typename Mutex>
    encoding_file_sink<Mutex>::encoding_file_sink(const std::filesystem::path& filename, log_encoding encoding, line_ending ending, bool write_bom)
        : encoding_(encoding), line_ending_(ending)
    {
        if (filename.has_parent_path()) {
            std::filesystem::create_directories(filename.parent_path());
        }

        file_stream_.open(filename, std::ios::out | std::ios::binary | std::ios::app);
        if (!file_stream_) {
            throw ::spdlog::spdlog_ex("Failed to open file: " + filename.string());
        }

        if (write_bom && file_stream_.tellp() == 0) {
            if (encoding_ == log_encoding::utf8) {
                const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
                file_stream_.write(reinterpret_cast<const char*>(utf8_bom), 3);
            }
        }

        this->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%+"));
    }
        

    template<typename Mutex>
    void encoding_file_sink<Mutex>::sink_it_(const ::spdlog::details::log_msg& msg) {
        ::spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        std::string raw_str(formatted.data(), formatted.size());

        while (!raw_str.empty() && (raw_str.back() == '\n' || raw_str.back() == '\r')) {
            raw_str.pop_back();
        }

        switch (line_ending_) {
        case line_ending::lf:   raw_str += "\n"; break;
        case line_ending::crlf: raw_str += "\r\n"; break;
        case line_ending::cr:   raw_str += "\r"; break;
        }

        if (encoding_ == log_encoding::cp949) {
            std::string cp949_str = convert_utf8_to_cp949(raw_str);
            file_stream_.write(cp949_str.data(), cp949_str.size());
        }
        else {
            file_stream_.write(raw_str.data(), raw_str.size());
        }
    }

    template<typename Mutex>
    void encoding_file_sink<Mutex>::flush_() {
        file_stream_.flush();
    }

    // encoding_rotating_zipping_sink 구현
    template<typename Mutex>
    encoding_rotating_zipping_sink<Mutex>::encoding_rotating_zipping_sink(
        const std::filesystem::path& filename,
        std::size_t max_size,
        std::size_t max_files,
        log_encoding encoding,
        line_ending ending,
        bool write_bom,
        bool delete_on_failure,
        int compression_level,
        std::size_t max_zip_count,
        time_zone_type timezone)
        :
            encoding_(encoding),
            line_ending_(ending),
            write_bom_(write_bom),
            max_files_(max_files),
            delete_on_failure_(delete_on_failure),
            compression_level_(std::clamp(compression_level, 0, 9)),
            max_zip_count_(max_zip_count),
            timezone_(timezone),
            skip_handler_(std::make_shared<std::atomic<bool>>(false)) // use shared atomic
    {
        auto console_sink = std::make_shared<::spdlog::sinks::stdout_color_sink_mt>();
        console_logger_ = std::make_shared<::spdlog::logger>("encoding_rotating_zipping_sink", console_sink);

        // capture variables used by both new-handler and fallback branch
        std::size_t capture_max_files = max_files;
        int capture_comp_level = compression_level_;
        std::size_t capture_max_zip_count = max_zip_count_;
        time_zone_type capture_tz = timezone_;
        bool capture_delete_on_failure = delete_on_failure_;
        auto console_log_copy = console_logger_;

        // create local shared flag copy for lambdas
        auto skip_flag = skip_handler_;

#if defined(MINO_SPDLOG_NEW)
        ::spdlog::file_event_handlers handlers;

        /*
        // after_close handler: capture skip_flag (not this)
        handlers.after_close = [
            skip_flag,
            capture_max_files,
            capture_comp_level,
            capture_max_zip_count,
            capture_tz,
            capture_delete_on_failure,
            console_log_copy]
            (const ::spdlog::filename_t& closed_filename)
        {
            if (skip_flag && skip_flag->load()) {
                if (console_log_copy) console_log_copy->info("[after_close] skipped due to skip_handler_ flag");
                return;
            }

            try {
                namespace fs = std::filesystem;
                fs::path p(closed_filename);
                if (console_log_copy) console_log_copy->info("[after_close] called with '{}'", p.string());

                if (p.empty()) {
                    if (console_log_copy) console_log_copy->warn("[after_close] closed filename empty");
                    return;
                }

                // closed_filename의 stem이 "base.N" 형식인지 검사
                std::string stem = p.stem().string(); // e.g. "all.2"
                std::regex re(R"(^(.+)\.(\d+)$)");
                std::smatch m;
                auto m_size = m.size();
                auto match_redult = std::regex_match(stem, m, re);
                if (match_redult && m_size >= 3) {
                    // 추출: base와 인덱스
                    std::string base = m[1].str();
                    std::size_t idx = 0;
                    try { idx = static_cast<std::size_t>(std::stoull(m[2].str())); }
                    catch (...) { idx = 0; }

                    if (capture_max_files == 0) {
                        if (console_log_copy) console_log_copy->warn("[after_close] capture_max_files == 0, skip");
                        return;
                    }

                    // 기존: 모든 인덱스에 대해 스케줄하는 변경으로 인해 중복 스케줄이 발생할 수 있음.
                    // 여기서는 인덱스가 0이면 스킵(비정상), 그 외에는 스케줄 시 g_zip_in_progress 체크/등록을 하도록 함.
                    if (idx == 0) {
                        if (err_log_copy) err_log_copy->info("[after_close] '{}' index {} is not target {} - skip", p.string(), idx, capture_max_files);
                        return;
                    }

                    // 중복 스케줄 방지: before_close와 동일하게 in-progress 집합에 등록
                    std::string orig_key = p.string();
                    {
                        std::lock_guard<std::mutex> lk(g_zip_mu);
                        if (g_zip_in_progress.find(orig_key) != g_zip_in_progress.end()) {
                            if (console_log_copy) console_log_copy->info("[after_close] compression already scheduled for '{}', skip", orig_key);
                            return;
                        }
                        g_zip_in_progress.insert(orig_key);
                        if (console_log_copy) console_log_copy->info("[after_close] marked '{}' as in-progress", orig_key);
                    }

                    if (fs::exists(p) && fs::is_regular_file(p)) {
                        if (console_log_copy) console_log_copy->info("[after_close] schedule zip for latest '{}'", p.string());
                        std::thread([
                            p,
                            capture_delete_on_failure,
                            capture_comp_level,
                            capture_max_zip_count,
                            capture_tz,
                            err_log_copy,
                            orig_key]()
                            {
                                // original_source 파라미터로 orig_key 전달하여
                                // 작업 완료 시 in-progress에서 제거하게 함
                                process_zip_archive(
                                    p,
                                    capture_delete_on_failure,
                                    capture_comp_level,
                                    capture_max_zip_count,
                                    capture_tz,
                                    err_log_copy,
                                    orig_key);
                            }).detach();
                    }
                }
                else {
                    if (err_log_copy) err_log_copy->info("[after_close] closed stem '{}' does not match base.N pattern : no zip scheduled", stem);
                }
            }
            catch (const std::exception& e) {
                if (err_log_copy) err_log_copy->error("[after_close] exception: {}", e.what());
            }

        }; // after_close 람다
        //*/

        // rotating sink: before_close 람다 (두 번째 FILE* 파라미터 포함)
        handlers.before_close = [
            skip_flag,
            capture_max_files,
            capture_comp_level,
            capture_max_zip_count,
            capture_tz,
            capture_delete_on_failure,
            console_log_copy]
            (const ::spdlog::filename_t& closed_filename, FILE* /*fp*/)
        {
            if (skip_flag && skip_flag->load()) {
                if (console_log_copy)
                    console_log_copy->debug("[before_close] skipped due to skip_handler_ flag");
                return;
            }

            try {
                namespace fs = std::filesystem;
                fs::path p(closed_filename);
                if (console_log_copy) console_log_copy->info("[before_close] called with '{}'", p.string());

                if (p.empty()) {
                    if (console_log_copy) console_log_copy->warn("[before_close] closed filename empty");
                    return;
                }

                // stem 검사: base.N 혹은 base
                std::string stem = p.stem().string();           // e.g. "all.2" or "all"
                std::string base_stem;
                std::size_t idx_val = 0;
                bool stem_has_index = false;

                std::regex re_index(R"(^(.+)\.(\d+)$)");
                std::smatch m;
                if (std::regex_match(stem, m, re_index) && m.size() >= 3) {
                    base_stem = m[1].str();
                    try { idx_val = static_cast<std::size_t>(std::stoull(m[2].str())); }
                    catch (...) { idx_val = 0; }
                    stem_has_index = true;
                }
                else {
                    base_stem = stem; // closed is base (no numeric suffix)
                }

                fs::path candidate;
                if (stem_has_index) {
                    // closed_filename already indexed -> only proceed if this index equals configured max_files
                    if (capture_max_files == 0) {
                        if (console_log_copy) console_log_copy->warn("[before_close] capture_max_files == 0, skip");
                        return;
                    }
                    if (idx_val != capture_max_files) {
                        if (console_log_copy) console_log_copy->info("[before_close] '{}' index {} is not target {} - skip", p.string(), idx_val, capture_max_files);
                        return;
                    }

                    // candidate는 p 자체
                    if (fs::exists(p) && fs::is_regular_file(p)) candidate = p;
                    else {
                        if (console_log_copy) console_log_copy->warn("[before_close] indexed closed file not found: '{}'", p.string());
                        return;
                    }
                }
                else {
                    // closed was base file (e.g. "all.log") -> pick the file with index == capture_max_files
                    if (capture_max_files == 0) {
                        if (console_log_copy) console_log_copy->warn("[before_close] capture_max_files == 0, skip");
                        return;
                    }
                    fs::path dir = p.parent_path();
                    std::string ext = p.extension().string(); // ".log"
                    std::string target_name = base_stem + "." + std::to_string(capture_max_files) + ext;
                    fs::path target_path = dir / target_name;
                    if (fs::exists(target_path) && fs::is_regular_file(target_path)) {
                        candidate = target_path;
                    }
                    else {
                        if (console_log_copy) console_log_copy->info("[before_close] target indexed file '{}' not found for base '{}'", target_path.string(), base_stem);
                        return;
                    }
                }

                // candidate 가 결정되면 임시 복사본을 만들고 압축 스레드로 넘김
                // candidate 결정 후 (candidate가 롤된 실제 파일 경로일 때)
                std::string orig_key = candidate.string();
                {
                    std::lock_guard<std::mutex> lk(g_zip_mu);
                    if (g_zip_in_progress.find(orig_key) != g_zip_in_progress.end()) {
                        if (console_log_copy) console_log_copy->info("[before_close] compression already scheduled for '{}', skip", orig_key);
                        return;
                    }
                    // 등록: 진행중으로 마크
                    g_zip_in_progress.insert(orig_key);
                    if (console_log_copy) console_log_copy->info("[before_close] marked '{}' as in-progress", orig_key);
                }

                // 임시 복사본 생성 (tmp) 후 스레드 실행
                try {
                    // 임시 파일명 생성 (날짜 기반)
                    auto now = std::chrono::system_clock::now();
                    auto in_time_t = std::chrono::system_clock::to_time_t(now);
                    std::tm buf;
                    if (capture_tz == time_zone_type::utc) {
#if defined(_WIN32)
                        gmtime_s(&buf, &in_time_t);
#else
                        gmtime_r(&in_time_t, &buf);
#endif
                    }
                    else {
#if defined(_WIN32)
                        localtime_s(&buf, &in_time_t);
#else
                        localtime_r(&in_time_t, &buf);
#endif
                    }
                    char time_str[32];
                    std::strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S", &buf);

                    fs::path tmp =
                        candidate.parent_path() / (candidate.stem().string()
                            + time_str
                            + candidate.extension().string());

                    fs::copy_file(candidate, tmp, fs::copy_options::overwrite_existing);
                    if (console_log_copy) console_log_copy->info("[before_close] copied '{}' -> '{}' (for zip)", candidate.string(), tmp.string());
                    {
                        // 변경(람다로 캡처):
                        auto tmp_copy = tmp;
                        auto orig_copy = orig_key;
                        auto console_log_copy_local = console_log_copy;

                        // called process_zip_archive in a separate thread to avoid blocking the logging thread
                        std::thread([
                            tmp_copy,
                            capture_delete_on_failure,
                            capture_comp_level,
                            capture_max_zip_count,
                            capture_tz,
                            console_log_copy_local,
                            orig_copy]() {
                                process_zip_archive(
                                    tmp_copy,
                                    capture_delete_on_failure,
                                    capture_comp_level,
                                    capture_max_zip_count,
                                    capture_tz,
                                    console_log_copy_local,
                                    orig_copy);
                            }).detach();
                    }
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lk(g_zip_mu);
                    g_zip_in_progress.erase(orig_key);
                    if (console_log_copy) console_log_copy->error("[before_close] copy failed for '{}': {}", candidate.string(), e.what());
                }
            }
            catch (const std::exception& e) {
                if (console_log_copy) console_log_copy->error("[before_close] exception: {}", e.what());
            }
        }; // before_close 람다

        backend_sink_ = std::make_shared<::spdlog::sinks::rotating_file_sink<Mutex>>(filename.string(), max_size, max_files, false, handlers);
        backend_sink_->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%v"));
#else
        // Older spdlog fallback: cannot attach file event handlers -> create rotating sink without handlers.
        // Zipping-on-close behavior is disabled for older spdlog versions.
        backend_sink_ = std::make_shared<::spdlog::sinks::rotating_file_sink<Mutex>>(filename.string(), max_size, max_files);
        backend_sink_->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%v"));
#endif

        if (write_bom_ && encoding_ == log_encoding::utf8) {
            if (std::filesystem::exists(filename) && std::filesystem::file_size(filename) == 0) {
                std::ofstream fs(filename, std::ios::binary | std::ios::app);
                const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
                fs.write(reinterpret_cast<const char*>(utf8_bom), 3);
            }
        }

        this->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%+"));
    } 

    template<typename Mutex>
    encoding_rotating_zipping_sink<Mutex>::~encoding_rotating_zipping_sink() {

        this->set_skip_handler(true); // 소멸자에서 after_close/before_close 핸들러 스킵

        // 소멸자에서 진행중 집합을 비우고, 필요시 로그
        std::lock_guard<std::mutex> lk(g_zip_mu);
        if (!g_zip_in_progress.empty()) {
            if (console_logger_) {
                for (const auto& item : g_zip_in_progress) {
                    console_logger_->warn("[~encoding_rotating_zipping_sink] still in-progress: '{}'", item);
                }
            }
            g_zip_in_progress.clear();
        }
    }

    // set_skip_handler 구현 (rotating)
    template<typename Mutex>
    void encoding_rotating_zipping_sink<Mutex>::set_skip_handler(bool skip) {
        if (!skip_handler_) skip_handler_ = std::make_shared<std::atomic<bool>>(skip);
        else skip_handler_->store(skip);
    }
    
    template<typename Mutex>
    void encoding_rotating_zipping_sink<Mutex>::sink_it_(const ::spdlog::details::log_msg& msg) {
        ::spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        std::string raw_str(formatted.data(), formatted.size());

        while (!raw_str.empty() && (raw_str.back() == '\n' || raw_str.back() == '\r')) {
            raw_str.pop_back();
        }

        switch (line_ending_) {
        case line_ending::lf:   raw_str += "\n"; break;
        case line_ending::crlf: raw_str += "\r\n"; break;
        case line_ending::cr:   raw_str += "\r"; break;
        }

        ::spdlog::memory_buf_t output_buffer;
        if (encoding_ == log_encoding::cp949) {
            std::string cp949_str = convert_utf8_to_cp949(raw_str);
            output_buffer.append(cp949_str.data(), cp949_str.data() + cp949_str.size());
        }
        else {
            output_buffer.append(raw_str.data(), raw_str.data() + raw_str.size());
        }

        ::spdlog::details::log_msg forward_msg = msg;
        // Use portable construction rather than internal detail helper
        forward_msg.payload = ::spdlog::string_view_t(output_buffer.data(), output_buffer.size());
        backend_sink_->log(forward_msg);
    }

    template<typename Mutex>
    void encoding_rotating_zipping_sink<Mutex>::flush_() {
        backend_sink_->flush();
    }

    // encoding_daily_zipping_sink: after_close 동일한 방식 적용
    template<typename Mutex, typename FileNameCalc>
    encoding_daily_zipping_sink<Mutex, FileNameCalc>::encoding_daily_zipping_sink(
        const std::filesystem::path& filename,
        int rotation_hour,
        int rotation_minute,
        log_encoding encoding,
        line_ending ending,
        bool write_bom,
        bool delete_on_failure,
        int compression_level,
        std::size_t max_zip_count,
        uint32_t max_files,
        time_zone_type timezone)
        :
            encoding_(encoding),
            line_ending_(ending),
            write_bom_(write_bom),
            delete_on_failure_(delete_on_failure),
            compression_level_(std::clamp(compression_level, 0, 9)),
            max_zip_count_(max_zip_count),
            timezone_(timezone),
            skip_handler_(std::make_shared<std::atomic<bool>>(false)) // add shared atomic initialization
    {
        auto console_sink = std::make_shared<::spdlog::sinks::stdout_color_sink_mt>();
        console_logger_ = std::make_shared<::spdlog::logger>("encoding_daily_zipping_sink", console_sink);

        std::size_t capture_max_files = max_files;
        int capture_comp_level = compression_level_;
        std::size_t capture_max_zip_count = max_zip_count_;
        time_zone_type capture_tz = timezone_;
        bool capture_delete_on_failure = delete_on_failure_;
        auto console_log_copy = console_logger_;

        // create local shared flag copy for lambdas
        auto skip_flag = skip_handler_;

#if defined(MINO_SPDLOG_NEW)
        ::spdlog::file_event_handlers handlers;
        // before_close handler: capture skip_flag
        handlers.before_close = [
            skip_flag,
            capture_max_files,
            capture_comp_level,
            capture_max_zip_count,
            capture_tz,
            capture_delete_on_failure,
            console_log_copy]
            (const ::spdlog::filename_t& closed_filename, FILE* /*fp*/)
        {
            if (skip_flag && skip_flag->load()) {
                if (console_log_copy)
                    console_log_copy->debug("[daily_before_close] skipped due to skip_handler_ flag");
                return;
            }

            try {
                namespace fs = std::filesystem;
                fs::path p(closed_filename);
                if (console_log_copy) console_log_copy->info("[daily_before_close] called with '{}'", p.string());

                if (p.empty()) {
                    if (console_log_copy) console_log_copy->warn("[daily_before_close] closed filename empty");
                    return;
                }

                // stem 검사: base.N 혹은 base
                std::string stem = p.stem().string();
                std::string base_stem;
                std::size_t idx_val = 0;
                bool stem_has_index = false;

                std::regex re_index(R"(^(.+)\.(\d+)$)");
                std::smatch m;
                auto m_size = m.size();
                auto match_result = std::regex_match(stem, m, re_index);
                if (match_result && m_size >= 3) {
                    base_stem = m[1].str();
                    try { idx_val = static_cast<std::size_t>(std::stoull(m[2].str())); }
                    catch (...) { idx_val = 0; }
                    stem_has_index = true;
                }
                else {
                    base_stem = stem;
                }

                fs::path candidate;
                if (stem_has_index) {
                    if (capture_max_files == 0) {
                        if (console_log_copy) console_log_copy->warn("[daily_before_close] capture_max_files == 0, skip");
                        return;
                    }
                    if (idx_val != capture_max_files) {
                        if (console_log_copy) console_log_copy->info("[daily_before_close] '{}' index {} is not target {} - skip", p.string(), idx_val, capture_max_files);
                        return;
                    }

                    if (fs::exists(p) && fs::is_regular_file(p)) candidate = p;
                    else {
                        if (console_log_copy) console_log_copy->warn("[daily_before_close] indexed closed file not found: '{}'", p.string());
                        return;
                    }
                }
                else {
                    if (capture_max_files == 0) {
                        if (console_log_copy) console_log_copy->warn("[daily_before_close] capture_max_files == 0, skip");
                        return;
                    }
                    fs::path dir = p.parent_path();
                    std::string ext = p.extension().string();

                    // 변경: base_stem이 이미 날짜 접미사(예: _YYYY-MM-DD 또는 _YYYYMMDD) 혹은 _YYYYMMDD_HHMMSS 를 포함하는 경우,
                    //       해당 파일(p) 자체를 candidate로 사용하도록 함.
                    std::regex date_re1(R"(^(.+)_\d{4}-\d{2}-\d{2}$)");
                    std::regex date_re2(R"(^(.+)_\d{8}$)");
                    std::regex date_re3(R"(^(.+)_\d{8}_\d{6}$)");
                    if (std::regex_match(base_stem, m, date_re1) || std::regex_match(base_stem, m, date_re2) || std::regex_match(base_stem, m, date_re3)) {
                        if (fs::exists(p) && fs::is_regular_file(p)) {
                            candidate = p;
                        }
                        else {
                            if (console_log_copy) console_log_copy->info("[daily_before_close] date-stamped file '{}' not found for base '{}'", p.string(), base_stem);
                            return;
                        }
                    }
                    else {
                        std::string target_name = base_stem + "." + std::to_string(capture_max_files) + ext;
                        fs::path target_path = dir / target_name;

                        auto is_regular_file = fs::is_regular_file(target_path);
                        auto is_exists = fs::exists(target_path);
                        if (is_exists && is_regular_file) {
                            candidate = target_path;
                        }
                        else {
                            if (console_log_copy) console_log_copy->info("[daily_before_close] target indexed file '{}' not found for base '{}'", target_path.string(), base_stem);
                            return;
                        }
                    }
                }

                std::string orig_key = candidate.string();
                {
                    std::lock_guard<std::mutex> lk(g_zip_mu);
                    if (g_zip_in_progress.find(orig_key) != g_zip_in_progress.end()) {
                        if (console_log_copy) console_log_copy->info("[daily_before_close] compression already scheduled for '{}', skip", orig_key);
                        return;
                    }
                    g_zip_in_progress.insert(orig_key);
                    if (console_log_copy) console_log_copy->info("[daily_before_close] marked '{}' as in-progress", orig_key);
                }

                try {
                    // 시간 기반 tmp 파일명 생성
                    auto now = std::chrono::system_clock::now();
                    auto in_time_t = std::chrono::system_clock::to_time_t(now);
                    std::tm buf;
                    if (capture_tz == time_zone_type::utc) {
#if defined(_WIN32)
                        gmtime_s(&buf, &in_time_t);
#else
                        gmtime_r(&in_time_t, &buf);
#endif
                    }
                    else {
#if defined(_WIN32)
                        localtime_s(&buf, &in_time_t);
#else
                        localtime_r(&in_time_t, &buf);
#endif
                    }
                    char time_str[32];
                    std::strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S", &buf);

                    fs::path tmp =
                        candidate.parent_path() / (candidate.stem().string()
                            + time_str
                            + candidate.extension().string());

                    fs::copy_file(candidate, tmp, fs::copy_options::overwrite_existing);
                    if (console_log_copy) console_log_copy->info("[daily_before_close] copied '{}' -> '{}' (for zip)", candidate.string(), tmp.string());
                    {
                        auto tmp_copy = tmp;
                        auto orig_copy = orig_key;
                        auto console_log_copy_local = console_log_copy;
                        std::thread([
                            tmp_copy,
                            capture_delete_on_failure,
                            capture_comp_level,
                            capture_max_zip_count,
                            capture_tz,
                            console_log_copy_local,
                            orig_copy]()
                            {
                                process_zip_archive(
                                    tmp_copy,
                                    capture_delete_on_failure,
                                    capture_comp_level,
                                    capture_max_zip_count,
                                    capture_tz,
                                    console_log_copy_local,
                                    orig_copy);
                            }).detach();
                    }
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lk(g_zip_mu);
                    g_zip_in_progress.erase(orig_key);
                    if (console_log_copy) console_log_copy->error("[daily_before_close] copy failed for '{}': {}", candidate.string(), e.what());
                }
            }
            catch (const std::exception& e) {
                if (console_log_copy) console_log_copy->error("[daily_before_close] exception: {}", e.what());
            }
        };

        /*
        // after_close: 기존 방식 유지하되 로깅 메시지 통일
        handlers.after_close = [
            this,
            capture_max_files,
            capture_comp_level,
            capture_max_zip_count,
            capture_tz,
            capture_delete_on_failure,
            err_log_copy]
            (const ::spdlog::filename_t& closed_filename)
        {
            if (this->skip_handler_) {
                if (err_log_copy) err_log_copy->info("[daily_after_close] skipped due to skip_handler_ flag");
                return;
            }

            try {
                namespace fs = std::filesystem;
                fs::path p(closed_filename);
                if (p.empty()) {
                    return;
                }

                std::string stem = p.stem().string();
                std::regex re(R"(^(.+)\.(\d+)$)");
                std::smatch m;
                auto match_result = std::regex_match(stem, m, re);
                auto m_size = m.size();
                if (match_result && (m_size >= 3)) {
                    // 추출: base와 인덱스
                    std::string base = m[1].str();
                    std::size_t idx = 0;
                    try { idx = static_cast<std::size_t>(std::stoull(m[2].str())); }
                    catch (...) { idx = 0; }

                    if (capture_max_files == 0) {
                        if (err_log_copy) err_log_copy->warn("[daily_after_close] capture_max_files == 0, skip");
                        return;
                    }

                    if (idx != capture_max_files) {
                        if (err_log_copy) err_log_copy->info("[daily_after_close] '{}' is not target ({} != {}), skip zip", p.string(), idx, capture_max_files);
                        return;
                    }

                    if (fs::exists(p) && fs::is_regular_file(p)) {
                        std::thread([
                            p,
                            capture_delete_on_failure,
                            capture_comp_level,
                            capture_max_zip_count,
                            capture_tz,
                            err_log_copy]()
                            {
                                process_zip_archive(
                                    p,
                                    capture_delete_on_failure,
                                    capture_comp_level,
                                    capture_max_zip_count,
                                    capture_tz,
                                    err_log_copy);
                            }).detach();
                    }
                }
            }
            catch (...) {
                if (err_log_copy) err_log_copy->error("[Zip Hook Error] exception in daily after_close handler");
            }
        }; // after_close 람다
        // */

        backend_sink_ = std::make_shared<::spdlog::sinks::daily_file_sink<Mutex, FileNameCalc>>(filename.string(), rotation_hour, rotation_minute, false, (uint16_t)max_files, handlers);
        backend_sink_->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%v"));

#else
    // 구버전 spdlog fallback: handlers 파라미터 없이 생성
    backend_sink_ = std::make_shared<::spdlog::sinks::daily_file_sink<Mutex, FileNameCalc>>(filename.string(), rotation_hour, rotation_minute, false, (uint16_t)max_files);
    backend_sink_->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%v"));
#endif

        if (write_bom_ && encoding_ == log_encoding::utf8) {
            if (std::filesystem::exists(filename) && std::filesystem::file_size(filename) == 0) {
                std::ofstream fs(filename, std::ios::binary | std::ios::app);
                const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
                fs.write(reinterpret_cast<const char*>(utf8_bom), 3);
            }
        }

        this->set_formatter(std::make_unique<::spdlog::pattern_formatter>("%+"));
    }

    template<typename Mutex, typename FileNameCalc>
    encoding_daily_zipping_sink<Mutex, FileNameCalc>::~encoding_daily_zipping_sink() {

        this->set_skip_handler(true); // 소멸자에서 after_close/before_close 핸들러 스킵

        // 소멸자에서 진행중 집합을 비우고, 필요시 로그
        std::lock_guard<std::mutex> lk(g_zip_mu);
        if (!g_zip_in_progress.empty()) {
            if (console_logger_) {
                for (const auto& item : g_zip_in_progress) {
                    console_logger_->warn("[~encoding_daily_zipping_sink] still in-progress: '{}'", item);
                }
            }
            g_zip_in_progress.clear();
        }

    } // ~encoding_daily_zipping_sink() 

    template<typename Mutex, typename FileNameCalc>
    void encoding_daily_zipping_sink<Mutex, FileNameCalc>::set_skip_handler(bool skip) {
        if (!skip_handler_) skip_handler_ = std::make_shared<std::atomic<bool>>(skip);
        else skip_handler_->store(skip);
    }

    template<typename Mutex, typename FileNameCalc>
    void encoding_daily_zipping_sink<Mutex, FileNameCalc>::sink_it_(const ::spdlog::details::log_msg& msg) {
        ::spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        std::string raw_str(formatted.data(), formatted.size());

        while (!raw_str.empty() && (raw_str.back() == '\n' || raw_str.back() == '\r')) {
            raw_str.pop_back();
        }

        switch (line_ending_) {
        case line_ending::lf:   raw_str += "\n"; break;
        case line_ending::crlf: raw_str += "\r\n"; break;
        case line_ending::cr:   raw_str += "\r"; break;
        }

        ::spdlog::memory_buf_t output_buffer;
        if (encoding_ == log_encoding::cp949) {
            std::string cp949_str = convert_utf8_to_cp949(raw_str);
            output_buffer.append(cp949_str.data(), cp949_str.data() + cp949_str.size());
        }
        else {
            output_buffer.append(raw_str.data(), raw_str.data() + raw_str.size());
        }

        ::spdlog::details::log_msg forward_msg = msg;

        // forward_msg.payload = ::spdlog::details::to_string_view(output_buffer);
        forward_msg.payload = ::spdlog::string_view_t(output_buffer.data(), output_buffer.size());

        backend_sink_->log(forward_msg);
    }

    template<typename Mutex, typename FileNameCalc>
    void encoding_daily_zipping_sink<Mutex, FileNameCalc>::flush_() {
        backend_sink_->flush();
    }

    // 명시적 인스턴스화
    template class encoding_file_sink<std::mutex>;
    template class encoding_file_sink<::spdlog::details::null_mutex>;

    template class encoding_rotating_zipping_sink<std::mutex>;
    template class encoding_rotating_zipping_sink<::spdlog::details::null_mutex>;

    template class encoding_daily_zipping_sink<std::mutex>;
    template class encoding_daily_zipping_sink<::spdlog::details::null_mutex>;

} // namespace mino::external::log::spd

// 구현을 헤더 선언과 정확히 일치하도록 한정명으로 정의합니다.
std::string mino::external::log::spd::convert_utf8_to_cp949(const std::string& utf8_str) {
#ifdef _WIN32
    if (utf8_str.empty()) return {};

    int wlen = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8_str.c_str(),
        -1,
        nullptr,
        0);
    if (wlen <= 0) return {};

    std::wstring wstr(static_cast<std::size_t>(wlen), 0);
    MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8_str.c_str(),
        -1,
        &wstr[0],
        wlen);

    int klen = WideCharToMultiByte(
        949,
        0,
        wstr.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr);
    if (klen <= 0) return {};

    std::string cp949_str(static_cast<std::size_t>(klen), 0);
    WideCharToMultiByte(
        949,
        0,
        wstr.c_str(),
        -1,
        &cp949_str[0],
        klen,
        nullptr,
        nullptr);

    if (!cp949_str.empty() && cp949_str.back() == '\0') {
        cp949_str.pop_back();
    }
    return cp949_str;
#else
    (void)utf8_str;
    return utf8_str;
#endif
}


