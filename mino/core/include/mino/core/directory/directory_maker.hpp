#pragma once

#include <filesystem>
#include <system_error>
#include <string>
#include <vector>

namespace mino::core::directory {

    // -----------------------------------------------------------------------
    // 메시지 언어 설정
    // -----------------------------------------------------------------------
    enum class language {
        English,  // 영어 메시지
        Korean    // 한국어 메시지
    };

    // -----------------------------------------------------------------------
    // 디렉터리 생성 결과 코드
    // -----------------------------------------------------------------------
    enum class create_path_code {
        created,            // 새 디렉터리가 생성됨
        already_exists,      // 이미 디렉터리가 존재함
        invalid_path,        // 잘못된 경로 (빈 문자열 등)
        not_directory,       // 경로 중 일부가 디렉터리가 아님
        permission_denied,   // 권한 부족
        symlink_blocked,     // 심볼릭 링크 차단됨
        read_only_filesystem, // 읽기 전용 파일 시스템
        name_too_long,        // 파일 이름이 너무 김
        no_such_parent,       // 부모 디렉터리가 존재하지 않음
        disk_full,           // 디스크 공간 부족
        interrupted,        // 호출이 중단됨
        unknown_error        // 알 수 없는 오류
    };

    // -----------------------------------------------------------------------
    // 권한 설정 모드
    // -----------------------------------------------------------------------
    enum class perm_mode {
        add,    // 기존 권한에 추가
        remove, // 기존 권한에서 제거
        exact   // 정확히 지정한 권한으로 설정
    };

    // -----------------------------------------------------------------------
    // 디렉터리 생성 옵션 구조체
    // -----------------------------------------------------------------------
    struct  create_dir_options {
        bool make_parents = true;  // 상위 디렉터리 자동 생성 여부
        bool succeed_if_exists = true; // 디렉터리 이미 존재 시 성공으로 간주 여부
        bool follow_symlinks = false; // 심볼릭 링크 따라갈지 여부
        bool set_permissions = false; // 생성 후 권한 설정 여부

        // 기본 권한: 소유자 전체, 그룹 읽기/실행, 기타 읽기/실행
        std::filesystem::perms perms_mask =
            std::filesystem::perms::owner_all |
            std::filesystem::perms::group_read |
            std::filesystem::perms::group_exec |
            std::filesystem::perms::others_read |
            std::filesystem::perms::others_exec;

        // 권한 설정 모드
        perm_mode mode = perm_mode::exact; // renamed member to avoid collision with the type `perm_mode`
    };

    // -----------------------------------------------------------------------
    // 디렉터리 생성 결과 구조체
    // -----------------------------------------------------------------------
    struct  create_dir_result {
        create_path_code code = create_path_code::unknown_error; // 결과 코드
        std::error_code ec;                                // 에러 코드
        std::string message;                               // 결과 메시지
        std::filesystem::path requested;                   // 요청한 경로
        std::vector<std::filesystem::path> created;        // 실제 생성된 경로 목록
        bool success = false;                              // 최종 성공 여부

        // bool 변환 연산자: 성공 여부 반환
        explicit operator bool() const { return success; }
    };

    // -----------------------------------------------------------------------
    // 디렉터리 생성기 클래스
    // -----------------------------------------------------------------------
    class  directory_maker {
    public:
        // 생성자
        // - 언어(language)와 옵션(create_dir_options)을 설정합니다.
        // - 기본값: language::English, 부모 자동 생성 옵션 활성화
        //
        explicit directory_maker(
            language lang = language::English,
            create_dir_options opt = {}) noexcept;

        // 현재 언어 설정을 반환합니다.
        // 예시:
        // language lang = maker.get_language();
        language get_language() const noexcept { return lang_; }

        // 언어를 변경합니다.
        // 예시:
        // maker.set_language(language::English);
        void set_language(language lang) noexcept { lang_ = lang; }

        // 현재 옵션을 반환합니다.
        // 예시:
        // const create_dir_options& opt = maker.options();
        const create_dir_options& options() const noexcept { return opt_; }

        // 옵션을 변경합니다.
        // 예시:
        // create_dir_options opt; opt.make_parents = false;
        // maker.set_options(opt);
        void set_options(const create_dir_options& opt) noexcept { opt_ = opt; }

        //  NOTICE: 디렉터리 생성 함수
        // 지정한 경로의 디렉터리를 생성합니다.
        // 옵션에 따라 부모 경로 생성, 권한 설정 등이 적용됩니다.
        //
        // 예시:
        // create_dir_result res = maker.create_directory_tree("data/output");
        // if (res) std::cout << "성공" << std::endl;
        create_dir_result create_directory_tree(const std::filesystem::path& target) const noexcept;

        // create_path_code를 문자열로 변환합니다.
        // 예시:
        // const char* s = directory_maker::to_string(create_path_code::Created);
        static const char* to_string(create_path_code c) noexcept;

    private:
        // std::error_code를 create_path_code로 매핑
        static create_path_code map_errc_(const std::error_code& ec) noexcept;

        // 안전하게 파일 상태를 조회
        std::filesystem::file_status safe_status_(
            const std::filesystem::path& p,
            bool follow_symlinks,
            std::error_code& ec) const noexcept;

        // 권한 설정 시도 (옵션에 따라 적용)
        void try_set_perms_(const std::filesystem::path& p) const noexcept;

        // 메시지 생성 함수들 (언어 설정에 따라 다르게 출력)
        std::string msg_invalid_path_() const; // 빈 경로 또는 잘못된 형식 메시지
        std::string msg_already_exists_() const; // 이미 존재하지만 성공으로 간주 메시지
        std::string msg_already_exists_error_() const; // 이미 존재하지만 오류로 간주 메시지
        std::string msg_symlink_blocked_() const; // 심볼릭 링크 차단 메시지
        std::string msg_not_directory_(const std::filesystem::path& p) const; // 디렉터리가 아닌 항목 존재 메시지
        std::string msg_state_fail_(const std::filesystem::path& p,
            const std::error_code& ec) const; // 경로 상태 확인 실패 메시지
        std::string msg_no_parent_(const std::filesystem::path& p) const; // 부모 경로 존재하지 않음 메시지
        std::string msg_create_fail_(const std::filesystem::path& p,
            const std::error_code& ec) const; // 디렉터리 생성 실패 메시지
        std::string msg_created_ok_() const; // 디렉터리 생성 성공 메시지
        std::string msg_nothing_new_() const; // 새로 생성된 경로가 없지만 성공으로 간주 메시지
        std::string msg_fs_exception_(const std::string& what,
            const std::error_code& ec) const; // std::filesystem 예외 메시지   
        std::string msg_std_exception_(const std::string& what) const; // std::exception 메시지
        std::string msg_unknown_exception_() const; // 알 수 없는 예외 메시지

    private:
        language lang_;               // 현재 언어 설정
        create_dir_options opt_;      // 현재 옵션 설정
    };

}  