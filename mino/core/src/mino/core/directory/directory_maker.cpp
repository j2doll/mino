#include "mino/core/directory/directory_maker.hpp"
#include "mino/core/string/to_console_encoding.hpp"

namespace mino::core::directory {

    using std::errc;

    directory_maker::directory_maker(language lang, create_dir_options opt) noexcept
        : lang_(lang), opt_(opt) {
    }

    create_path_code directory_maker::map_errc_(const std::error_code& ec) noexcept {
        switch (static_cast<errc>(ec.value())) {
        case errc::permission_denied:         return create_path_code::permission_denied;
        case errc::read_only_file_system:     return create_path_code::read_only_filesystem;
        case errc::file_exists:               return create_path_code::already_exists;
        case errc::not_a_directory:           return create_path_code::not_directory;
        case errc::no_such_file_or_directory: return create_path_code::no_such_parent;
        case errc::filename_too_long:         return create_path_code::name_too_long;
        case errc::no_space_on_device:        return create_path_code::disk_full;
        case errc::interrupted:               return create_path_code::interrupted;
        default:                              return create_path_code::unknown_error;
        }
    }

    std::filesystem::file_status
        directory_maker::safe_status_(const std::filesystem::path& p,
            bool follow_symlinks,
            std::error_code& ec) const noexcept {
        return follow_symlinks ? std::filesystem::status(p, ec)
            : std::filesystem::symlink_status(p, ec);
    }

    void directory_maker::try_set_perms_(const std::filesystem::path& p) const noexcept {
        if (!opt_.set_permissions) return;
        std::error_code ign;
        switch (opt_.mode) {
        case perm_mode::exact:
            std::filesystem::permissions(p, opt_.perms_mask, ign);
            break;
        case perm_mode::add:
            std::filesystem::permissions(p, opt_.perms_mask, std::filesystem::perm_options::add, ign);
            break;
        case perm_mode::remove:
            std::filesystem::permissions(p, opt_.perms_mask, std::filesystem::perm_options::remove, ign);
            break;
        }
    }

