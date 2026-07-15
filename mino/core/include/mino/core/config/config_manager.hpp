#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <optional>

namespace mino::core::config {

        /**
         * @brief 설정 파일의 각 줄 정보를 저장하는 구조체
         */
        struct  config_line {
            std::string key;        // 설정 키 (데이터인 경우에만 존재)
            std::string value;      // 설정 값 (데이터인 경우에만 존재)
            std::string raw_text;   // 원본 텍스트 (주석이나 빈 줄 보존용)
            bool is_data;           // 실제 'key=value' 데이터인지 여부
        };

        class  config_manager {
        public:
            /**
             * @brief Singleton 인스턴스 반환
             */
            static config_manager& get_instance();

            /**
             * @brief 기본 설정 파일 경로 설정 (이후 인수 없이 load()/save() 사용 가능)
             */
            void set_config_file(const std::string& file_path);

            /**
             * @brief 설정 파일을 읽어 메모리에 로드 (주석 포함)
             */
            bool load(const std::string& file_path);
            bool load(); // use previously set config file path

            /**
             * @brief 현재 메모리의 설정을 파일로 저장 (주석 및 순서 보존)
             */
            bool save(const std::string& file_path);
            bool save(); // use previously set config file path

            /**
             * @brief 문자열 값 가져오기 (존재하지 않으면 std::nullopt)
             */
            std::optional<std::string> get_string(const std::string& key);

            /**
             * @brief 문자열 값 가져오기 (존재하지 않으면 default_val 반환)
             */
            std::string get_string_or(const std::string& key, const std::string& default_val = "");

            /**
             * @brief 정수 값 가져오기 (존재하지 않으면 std::nullopt)
             */
            std::optional<int> get_int(const std::string& key);

            /**
             * @brief 정수 값 가져오기 (기존 스타일: 존재하지 않으면 default_val 반환)
             */
            int get_int(const std::string& key, int default_val);
            int get_int_or(const std::string& key, int default_val = 0);

            /**
             * @brief 불리언 값 가져오기 (존재하지 않으면 std::nullopt)
             */
            std::optional<bool> get_bool(const std::string& key);

            /**
             * @brief 불리언 값 가져오기 (기존 스타일: 존재하지 않으면 default_val 반환)
             */
            bool get_bool(const std::string& key, bool default_val);
            bool get_bool_or(const std::string& key, bool default_val = false);

            /**
             * @brief 실수(double) 값 가져오기 (존재하지 않으면 std::nullopt)
             */
            std::optional<double> get_double(const std::string& key);

            /**
             * @brief 실수(double) 값 가져오기 (기존 스타일: 존재하지 않으면 default_val 반환)
             */
            double get_double(const std::string& key, double default_val);
            double get_double_or(const std::string& key, double default_val = 0.0);

            /**
             * @brief 파일에 있는 모든 라인 정보를 복사해서 반환 (데이터/주석/빈줄 포함)
             */
            std::vector<config_line> get_all() const;

            /**
             * @brief 값 설정 (기존 키가 있으면 해당 줄 업데이트, 없으면 맨 뒤에 추가)
             */
            void set(const std::string& key, const std::string& value);

        private:
            config_manager() = default;
            ~config_manager() = default;
            config_manager(const config_manager&) = delete;
            config_manager& operator=(const config_manager&) = delete;

            // 내부 유틸리티: 공백 제거
            void trim(std::string& s);

            // 내부 유틸리티: 따옴표 및 이스케이프 처리
            std::string process_escape(std::string val);

            std::vector<config_line> m_lines; // 파일의 모든 줄 정보를 순서대로 저장
            mutable std::mutex m_mutex;

            // 저장된 기본 config 파일 경로
            std::string m_config_file_path;
        };

} // namespace mino::core::config
