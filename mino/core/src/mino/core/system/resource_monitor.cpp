#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "mino/core/system/resource_monitor.hpp"

#ifdef _WIN32   
#   include <windows.h>
#   include <psapi.h>
#   include <pdh.h>
#   pragma comment(lib, "pdh.lib")
#else
#   include <sys/sysinfo.h>
#   include <sys/statvfs.h>
#   include <unistd.h>
#endif

namespace mino::core::system {

#ifdef _WIN32

// Windows CPU Usage 계산 (일반 권한)
static uint64_t file_time_to_uint64(const FILETIME& ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

double resource_monitor::get_cpu_usage() {
    static uint64_t last_idle = 0, last_kernel = 0, last_user = 0;
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return 0.0;

    uint64_t curr_idle = file_time_to_uint64(idle);
    uint64_t curr_kernel = file_time_to_uint64(kernel);
    uint64_t curr_user = file_time_to_uint64(user);

    uint64_t idle_diff = curr_idle - last_idle;
    uint64_t kernel_diff = curr_kernel - last_kernel;
    uint64_t user_diff = curr_user - last_user;
    uint64_t total_diff = kernel_diff + user_diff;

    last_idle = curr_idle; last_kernel = curr_kernel; last_user = curr_user;

    if (total_diff == 0) return 0.0;
    return (static_cast<double>(total_diff - idle_diff) / total_diff) * 100.0;
}

std::vector<double> resource_monitor::get_cpu_usage_per_core() {
    // PDH 기반 구현: 논리 코어 수만큼 "\\Processor(n)\\% Processor Time" 카운터를 만들어 사용률을 읽음.
    // 첫 호출 시에는 카운터를 설정하고 초기 샘플을 수집하므로 0.0 벡터를 반환합니다.
    static PDH_HQUERY s_query = nullptr;
    static std::vector<PDH_HCOUNTER> s_counters;
    static int s_core_count = 0;
    static bool s_initialized = false;

    if (!s_initialized) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        s_core_count = static_cast<int>(si.dwNumberOfProcessors);
        if (s_core_count <= 0) return {};

        s_counters.resize(s_core_count, nullptr);

        if (PdhOpenQuery(nullptr, 0, &s_query) != ERROR_SUCCESS) {
            // PDH 열기에 실패하면 폴백으로 전체 사용률만 반환
            return std::vector<double>(1, get_cpu_usage());
        }

        for (int i = 0; i < s_core_count; ++i) {
            char path[128];
            // "%%" -> single '%' in output string
            snprintf(path, sizeof(path), "\\Processor(%d)\\%% Processor Time", i);

            PDH_STATUS st = PdhAddEnglishCounterA(s_query, path, 0, &s_counters[i]);
            if (st != ERROR_SUCCESS) {
                // 영어 카운터 추가 실패 시 로컬 네임으로 시도
                if (PdhAddCounterA(s_query, path, 0, &s_counters[i]) != ERROR_SUCCESS) {
                    s_counters[i] = nullptr; // 실패 마킹
                }
            }
        }

        // 초기 샘플 수집 (첫 샘플은 비교값을 만들기 위해 필요)
        PdhCollectQueryData(s_query);
        s_initialized = true;

        // 첫 호출에서는 의미있는 값이 없으므로 0.0 채움
        return std::vector<double>(s_core_count, 0.0);
    }

    // 정상 경로: 샘플 수집 후 각 카운터 값 읽기
    if (s_query == nullptr) {
        return std::vector<double>(1, get_cpu_usage());
    }

    PdhCollectQueryData(s_query);

    std::vector<double> usages;
    usages.resize(s_core_count, 0.0);

    for (int i = 0; i < s_core_count; ++i) {
        if (s_counters[i] == nullptr) {
            usages[i] = 0.0;
            continue;
        }
        PDH_FMT_COUNTERVALUE val;
        PDH_STATUS st = PdhGetFormattedCounterValue(s_counters[i], PDH_FMT_DOUBLE, nullptr, &val);
        if (st == ERROR_SUCCESS) {
            usages[i] = val.doubleValue;
        } else {
            usages[i] = 0.0;
        }
    }

    return usages;
}

memory_info resource_monitor::get_memory_info() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    memory_info info;
    info.total_phys_kb = memInfo.ullTotalPhys / 1024;
    info.available_phys_kb = memInfo.ullAvailPhys / 1024;
    info.usage_percent = static_cast<double>(memInfo.dwMemoryLoad);
    return info;
}

