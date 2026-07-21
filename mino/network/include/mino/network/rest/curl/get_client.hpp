#pragma once

#ifdef USE_CURL

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <mutex>

typedef void CURL;

namespace mino::network::rest::curl {

    class  get_client
    {
    public:

        // REST API 반환값 및 에러 분류를 위한 enum
        enum class http_status
        {
            unknown = 0, // 알 수 없는 상태 (예: 네트워크 오류로 인해 HTTP 응답을 받지 못한 경우)
            ok = 200, // 성공
            created = 201, // 리소스가 성공적으로 생성됨
            no_content = 204, // 요청은 성공했지만 반환할 콘텐츠가 없음
            bad_request = 400, // 클라이언트의 요청이 잘못됨
            unauthorized = 401, // 인증이 필요하거나 실패함
            forbidden = 403, // 서버가 요청을 이해했지만 권한이 없어 거부함
            not_found = 404, // 요청한 리소스를 찾을 수 없음
            internal_server_error = 500, // 서버 내부 오류
            bad_gateway = 502, // 게이트웨이 또는 프록시 서버가 상위 서버로부터 잘못된 응답을 받음
            service_unavailable = 503 // 서버가 일시적으로 과부하 또는 유지보수로 인해 요청을 처리할 수 없음
        };

        // GET 요청 결과를 분류하기 위한 enum
        enum class result_code
        {
            ok, // 성공 (HTTP 2xx)
            curl_timeout, // cURL 타임아웃 오류
            curl_ssl_error, // cURL SSL 인증 오류
            curl_network_error, // cURL 네트워크 연결 오류
            curl_other_error, // cURL 기타 오류

            http_client_error_4xx, // HTTP 클라이언트 오류 (4xx)
            http_not_found, // HTTP 404 Not Found
            http_server_error_5xx, // HTTP 서버 오류 (5xx)
            http_redirect_3xx, // HTTP 리다이렉션 (3xx)
            http_other_error, // HTTP 기타 오류 (1xx, 2xx가 아닌 경우)

            unknown_error // 알 수 없는 오류 (예: 예외 발생)
        };

        // GET 요청 결과를 담는 구조체
        struct response
        {
            http_status  status = http_status::unknown; // HTTP 상태 코드 (알 수 없는 경우 unknown)
            long         raw_status_code = 0; // HTTP 상태 코드의 원시 값 (예: 200, 404 등)
            int          curl_error_code = 0; // cURL 오류 코드 (0이면 cURL 오류 없음)
            std::string  body; // HTTP 응답 본문
            std::string  error; // 오류 메시지 (cURL 오류 또는 예외 메시지)
            std::vector<std::string> headers; // HTTP 응답 헤더 (원시 문자열 형태)
            std::string content_type; // Content-Type 헤더 값 (없으면 빈 문자열)

            bool is_success() const // HTTP 2xx이면서 cURL 오류가 없는 경우를 성공으로 간주
            {
                if (!error.empty()) return false;
                return (raw_status_code >= 200 && raw_status_code < 300);
            }
        };

        using query_params = std::vector<std::pair<std::string, std::string>>;
        using headers = std::vector<std::pair<std::string, std::string>>;

        get_client();
        ~get_client();

        // REST API 서버 정보 설정
        void set_server(const std::string& scheme,
            const std::string& host,
            long port,
            const std::string& path);

        // HTTP 요청 헤더 설정
        void set_headers(const headers& headers);

        // HTTP 요청 타임아웃 설정 (ms 단위)
        void set_timeout_ms(long timeout_ms);

        // SSL 인증서 오류 무시 여부 설정
        void set_ignore_ssl_errors(bool ignore);

        // GET 요청 실행 (예외 발생 가능)
        response get(const query_params& query_params = {});

        // GET 요청 실행 (예외 발생 없이 결과 코드로 반환)        
        result_code get(const query_params& query_params, response& out_resp) noexcept;

        // 응답 결과 분류
        static result_code classify(const response& resp);

    private:
        CURL* curl_ = nullptr;

        std::string scheme_ = "http";
        std::string host_;
        long        port_ = 0;
        std::string path_;

        headers headers_;
        long timeout_ms_ = 30000;   // 30 seconds

        bool ignore_ssl_errors_ = false;

        static void global_init();
        static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
        static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

        std::string build_url(const query_params& query_params);
        static std::vector<std::string> build_header_lines(const headers& headers);
        static http_status to_http_status(long code);
    };


}  

#endif // USE_CURL
