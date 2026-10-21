#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "mino/core/system/system.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// ==========================================
// 3. resource_monitor 테스트
// ==========================================
void test_resource_monitor() {
    // 출력을 위한 람다 및 인코딩 보조 유틸리티 정의
    const auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    const auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;
    auto tce = mino::core::string::to_console_encoding;

    namespace mcsys = mino::core::system;
    using resource_monitor = mcsys::resource_monitor;

    print(tce("[TEST] 3. resource_monitor 테스트 시작"));

    // CPU 사용률 테스트 (이전 호출 대비 차이를 계산하므로 약간의 딜레이 후 2회 호출)
    double cpu_usage1 = resource_monitor::get_cpu_usage(); // 전체 CPU 사용률 첫 번째 샘플
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 잠시 대기

    double cpu_usage2 = resource_monitor::get_cpu_usage(); // 전체 CPU 사용률 두 번째 샘플
    print(tce("  -> 전체 CPU 사용률: "), cpu_usage2,
        tce("% (이전 샘플: "), cpu_usage1, tce("%)"));
    assert(cpu_usage2 >= 0.0 && cpu_usage2 <= 100.0);

    // 코어별 CPU 사용률 테스트
    auto core_usages = resource_monitor::get_cpu_usage_per_core(); // 코어별 CPU 사용률 첫 번째 샘플
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 잠시 대기
    core_usages = resource_monitor::get_cpu_usage_per_core(); // 코어별 CPU 사용률 두 번째 샘플

    print(tce("  -> 코어 개수: "), std::to_string(core_usages.size())); // 코어 개수 출력
    for (size_t i = 0; i < core_usages.size(); ++i) {
        print(tce("     - Core "), std::to_string(i), tce(": "), core_usages[i], tce("%"));
        assert(core_usages[i] >= 0.0 && core_usages[i] <= 100.0);
    }

    // 메모리 정보 테스트
    mcsys::memory_info mem = resource_monitor::get_memory_info(); // 메모리 정보 수집
    print(tce("  -> 메모리 정보:"));
    print(tce("     - 총 물리 메모리: "), (mem.total_phys_kb / 1024), tce(" MB"));
    print(tce("     - 사용 가능 메모리: "), (mem.available_phys_kb / 1024), tce(" MB"));
    print(tce("     - 사용률: "), mem.usage_percent, tce("%"));
    assert(mem.total_phys_kb > 0);
    assert(mem.usage_percent >= 0.0 && mem.usage_percent <= 100.0);

    // 디스크 정보 테스트 (기본 루트 경로)
    mcsys::disk_info disk = resource_monitor::get_disk_info(); // 디스크 정보 수집
    print(tce("  -> 디스크 정보 ("), disk.mount_path, tce("):"));
    print(tce("     - 총 용량: "), disk.total_gb, tce(" GB"));
    print(tce("     - 여유 공간: "), disk.free_gb, tce(" GB"));
    print(tce("     - 사용률: "), disk.usage_percent, tce("%"));
    assert(disk.total_gb > 0);
    assert(disk.usage_percent >= 0.0 && disk.usage_percent <= 100.0);

    print(tce("[PASS] resource_monitor 테스트 완료\n"));
}

