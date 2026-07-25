#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

namespace mino::core::system {

        /**
         * @brief 관리자 권한 없이 장치 고유 ID를 생성하는 클래스
         */
        class  device_id_generator {
        public:
            /**
             * @brief 현재 기기의 고유 ID(64자 해시)를 반환합니다.
             * @return std::string 64글자의 16진수 문자열
             */
            static std::string get_unique_id();

        private:
            // 내부 보조 함수들
            static std::string get_cpu_info();
            static std::string get_os_machine_id();
            static std::string get_env_info();
            static std::string simple_hash(const std::string& input);
        };

} 