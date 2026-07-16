#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <deque>

#if defined(_WIN32)
#   ifndef NOMINMAX
#      define NOMINMAX
#   endif
#   include <windows.h>
#   include <cwchar> // _wcsicmp
#endif

#include "mino/core//file/file_finder.hpp"
#include "mino/core/file/path.hpp"

namespace mino::core::file {

    namespace fs = std::filesystem;

    // -------------------- 내부 유틸 --------------------

#if defined(_WIN32)

#else
    // ASCII 범위만 소문자화(한글 등 비ASCII에는 영향 없음)
    static std::string to_lower_ascii(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }
#endif

    static bool is_hidden_unixlike(const fs::path& p) {
        const auto name = p.filename().string();
        return !name.empty() && name[0] == '.';
    }

    static bool get_win_attributes(const fs::path& p, bool& is_hidden, bool& is_system) {
#if defined(_WIN32)
        std::wstring w = p.wstring();
        DWORD attr = GetFileAttributesW(w.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) { is_hidden = false; is_system = false; return false; }
        is_hidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
        is_system = (attr & FILE_ATTRIBUTE_SYSTEM) != 0;
        return true;
#else
        (void)p; is_hidden = false; is_system = false; return false;
#endif
    }

    // 간단 와일드카드('*','?') 매칭 (파일/디렉토리 "이름" 대상)
    static bool wildcard_match(const std::string& pat, const std::string& text, bool case_sensitive) {
        auto norm = [&](char c) { return case_sensitive ? c : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
        size_t pi = 0, ti = 0, star = std::string::npos, star_t = 0;
        while (ti < text.size()) {
            if (pi < pat.size() && (pat[pi] == '?' || norm(pat[pi]) == norm(text[ti]))) { ++pi; ++ti; }
            else if (pi < pat.size() && pat[pi] == '*') { star = ++pi; star_t = ti; }
            else if (star != std::string::npos) { pi = star; ti = ++star_t; }
            else return false;
        }
        while (pi < pat.size() && pat[pi] == '*') ++pi;
        return pi == pat.size();
    }

    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    static bool match_extension(const fs::path& p, const file_finder::finder_options& opt) {
        if (opt.extensions.empty()) return true;
        std::string ext = p.extension().string();
        if (!opt.ext_case_sensitive) ext = to_lower(ext);
        for (const auto& want : opt.extensions) {
            if (opt.ext_case_sensitive) { if (ext == want) return true; }
            else { if (ext == to_lower(want)) return true; }
        }
        return false;
    }

    static bool match_name_filters(const std::string& name, const file_finder::finder_options& opt) {
        if (!opt.include_globs.empty()) {
            bool any = false;
            for (const auto& g : opt.include_globs)
                if (wildcard_match(g, name, opt.ext_case_sensitive)) { any = true; break; }
            if (!any) return false;
        }
        for (const auto& g : opt.exclude_globs)
            if (wildcard_match(g, name, opt.ext_case_sensitive)) return false;

        if (opt.name_regex.has_value())
            if (!std::regex_match(name, *opt.name_regex)) return false;

        return true;
    }

    static bool should_skip_dir_name(const std::string& name, const file_finder::finder_options& opt) {
        for (const auto& g : opt.exclude_dir_globs)
            if (wildcard_match(g, name, opt.ext_case_sensitive)) return true;
        return false;
    }

    static void push_err(std::vector<std::string>& dst, const fs::path& p,
        const std::error_code& ec, const char* what) {
        std::ostringstream oss;
        oss << what << ": " << p.string() << " : " << ec.message();
        dst.emplace_back(oss.str());
    }

    static std::optional<std::chrono::system_clock::time_point>
        get_mtime_ec(const fs::path& p) noexcept {
        std::error_code ec;
        auto ftime = fs::last_write_time(p, ec);
        if (ec) return std::nullopt;

        // C++17 호환 변환: file_clock 기준 시각 차이를 system_clock 기준으로 보정
        const auto now_sys = std::chrono::system_clock::now();
        const auto now_file = fs::file_time_type::clock::now();

        // file_clock 기준에서의 (파일시각 - 현재) 차이
        const auto diff_file = ftime - now_file;

        // diff_file 을 system_clock::duration 으로 캐스팅 후 더하기
        const auto sctp = now_sys +
            std::chrono::duration_cast<std::chrono::system_clock::duration>(diff_file);

        return sctp;
    }

    static std::optional<std::uintmax_t> get_size_ec(const fs::path& p) noexcept {
        std::error_code ec;
        auto s = fs::file_size(p, ec);
        if (ec) return std::nullopt;
        return s;
    }

    static bool filtered_out_hidden_system(const fs::path& p, const file_finder::finder_options& opt) {
        if (opt.include_hidden && opt.include_system) return false;
#if defined(_WIN32)
        bool is_hidden = false, is_system = false;
        if (get_win_attributes(p, is_hidden, is_system)) {
            if (!opt.include_hidden && is_hidden) return true;
            if (!opt.include_system && is_system) return true;
            return false;
        }
        if (!opt.include_hidden && is_hidden_unixlike(p)) return true;
        return false;
#else
        if (!opt.include_hidden && is_hidden_unixlike(p)) return true;
        return false;
#endif
    }

    static bool pass_time_size_filters(const file_finder::entry& e, const file_finder::finder_options& opt) {
        if (opt.mtime_since && e.mtime && *e.mtime < *opt.mtime_since) return false;
        if (opt.mtime_until && e.mtime && *e.mtime > *opt.mtime_until) return false;
        if (opt.min_size && e.size && *e.size < *opt.min_size) return false;
        if (opt.max_size && e.size && *e.size > *opt.max_size) return false;
        return true;
    }

    static file_finder::entry make_entry(const fs::path& p,
        const fs::file_status& st_sym,
        const fs::file_status& st,
        bool compute_size) {
        file_finder::entry e;
        e.path = p;
        e.is_symlink = fs::is_symlink(st_sym);
        e.is_dir = fs::is_directory(st);
        e.is_file = fs::is_regular_file(st);
        e.mtime = get_mtime_ec(p);
        if (compute_size && e.is_file) e.size = get_size_ec(p);
        e.found_time = std::chrono::system_clock::now();
        return e;
    }

    // -------------------- 스캔(싱글 스레드) --------------------

    using visited_set = std::unordered_set<std::string>;

    static bool scan_dir_children(const fs::path& dir, int depth,
        const file_finder::finder_options& opt,
        visited_set& visited,
        std::vector<std::string>& errs,
        std::vector<file_finder::entry>& out)
    {
        std::error_code ec;
        fs::directory_options dopt = fs::directory_options::none;
        if (opt.skip_permission_denied) dopt |= fs::directory_options::skip_permission_denied;

        if (opt.max_depth >= 0 && depth >= opt.max_depth) return true;

        for (fs::directory_iterator it(dir, dopt, ec), end; it != end; ) {
            const fs::directory_entry ent = *it;
            ec.clear();

            // 디렉토리 스킵(이름 글롭)
            if (ent.is_directory()) {
                const auto dname = ent.path().filename().string();
                if (should_skip_dir_name(dname, opt)) {
                    it.increment(ec);
                    if (ec) { push_err(errs, ent.path(), ec, "iterator 증가 실패"); if (opt.stop_on_error) return false; ec.clear(); }
                    continue;
                }
            }

            std::error_code e1, e2;
            auto st_sym = ent.symlink_status(e1);
            auto st = ent.status(e2);
            if (e1) push_err(errs, ent.path(), e1, "symlink_status 실패");
            if (e2) push_err(errs, ent.path(), e2, "status 실패");

            const auto name = ent.path().filename().string();

            // 결과 수집 여부
            bool take = false;
            if (!filtered_out_hidden_system(ent.path(), opt) && match_name_filters(name, opt)) {
                if (fs::is_symlink(st_sym) && opt.include_symlinks_as_items) take = true;
                if (fs::is_regular_file(st) && opt.include_files && match_extension(ent.path(), opt)) take = true;
                if (fs::is_directory(st) && opt.include_dirs) take = true;
            }

            if (take) {
                auto e = make_entry(ent.path(), st_sym, st, opt.compute_size);
                if (pass_time_size_filters(e, opt)) {
                    if (opt.on_visit) { if (!opt.on_visit(e)) return false; }
                    else {
                        out.push_back(std::move(e));
                        if (opt.limit_results && out.size() >= opt.limit_results) return false;
                    }
                }
            }

            // 하위 진입
            if (fs::is_directory(st)) {
                if (!opt.follow_symlinks && fs::is_symlink(st_sym)) {
                    // 링크 디렉토리는 미추적
                }
                else {
                    std::error_code ec_can;
                    auto can = fs::weakly_canonical(ent.path(), ec_can);
                    if (!ec_can) {
                        const auto key = can.string();
                        if (visited.insert(key).second) {
                            if (!scan_dir_children(ent.path(), depth + 1, opt, visited, errs, out)) return false;
                        }
                    }
                    else {
                        push_err(errs, ent.path(), ec_can, "weakly_canonical 실패");
                        if (opt.stop_on_error) return false;
                    }
                }
            }

            it.increment(ec);
            if (ec) { push_err(errs, ent.path(), ec, "iterator 증가 실패"); if (opt.stop_on_error) return false; ec.clear(); }
        }
        return true;
    }

    // 수정: "자식 1레벨"을 보려면 max_depth=1 이어야 합니다.
    static file_finder::finder_options make_non_recursive(file_finder::finder_options o) {
        o.recursive = true; // 내부 스캔은 재귀 함수
        o.max_depth = 1;    // depth=0(루트) -> 자식은 depth=1까지 허용
        return o;
    }

    // -------------------- 공개 API --------------------

    std::vector<file_finder::entry>
        file_finder::find(const fs::path& root, const finder_options& opt) noexcept {
        std::vector<file_finder::entry> out;
        std::vector<std::string> dummy_errs;

        std::error_code ec;
        auto st_root = fs::status(root, ec);
        if (ec) return out;

        // 루트 자체도 조건에 따라 포함
        if (opt.include_dirs && fs::is_directory(st_root) && !filtered_out_hidden_system(root, opt)) {
            const auto name = root.filename().string();
            if (match_name_filters(name.empty() ? root.string() : name, opt)) {
                auto e = make_entry(root, fs::symlink_status(root, ec), st_root, opt.compute_size);
                if (pass_time_size_filters(e, opt)) {
                    if (opt.on_visit) { if (!opt.on_visit(e)) return out; }
                    else out.push_back(std::move(e));
                }
            }
        }
        else if (opt.include_files && fs::is_regular_file(st_root)
            && !filtered_out_hidden_system(root, opt) && match_extension(root, opt)) {
            const auto name = root.filename().string();
            if (match_name_filters(name.empty() ? root.string() : name, opt)) {
                auto e = make_entry(root, fs::symlink_status(root, ec), st_root, opt.compute_size);
                if (pass_time_size_filters(e, opt)) {
                    if (opt.on_visit) { if (!opt.on_visit(e)) return out; }
                    else out.push_back(std::move(e));
                }
            }
        }

        if (opt.recursive && fs::is_directory(st_root)) {
            visited_set visited;
            auto can = fs::weakly_canonical(root, ec);
            if (!ec) visited.insert(can.string());
            scan_dir_children(root, 0, opt, visited, dummy_errs, out);
        }
        else if (!opt.recursive && fs::is_directory(st_root)) {
            visited_set visited;
            auto nonrec = make_non_recursive(opt);
            scan_dir_children(root, 0, nonrec, visited, dummy_errs, out);
        }

        // 정렬/리밋
        auto sorter = [&](auto& v, sort_key key, bool asc) {
            auto less = [&](const auto& a, const auto& b) {
                switch (key) {
                case sort_key::path: return a.path.string() < b.path.string();
                case sort_key::name: return a.path.filename().string() < b.path.filename().string();
                case sort_key::mtime: {
                    auto at = a.mtime.value_or(std::chrono::system_clock::time_point::min());
                    auto bt = b.mtime.value_or(std::chrono::system_clock::time_point::min());
                    return at < bt;
                }
                case sort_key::size: {
                    auto asz = a.size.value_or(0), bsz = b.size.value_or(0);
                    return asz < bsz;
                }
                default: return false;
                }
                };
            if (key != sort_key::none)
                std::sort(v.begin(), v.end(), [&](const auto& x, const auto& y) { return asc ? less(x, y) : less(y, x); });
            };
        sorter(out, opt.sort_by, opt.sort_ascending);
        if (opt.limit_results && out.size() > opt.limit_results) out.resize(opt.limit_results);

        return out;
    }

    std::vector<file_finder::entry>
        file_finder::find(const fs::path& root) noexcept {
        last_errors_.clear();

        std::error_code ec;
        auto st_root = fs::status(root, ec);
        if (ec) { push_err(last_errors_, root, ec, "status 실패"); return {}; }

        std::vector<file_finder::entry> out;

        // 루트 포함 로직(정적 버전과 동일)
        if (opts_.include_dirs && fs::is_directory(st_root) && !filtered_out_hidden_system(root, opts_)) {
            const auto name = root.filename().string();
            if (match_name_filters(name.empty() ? root.string() : name, opts_)) {
                auto e = make_entry(root, fs::symlink_status(root, ec), st_root, opts_.compute_size);
                if (pass_time_size_filters(e, opts_)) {
                    if (opts_.on_visit) { if (!opts_.on_visit(e)) return out; }
                    else out.push_back(std::move(e));
                }
            }
        }
        else if (opts_.include_files && fs::is_regular_file(st_root)
            && !filtered_out_hidden_system(root, opts_) && match_extension(root, opts_)) {
            const auto name = root.filename().string();
            if (match_name_filters(name.empty() ? root.string() : name, opts_)) {
                auto e = make_entry(root, fs::symlink_status(root, ec), st_root, opts_.compute_size);
                if (pass_time_size_filters(e, opts_)) {
                    if (opts_.on_visit) { if (!opts_.on_visit(e)) return out; }
                    else out.push_back(std::move(e));
                }
            }
        }

        if (opts_.recursive && fs::is_directory(st_root)) {
            visited_set visited;
            auto can = fs::weakly_canonical(root, ec);
            if (!ec) visited.insert(can.string());
            std::vector<std::string> errs;
            scan_dir_children(root, 0, opts_, visited, errs, out);
            last_errors_ = std::move(errs);
        }
        else if (!opts_.recursive && fs::is_directory(st_root)) {
            visited_set visited;
            std::vector<std::string> errs;
            auto nonrec = make_non_recursive(opts_);
            scan_dir_children(root, 0, nonrec, visited, errs, out);
            last_errors_ = std::move(errs);
        }

        auto sorter = [&](auto& v, sort_key key, bool asc) {
            auto less = [&](const auto& a, const auto& b) {
                switch (key) {
                case sort_key::path: return a.path.string() < b.path.string();
                case sort_key::name: return a.path.filename().string() < b.path.filename().string();
                case sort_key::mtime: {
                    auto at = a.mtime.value_or(std::chrono::system_clock::time_point::min());
                    auto bt = b.mtime.value_or(std::chrono::system_clock::time_point::min());
                    return at < bt;
                }
                case sort_key::size: {
                    auto asz = a.size.value_or(0), bsz = b.size.value_or(0);
                    return asz < bsz;
                }
                default: return false;
                }
                };
            if (key != sort_key::none)
                std::sort(v.begin(), v.end(), [&](const auto& x, const auto& y) { return asc ? less(x, y) : less(y, x); });
            };
        sorter(out, opts_.sort_by, opts_.sort_ascending);
        if (opts_.limit_results && out.size() > opts_.limit_results) out.resize(opts_.limit_results);

        return out;
    }

    bool file_finder::exists_now(const fs::path& p) noexcept {
        std::error_code ec;
        return fs::exists(p, ec) && !ec;
    }

    std::time_t file_finder::to_time_t(const std::chrono::system_clock::time_point& tp) noexcept {
        return std::chrono::system_clock::to_time_t(tp);
    }

    void file_finder::print_entry_safe(const entry& e, std::ostream& os) noexcept {
        if (!exists_now(e.path)) { os << "[사라짐] " << e.path.string() << "\n"; return; }
        std::time_t ft = to_time_t(e.found_time);
        os << e.path.string()
            << "  (found_at=" << std::put_time(std::localtime(&ft), "%F %T") << ")";
        if (e.mtime) {
            std::time_t mt = to_time_t(*e.mtime);
            os << "  mtime=" << std::put_time(std::localtime(&mt), "%F %T");
        }
        if (e.size) os << "  size=" << *e.size << "B";
        os << (e.is_symlink ? "  [symlink]" : "")
            << (e.is_dir ? "  [dir]" : (e.is_file ? "  [file]" : ""))
            << "\n";
    }

    // -------------------- exists() 통합 헬퍼 --------------------

    // 경로 버전(권장: 유니코드/플랫폼 안전)
    bool exists(
        const fs::path& root,
        const fs::path& filename,
        bool recursive,
        bool case_insensitive
    ) noexcept
    {
        bool found = false;

        file_finder::finder_options opt;
        opt.recursive = recursive;
        opt.include_files = true;
        opt.include_dirs = false;

#if defined(_WIN32)
        // Windows: UTF-16 네이티브 비교. 대소문자 무시는 _wcsicmp 사용.
        const std::wstring want = filename.filename().native();
        opt.on_visit = [&](const file_finder::entry& e) -> bool {
            if (!e.is_file) return true;
            const std::wstring name = e.path.filename().native();
            bool eq = false;
            if (case_insensitive) eq = (_wcsicmp(name.c_str(), want.c_str()) == 0);
            else                  eq = (name == want);
            if (eq) { found = true; return false; }
            return true;
            };
#else
        // POSIX: UTF-8(바이트) 비교. 한글은 케이스 개념이 없어 case_insensitive 영향 없음.
        const std::string want = filename.filename().native();
        opt.on_visit = [&](const file_finder::entry& e) -> bool {
            if (!e.is_file) return true;
            std::string name = e.path.filename().native();
            if (case_insensitive) {
                if (to_lower_ascii(name) == to_lower_ascii(want)) { found = true; return false; }
            }
            else {
                if (name == want) { found = true; return false; }
            }
            return true;
            };
#endif

        file_finder finder(opt);
        (void)finder.find(root);
        return found;
    }

    // 문자열 버전(호환용) → 경로 버전으로 위임
    bool exists(
        const fs::path& root,
        const std::string& utf8_filename,
        bool recursive,
        bool case_insensitive
    ) noexcept
    {
        std::filesystem::path filepath = mino::core::file::path_from_utf8(utf8_filename);
        return exists(root, filepath, recursive, case_insensitive);
    }

    bool exists(
        const std::string& utf8_root,
        const std::string& utf8_filename,
        bool recursive,
        bool case_insensitive
    ) noexcept
    {
        std::filesystem::path rootpath = mino::core::file::path_from_utf8(utf8_root);
        std::filesystem::path filepath = mino::core::file::path_from_utf8(utf8_filename);
        return exists(rootpath, filepath, recursive, case_insensitive);
    }

    bool exists(
        const std::string& utf8_root,
        const fs::path& filename,
        bool recursive,
        bool case_insensitive
    ) noexcept
    {
        std::filesystem::path rootpath = mino::core::file::path_from_utf8(utf8_root);
        return exists(rootpath, filename, recursive, case_insensitive);
    }

} // namespace mino::core::file
