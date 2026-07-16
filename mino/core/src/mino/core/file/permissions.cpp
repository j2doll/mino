#include "mino/core/file/permissions.hpp"

#include <system_error>

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <Aclapi.h>
#   include <sddl.h>
#   include <accctrl.h>
#   pragma comment(lib, "advapi32.lib")
#else
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

namespace mino::core::file {

    permissions_info::permissions_info(const std::string& path)
        : path_(path), owner_{false, false, false}, group_{false, false, false}, others_{false, false, false}, loaded_(false)
    {
        load();
    }

    bool permissions_info::exists() const {
        return std::filesystem::exists(path_);
    }

    perms permissions_info::for_owner() const {
        return owner_;
    }

    perms permissions_info::for_group() const {
        return group_;
    }

    perms permissions_info::for_others() const {
        return others_;
    }

    std::string permissions_info::to_string() const {
        auto p = [](const perms& x) {
            std::string s;
            s += (x.read ? 'r' : '-');
            s += (x.write ? 'w' : '-');
            s += (x.execute ? 'x' : '-');
            return s;
        };
        return p(owner_) + p(group_) + p(others_);
    }

    namespace {
#ifdef _WIN32
        // 주어진 SID에 대해 DACL에서 효과적인 권한을 계산하여 read/write/execute를 채움
        static perms effective_perms_from_dacl(PACL dacl, PSID sid) {
            perms r{false, false, false};
            if (dacl == nullptr || sid == nullptr) {
                return r;
            }

            // BUILD_TRUSTEE_WITH_SID_A 매크로가 없는 환경을 위해 TRUSTEE_A 직접 초기화
            TRUSTEE_A trustee;
            ZeroMemory(&trustee, sizeof(trustee));
            trustee.pMultipleTrustee = nullptr;
            trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            trustee.TrusteeForm = TRUSTEE_IS_SID;
            trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
            trustee.ptstrName = reinterpret_cast<LPSTR>(sid);

            ACCESS_MASK accessMask = 0;
            DWORD rc = GetEffectiveRightsFromAclA(dacl, &trustee, &accessMask);
            if (rc != ERROR_SUCCESS) {
                return r;
            }

            // FILE_GENERIC_* 매크로를 사용하여 읽기/쓰기/실행을 판단 (근사)
            if (accessMask & FILE_GENERIC_READ) {
                r.read = true;
            }
            if (accessMask & FILE_GENERIC_WRITE) {
                r.write = true;
            }
            if (accessMask & FILE_GENERIC_EXECUTE) {
                r.execute = true;
            }

            return r;
        }

        // 이름으로 SID 얻기 (예: "BUILTIN\\Users", "Everyone")
        static PSID sid_from_account_name(const char* name) {
            DWORD sidSize = 0;
            DWORD domainSize = 0;
            SID_NAME_USE sidType;
            LookupAccountNameA(nullptr, name, nullptr, &sidSize, nullptr, &domainSize, &sidType);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                return nullptr;
            }

            // 예: LocalAlloc으로 SID 할당 (권장: sid_from_account_name 내부에서 일관성 유지)
            PSID sid = static_cast<PSID>(LocalAlloc(LMEM_FIXED, sidSize));
            if (!sid) return nullptr;
            std::unique_ptr<char[]> domainBuf(new (std::nothrow) char[domainSize + 1]);
            if (!domainBuf) { LocalFree(sid); return nullptr; }
            if (!LookupAccountNameA(nullptr, name, sid, &sidSize, domainBuf.get(), &domainSize, &sidType)) {
                LocalFree(sid);
                return nullptr;
            }
            return sid;
        }

#endif
    } // namespace

    void permissions_info::load() {
        if (loaded_) {
            return;
        }

#ifdef _WIN32
        owner_ = {false, false, false};
        group_ = {false, false, false};
        others_ = {false, false, false};

        // GetNamedSecurityInfo로 소유자 SID와 DACL을 얻음
        PSID ownerSid = nullptr;
        PACL dacl = nullptr;
        PSECURITY_DESCRIPTOR secDesc = nullptr;

        DWORD rc = GetNamedSecurityInfoA(
            path_.string().c_str(),
            SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            &ownerSid,
            nullptr,
            &dacl,
            nullptr,
            &secDesc);

        if (rc == ERROR_SUCCESS) {
            // owner 권한 계산
            if (ownerSid != nullptr && dacl != nullptr) {
                owner_ = effective_perms_from_dacl(dacl, ownerSid);
            }

            // group: 근사를 위해 BUILTIN\\Users 사용
            // sid_from_account_name는 LocalAlloc으로 할당하므로 LocalFree로 해제 가능 — 할당/해제 계약을 일관되게 유지함
            PSID tmpUsersSid = sid_from_account_name("BUILTIN\\Users");
            if (tmpUsersSid != nullptr) {
                group_ = effective_perms_from_dacl(dacl, tmpUsersSid);
                LocalFree(tmpUsersSid);
            } else {
                // fallback: try "Users"
                PSID lookupSid = sid_from_account_name("Users");
                if (lookupSid != nullptr) {
                    group_ = effective_perms_from_dacl(dacl, lookupSid);
                    LocalFree(lookupSid);
                }
            }

            // others: Everyone
            PSID everyoneSid = nullptr;
            if (!ConvertStringSidToSidA("S-1-1-0", &everyoneSid)) {
                everyoneSid = nullptr;
            }
            if (everyoneSid != nullptr) {
                others_ = effective_perms_from_dacl(dacl, everyoneSid);
                LocalFree(everyoneSid);
            }
        }

        if (secDesc != nullptr) {
            LocalFree(secDesc);
        }

#else
        // POSIX
        owner_ = {false, false, false};
        group_ = {false, false, false};
        others_ = {false, false, false};

        struct stat st;
        if (stat(path_.c_str(), &st) == 0) {
            mode_t m = st.st_mode;
            owner_.read = (m & S_IRUSR) != 0;
            owner_.write = (m & S_IWUSR) != 0;
            owner_.execute = (m & S_IXUSR) != 0;

            group_.read = (m & S_IRGRP) != 0;
            group_.write = (m & S_IWGRP) != 0;
            group_.execute = (m & S_IXGRP) != 0;

            others_.read = (m & S_IROTH) != 0;
            others_.write = (m & S_IWOTH) != 0;
            others_.execute = (m & S_IXOTH) != 0;
        }
#endif

        loaded_ = true;
    }

} // namespace mino::core::file