    create_dir_result directory_maker::create_directory_tree(const std::filesystem::path& target) const noexcept {
        create_dir_result out;
        out.requested = target;
        out.success = false;

        try {
            // 0) 입력 검증
            if (target.empty()) {
                out.code = create_path_code::invalid_path;
                out.message = msg_invalid_path_();
                return out;
            }

            // 1) 최종 경로 상태 확인
            {
                std::error_code ec;
                auto st = safe_status_(target, opt_.follow_symlinks, ec);

                std::error_code sec;
                auto sy = std::filesystem::symlink_status(target, sec);

                // Windows 디렉터리 링크/정션 검증 보완 logic
                bool is_link = (!sec && std::filesystem::is_symlink(sy));
                if (!ec && !sec && st.type() != sy.type()) {
                    is_link = true;
                }

                if (is_link && !opt_.follow_symlinks) {
                    out.code = create_path_code::symlink_blocked;
                    out.message = msg_symlink_blocked_();
                    return out;
                }

                if (!ec && st.type() != std::filesystem::file_type::not_found) {
                    if (std::filesystem::is_directory(st)) {
                        out.code = create_path_code::already_exists;
                        out.message = opt_.succeed_if_exists ?
                            msg_already_exists_() : msg_already_exists_error_();
                        out.success = opt_.succeed_if_exists;
                        return out;
                    }
                    out.code = create_path_code::not_directory;
                    out.message = msg_not_directory_(target);
                    return out;
                }
                else if (ec && ec != errc::no_such_file_or_directory) {
                    out.code = map_errc_(ec);
                    out.ec = ec;
                    out.message = msg_state_fail_(target, ec);
                    return out;
                }
            }

            // 2) make_parents=false 정책 처리
            if (!opt_.make_parents) {
                std::filesystem::path parent = target.parent_path();
                std::error_code pec;
                if (!parent.empty() && !std::filesystem::exists(parent, pec)) {
                    out.code = create_path_code::no_such_parent;
                    out.ec = pec;
                    out.message = msg_no_parent_(parent);
                    return out;
                }

                std::error_code mec;
                bool made = std::filesystem::create_directory(target, mec);
                if (!made) {
                    if (!mec) {
                        std::error_code rec;
                        if (std::filesystem::is_directory(safe_status_(target, opt_.follow_symlinks, rec))) {
                            out.code = create_path_code::already_exists;
                            out.message = opt_.succeed_if_exists ?
                                msg_already_exists_() : msg_already_exists_error_();
                            out.success = opt_.succeed_if_exists;
                            return out;
                        }
                        out.code = create_path_code::not_directory;
                        out.message = msg_not_directory_(target);
                        return out;
                    }
                    else {
                        if (mec == errc::file_exists) {
                            std::error_code rec;
                            if (std::filesystem::is_directory(safe_status_(target, opt_.follow_symlinks, rec))) {
                                out.code = create_path_code::already_exists;
                                out.message = opt_.succeed_if_exists ?
                                    msg_already_exists_() : msg_already_exists_error_();
                                out.success = opt_.succeed_if_exists;
                                return out;
                            }
                            out.code = create_path_code::not_directory;
                            out.ec = mec;
                            out.message = msg_not_directory_(target);
                            return out;
                        }
                        out.code = map_errc_(mec);
                        out.ec = mec;
                        out.message = msg_create_fail_(target, mec);
                        return out;
                    }
                }

                try_set_perms_(target);
                out.created.push_back(target);
                out.code = create_path_code::created;
                out.message = msg_created_ok_();
                out.success = true;
                return out;
            }

            // 3) make_parents=true 정책 처리 (상위부터 누적 생성)
            std::filesystem::path accum;
#if defined(_WIN32)
            if (target.has_root_name())
                accum /= target.root_name();
            if (target.has_root_directory())
                accum /= target.root_directory();
#else
            if (target.has_root_directory())
                accum /= target.root_directory();
#endif

            for (const auto& part : target.relative_path()) {
                accum /= part;

                std::error_code ec;
                auto st = safe_status_(accum, opt_.follow_symlinks, ec);

                std::error_code sec;
                auto sy = std::filesystem::symlink_status(accum, sec);

                // 중간 경로 누적 검사 시 심볼릭 링크/정션 교차 검증 보완
                bool loop_is_link = (!sec && std::filesystem::is_symlink(sy));
                if (!ec && !sec && st.type() != sy.type()) {
                    loop_is_link = true;
                }

                if (loop_is_link && !opt_.follow_symlinks) {
                    out.code = create_path_code::symlink_blocked;
                    out.message = msg_symlink_blocked_();
                    return out;
                }

                if (!ec && st.type() != std::filesystem::file_type::not_found) {
                    if (std::filesystem::is_directory(st)) continue;
                    out.code = create_path_code::not_directory;
                    out.message = msg_not_directory_(accum);
                    return out;
                }

                if (ec && ec != errc::no_such_file_or_directory) {
                    out.code = map_errc_(ec);
                    out.ec = ec;
                    out.message = msg_state_fail_(accum, ec);
                    return out;
                }

                std::error_code mec;
                bool made = std::filesystem::create_directory(accum, mec);
                if (!made) {
                    if (!mec) {
                        std::error_code rec;
                        if (std::filesystem::is_directory(safe_status_(accum, opt_.follow_symlinks, rec)))
                            continue;
                        out.code = create_path_code::not_directory;
                        out.message = msg_not_directory_(accum);
                        return out;
                    }
                    else {
                        if (mec == errc::file_exists) {
                            std::error_code rec;
                            if (std::filesystem::is_directory(safe_status_(accum, opt_.follow_symlinks, rec))) continue;
                            out.code = create_path_code::not_directory;
                            out.ec = mec;
                            out.message = msg_not_directory_(accum);
                            return out;
                        }
                        out.code = map_errc_(mec);
                        out.ec = mec;
                        out.message = msg_create_fail_(accum, mec);
                        return out;
                    }
                }

                try_set_perms_(accum);
                out.created.push_back(accum);
            }

            if (out.created.empty()) {
                out.code = create_path_code::already_exists;
                out.message = opt_.succeed_if_exists ? msg_nothing_new_()
                    : msg_already_exists_error_();
                out.success = opt_.succeed_if_exists;
            }
            else {
                out.code = create_path_code::created;
                out.message = msg_created_ok_();
                out.success = true;
            }
            return out;
        }
        catch (const std::filesystem::filesystem_error& ex) {
            out.ec = ex.code();
            out.code = map_errc_(out.ec);
            out.message = msg_fs_exception_(ex.what(), out.ec);
            return out;
        }
        catch (const std::exception& ex) {
            out.code = create_path_code::unknown_error;
            out.message = msg_std_exception_(ex.what());
            return out;
        }
        catch (...) {
            out.code = create_path_code::unknown_error;
            out.message = msg_unknown_exception_();
            return out;
        }
    }

