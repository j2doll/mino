#pragma once

#include <string>
#include <filesystem>

namespace mino::core::file {

    // file_info 클래스: 파일/디렉토리 정보 조회
    // - 경로 존재 여부, 파일/디렉토리 여부, 이름/확장자/상위 경로, 절대 경로, 파일 크기 등
    class  file_info {
    public:
        explicit file_info(const std::string& path);

        bool exists() const; // 경로 존재 여부
        bool is_file() const; // 파일 여부
        bool is_dir() const; // 디렉토리 여부

        std::string file_name() const; // 파일명(확장자 포함)
        std::string extension() const; // 확장자(점 포함, 예: ".txt", 없으면 빈 문자열)
        std::string parent_path() const; // 상위 경로(디렉토리 경로)
        std::string absolute_path() const; // 절대 경로(심볼릭 링크 해제, 정규화 포함)

        std::uintmax_t file_size() const; // 파일 크기(바이트 단위, 존재하지 않거나 디렉토리인 경우 0 반환)

    private:
        std::filesystem::path path_;
    };

}  
