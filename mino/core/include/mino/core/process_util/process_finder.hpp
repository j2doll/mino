#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace mino::core::process_util {

    struct process_info {
        uint32_t pid;
        std::string process_name;
    };

    // 현재 실행 중인 모든 프로세스 목록을 가져오는 함수
    std::vector<process_info> get_active_processes();

    // 방법 A: 이미 가지고 있는 프로세스 목록 내에서 특정 문자열을 검색 (새로 추가됨)
    std::vector<process_info> get_processes_by_name(
        const std::vector<process_info>& src_processes,
        std::string_view target_name,
        bool case_sensitive = true
    );

    // 방법 B: 내부에서 get_active_processes()를 직접 호출하여 검색 (기존 함수 유지)
    std::vector<process_info> get_processes_by_name(
        std::string_view target_name,
        bool case_sensitive = true
    );

} 