    const char* directory_maker::to_string(create_path_code c) noexcept {
        switch (c) {
        case create_path_code::created:              return "Created";
        case create_path_code::already_exists:       return "AlreadyExists";
        case create_path_code::invalid_path:         return "InvalidPath";
        case create_path_code::not_directory:        return "NotDirectory";
        case create_path_code::permission_denied:    return "PermissionDenied";
        case create_path_code::symlink_blocked:      return "SymlinkBlocked";
        case create_path_code::read_only_filesystem: return "ReadOnlyFilesystem";
        case create_path_code::name_too_long:        return "NameTooLong";
        case create_path_code::no_such_parent:       return "NoSuchParent";
        case create_path_code::disk_full:            return "DiskFull";
        case create_path_code::interrupted:          return "Interrupted";
        case create_path_code::unknown_error:        return "UnknownError";
        default:                                     return "Unknown";
        }
    }

    // ---------------------------
    // 메시지 맵핑 레이어 (한/영 지원)
    // ---------------------------
    std::string directory_maker::msg_invalid_path_() const {
        return lang_ == language::English
            ? "Invalid path: empty or malformed."
            : mino::core::string::to_console_encoding("유효하지 않은 경로입니다(빈 경로 또는 잘못된 형식).");
    }
    std::string directory_maker::msg_already_exists_() const {
        return lang_ == language::English
            ? "Directory already exists."
            : mino::core::string::to_console_encoding("경로가 이미 디렉터리로 존재합니다.");
    }
    std::string directory_maker::msg_already_exists_error_() const {
        return lang_ == language::English
            ? "Directory already exists (policy: treat as error)."
            : mino::core::string::to_console_encoding("경로가 이미 존재합니다(정책상 오류로 간주).");
    }
    std::string directory_maker::msg_symlink_blocked_() const {
        return lang_ == language::English
            ? "Operation blocked by symlink."
            : mino::core::string::to_console_encoding("심볼릭 링크가 존재하며 차단되었습니다.");
    }
    std::string directory_maker::msg_not_directory_(const std::filesystem::path& p) const {
        return lang_ == language::English
            ? "A non-directory entry exists: " + p.string()
            : mino::core::string::to_console_encoding("디렉터리가 아닌 항목이 존재합니다: " + p.string());
    }
    std::string directory_maker::msg_state_fail_(const std::filesystem::path& p, const std::error_code& ec) const {
        return lang_ == language::English
            ? "Failed to query status: " + p.string() + " : " + ec.message()
            : mino::core::string::to_console_encoding("경로 상태 확인 실패: " + p.string() + " : " + ec.message());
    }
    std::string directory_maker::msg_no_parent_(const std::filesystem::path& p) const {
        return lang_ == language::English
            ? "Parent does not exist: " + p.string()
            : mino::core::string::to_console_encoding("부모 경로가 존재하지 않습니다: " + p.string());
    }
    std::string directory_maker::msg_create_fail_(const std::filesystem::path& p, const std::error_code& ec) const {
        return lang_ == language::English
            ? "Failed to create: " + p.string() + " : " + ec.message()
            : mino::core::string::to_console_encoding("디렉터리 생성 실패: " + p.string() + " : " + ec.message());
    }
    std::string directory_maker::msg_created_ok_() const {
        return lang_ == language::English
            ? "Directory tree created."
            : mino::core::string::to_console_encoding("디렉터리 트리를 성공적으로 생성했습니다.");
    }
    std::string directory_maker::msg_nothing_new_() const {
        return lang_ == language::English
            ? "Nothing created; already exists."
            : mino::core::string::to_console_encoding("새로 생성된 디렉터리가 없습니다(이미 존재).");
    }
    std::string directory_maker::msg_fs_exception_(const std::string& what, const std::error_code& ec) const {
        return lang_ == language::English
            ? "filesystem_error: " + what + " : " + ec.message()
            : mino::core::string::to_console_encoding("filesystem_error: " + what + " : " + ec.message());
    }
    std::string directory_maker::msg_std_exception_(const std::string& what) const {
        return lang_ == language::English
            ? "std::exception: " + what
            : mino::core::string::to_console_encoding("std::exception: " + what);
    }
    std::string directory_maker::msg_unknown_exception_() const {
        return lang_ == language::English
            ? "Unknown exception."
            : mino::core::string::to_console_encoding("알 수 없는 예외가 발생했습니다.");
    }

}  
