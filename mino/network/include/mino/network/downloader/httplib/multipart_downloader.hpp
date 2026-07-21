#pragma once

#include <string>
#include <vector>

namespace mino::network::downloader::httplib
{
    // 진행 현황 콜백 인터페이스
    class  multipart_progress_callback
    {
    public:
        virtual ~multipart_progress_callback() = default;

        // total_bytes: 전체 크기 (알 수 없으면 0)
        // now_bytes  : 현재까지 받은 바이트
        // false 반환 시 다운로드 중단
        virtual bool on_progress(long long total_bytes, long long now_bytes) = 0;
    };

    // multipart/form-data 응답을 다운로드하는 클래스
    class  multipart_downloader
    {
    public:
        multipart_downloader();
        ~multipart_downloader();

        // 진행 콜백 등록 (nullptr 이면 비활성)
        void set_progress_callback(multipart_progress_callback* cb);

        // 다운로드 실패 시 임시 파일(.part) 삭제 여부 설정
        void set_delete_partial_file_on_fail(bool enable);

        // multipart/form-data 응답 다운로드
        //  인자
        //   url          : 다운로드할 URL (http://... 또는 https://...)
        //   output_dir   : 파일을 저장할 디렉토리 경로
        //   saved_files  : 저장된 파일 경로 목록 (출력)
        //   error_message: 오류 메시지 (출력, nullptr 가능)
        //  반환값
        //   성공 시 true, 실패 시 false 반환
        bool download_multipart(const std::string& url,
            const std::string& output_dir,
            std::vector<std::string>& saved_files,
            std::string* error_message = nullptr);

    public:
        multipart_progress_callback* progress_cb_{ nullptr };

    private:
        bool delete_partial_on_fail_{ true }; // 다운로드 실패 시 임시 파일 삭제 여부

        multipart_downloader(const multipart_downloader&) = delete;
        multipart_downloader& operator=(const multipart_downloader&) = delete;
    };

} 
