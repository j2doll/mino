#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace mino::core::system {

    struct  cpu_info {
        double usage_percent = 0.0; // CPU 사용률 (0.0 ~ 100.0)
        int core_count = 0; // CPU 코어 수 (물리적 또는 논리적)
    };

    struct  memory_info {
        uint64_t total_phys_kb = 0; // 총 물리 메모리 (KB)
        uint64_t available_phys_kb = 0; // 사용 가능한 물리 메모리 (KB)
        double usage_percent = 0.0; // 메모리 사용률 (0.0 ~ 100.0)
    };

    struct  disk_info {
        std::string mount_path; // 디스크가 마운트된 경로 (예: "/", "C:\\")
        uint64_t total_gb = 0; // 총 디스크 용량 (GB)
        uint64_t free_gb = 0; // 사용 가능한 디스크 용량 (GB)
        double usage_percent = 0.0; // 디스크 사용률 (0.0 ~ 100.0)
    };

    class  resource_monitor {
    public:
        /**
            * @brief CPU 사용률을 계산합니다. (이전 호출 시점과의 차이 계산 필요)
            */
        static double get_cpu_usage();

        /**
            * @brief CPU 코어별 사용률을 반환합니다.
            * @return 각 논리 CPU 코어에 대한 사용률 벡터 (인덱스 0 => cpu0, 1 => cpu1, ...)
            *
            * 참고:
            * - Linux: `/proc/stat` 기반으로 구현되어 정확한 코어별 사용률을 제공합니다.
            * - Windows: 현재 구현은 전체 CPU 사용률만 반환하는 폴백이며, 필요하면 PDH 기반 구현 추가 가능.
            */
        static std::vector<double> get_cpu_usage_per_core();

        /**
            * @brief 메모리 정보를 가져옵니다.
            */
        static memory_info get_memory_info();

        /**
            * @brief 특정 경로(또는 루트)의 디스크 정보를 가져옵니다.
            */
        static disk_info get_disk_info(const std::string& path =
    #ifdef _WIN32
            "C:\\"
    #else 
            "/"
    #endif 
        );

    };

}