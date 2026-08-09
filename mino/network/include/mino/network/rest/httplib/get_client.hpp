#pragma once

#include <string>
#include <vector>
#include <utility>
#include <mutex>
#include <memory>

namespace mino::network::rest::httplib {

    class get_client
    {
    public:

        // REST API 반환값 및 에러 분류를 위한 enum
        enum class http_status
        {
            unknown = 0,
            ok = 200,
            created = 201,
            no_content = 204,
            bad_request = 400,
            unauthorized = 401,
            forbidden = 403,
            not_found = 404,
            internal_server_error = 500,
            bad_gateway = 502,
            service_unavailable = 503
        };

        // GET 요청 결과를 분류하기 위한 enum
        enum class result_code
        {
            ok,
            curl_timeout,
            curl_ssl_error,
            curl_network_error,
            curl_other_error,

            http_client_error_4xx,
            http_not_found,
            http_server_error_5xx,
            http_redirect_3xx,
            http_other_error,

            unknown_error
        };

        // GET 요청 결과를 담는 구조체
        struct response
        {
            http_status  status = http_status::unknown;
            long         raw_status_code = 0;
            int          curl_error_code = 0;
            std::string  body;
            std::string  error;
            std::vector<std::string> headers;
            std::string content_type;

            bool is_success() const
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

        // GET 요청 실행
        response get(const query_params& query_params = {});

        // GET 요청 실행 (결과 코드로 반환)
        result_code get(const query_params& query_params, response& out_resp) noexcept;

        // 응답 결과 분류
        static result_code classify(const response& resp);

    private:
        // httplib 내부 클라이언트 포인터 (pimpl 형태)
        struct impl;
        std::unique_ptr<impl> impl_;

        std::string scheme_ = "http";
        std::string host_;
        long        port_ = 0;
        std::string path_;

        headers headers_;
        long timeout_ms_ = 30000;   // 30 seconds

        bool ignore_ssl_errors_ = false;

        static std::vector<std::string> build_header_lines(const headers& headers);
        static http_status to_http_status(long code);
    };

}