disk_info resource_monitor::get_disk_info(const std::string& path) {
    disk_info info;
    ULARGE_INTEGER freeBytes, totalBytes, totalFree;
    // path가 비어있으면 현재 드라이브 기준
    if (GetDiskFreeSpaceExA(path.c_str(), &freeBytes, &totalBytes, &totalFree)) {
        info.mount_path = path;
        info.total_gb = totalBytes.QuadPart / (1024 * 1024 * 1024);
        info.free_gb = freeBytes.QuadPart / (1024 * 1024 * 1024);
        info.usage_percent = 100.0 - (static_cast<double>(info.free_gb) / info.total_gb * 100.0);
    }
    return info;
}

#else

// Linux CPU Usage (/proc/stat 파싱)
double resource_monitor::get_cpu_usage() {
    static uint64_t last_user, last_nice, last_system, last_idle;
    std::ifstream file("/proc/stat");
    std::string cpu;
    uint64_t user, nice, system, idle;
    file >> cpu >> user >> nice >> system >> idle;

    uint64_t user_diff = user - last_user;
    uint64_t nice_diff = nice - last_nice;
    uint64_t system_diff = system - last_system;
    uint64_t idle_diff = idle - last_idle;

    last_user = user; last_nice = nice; last_system = system; last_idle = idle;

    uint64_t total = user_diff + nice_diff + system_diff + idle_diff;
    if (total == 0) return 0.0;
    return (static_cast<double>(total - idle_diff) / total) * 100.0;
}

std::vector<double> resource_monitor::get_cpu_usage_per_core() {
    std::ifstream file("/proc/stat");
    std::string line;

    // static으로 이전 측정값을 보관
    static std::vector<uint64_t> last_totals;
    static std::vector<uint64_t> last_idles;

    std::vector<double> usages;
    size_t index = 0;

    while (std::getline(file, line)) {
        if (line.rfind("cpu", 0) != 0) break; // "cpu..." 라인이 아니면 종료
        std::istringstream ss(line);
        std::string label;
        ss >> label;
        if (label == "cpu") continue; // aggregate는 건너뜀

        // /proc/stat의 cpuN 필드: user nice system idle iowait irq softirq steal guest guest_nice
        uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
        ss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        uint64_t idle_all = idle + iowait;
        uint64_t non_idle = user + nice + system + irq + softirq + steal;
        uint64_t total = idle_all + non_idle;

        if (last_totals.size() <= index) {
            // 초기화 단계: 첫 호출에서는 사용률 계산을 위해 이전값으로 설정하고 0.0 반환
            last_totals.resize(index + 1);
            last_idles.resize(index + 1);
            last_totals[index] = total;
            last_idles[index] = idle_all;
            usages.push_back(0.0);
        } else {
            uint64_t total_diff = total - last_totals[index];
            uint64_t idle_diff = idle_all - last_idles[index];
            double usage = 0.0;
            if (total_diff != 0) {
                usage = (static_cast<double>(total_diff - idle_diff) / total_diff) * 100.0;
            }
            usages.push_back(usage);

            last_totals[index] = total;
            last_idles[index] = idle_all;
        }

        ++index;
    }

    return usages;
}

memory_info resource_monitor::get_memory_info() {
    struct sysinfo si;
    sysinfo(&si);
    memory_info info;
    info.total_phys_kb = (si.totalram * si.mem_unit) / 1024;
    info.available_phys_kb = (si.freeram * si.mem_unit) / 1024;
    info.usage_percent = 100.0 - (static_cast<double>(info.available_phys_kb) / info.total_phys_kb * 100.0);
    return info;
}

disk_info resource_monitor::get_disk_info(const std::string& path) {
    struct statvfs vfs;
    disk_info info;
    if (statvfs(path.c_str(), &vfs) == 0) {
        info.mount_path = path;
        info.total_gb = (vfs.f_blocks * vfs.f_frsize) / (1024 * 1024 * 1024);
        info.free_gb = (vfs.f_bfree * vfs.f_frsize) / (1024 * 1024 * 1024);
        info.usage_percent = 100.0 - (static_cast<double>(info.free_gb) / info.total_gb * 100.0);
    }
    return info;
}

#endif

}