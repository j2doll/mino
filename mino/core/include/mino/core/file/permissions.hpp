#pragma once

#include <string>
#include <filesystem>

namespace mino::core::file {

    // 권한 구조체: 읽기/쓰기/실행
    struct perms {
        bool read;
        bool write;
        bool execute;
    };

    // 파일 권한 정보 조회: POSIX 모드와 Windows ACL을 공통 인터페이스로 노출
    class  permissions_info {
    public:
        explicit permissions_info(const std::string& path);

        // 경로 존재 여부
        bool exists() const;

        // 소유자/그룹/기타 권한 조회 (POSIX의 owner/group/others에 대응시키는 근사 매핑)
        perms for_owner() const;
        perms for_group() const;
        perms for_others() const;

        // 사람이 읽을 수 있는 문자열(예: "rwxr-xr--")
        std::string to_string() const;

    private:
        void load();

        std::filesystem::path path_;
        perms owner_;
        perms group_;
        perms others_;
        bool loaded_;
    };

} // namespace mino::core::file
