#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <functional>

#include "mino/core/system/device_id.hpp"

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <intrin.h>
#else
#   include <unistd.h>
#   include <sys/utsname.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <cpuid.h>
#endif

namespace mino::core::system {

        std::string device_id_generator::get_unique_id() {
            std::stringstream ss;

            ss << get_cpu_info();
            ss << get_os_machine_id();
            ss << get_env_info();

            return simple_hash(ss.str());
        }

        std::string device_id_generator::get_cpu_info() {
            std::stringstream ss;
#ifdef _WIN32
            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            ss << "CPUs:" << sys_info.dwNumberOfProcessors;

            // CPUID를 통한 아키텍처 식별 (간략화)
            ss << "Arch:" << sys_info.wProcessorArchitecture;
#else
            long n_procs = sysconf(_SC_NPROCESSORS_ONLN);
            ss << "CPUs:" << n_procs;
#endif
            return ss.str();
        }

        std::string device_id_generator::get_os_machine_id() {
#ifdef _WIN32
            // Windows: C 드라이브 볼륨 시리얼 (관리자 권한 불필요)
            DWORD serial_number = 0;
            if (GetVolumeInformationA("C:\\", NULL, 0, &serial_number, NULL, NULL, NULL, 0)) {
                std::stringstream ss;
                ss << std::hex << serial_number;
                return ss.str();
            }
            return "win-vol-fallback";
#else
            // Linux: 표준 machine-id 파일 읽기
            const char* paths[] = { "/etc/machine-id", "/var/lib/dbus/machine-id" };
            for (const char* path : paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    std::string id;
                    file >> id;
                    return id;
                }
            }
            return "linux-machine-fallback";
#endif
        }

        std::string device_id_generator::get_env_info() {
#ifdef _WIN32
            char buffer[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD size = sizeof(buffer);
            if (GetComputerNameA(buffer, &size)) return std::string(buffer);
#else
            char buffer[256];
            if (gethostname(buffer, sizeof(buffer)) == 0) return std::string(buffer);
#endif
            return "unknown-host";
        }

        std::string device_id_generator::simple_hash(const std::string& input) {
            // std::hash를 조합하여 64자 해시 시뮬레이션
            // 보안이 중요한 라이선스 시스템이라면 실제 SHA-256 라이브러리 연동을 권장합니다.
            size_t h1 = std::hash<std::string>{}(input);
            size_t h2 = std::hash<std::string>{}(input + "j2_salt_v1");
            size_t h3 = std::hash<std::string>{}(input + "device_id_key");
            size_t h4 = std::hash<std::string>{}(std::to_string(h1) + std::to_string(h2));

            std::stringstream ss;
            ss << std::hex << std::setfill('0')
                << std::setw(16) << h1
                << std::setw(16) << h2
                << std::setw(16) << h3
                << std::setw(16) << h4;

            return ss.str(); // 총 64자 (16*4)
        }


}