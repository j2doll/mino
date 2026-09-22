#pragma once

#ifdef USE_CURL

#include <string>
#include <vector>
#include <utility>
#include <mutex>

typedef void CURL;

namespace mino::network_curl::rest {

    class put_client
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

        // PUT 요청 결과를 분류하기 위한 enum
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

        // PUT 요청 결과를 담는 구조체
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

        put_client();
        ~put_client();

        bool set_server(const std::string& scheme,
            const std::string& host,
            long port,
            const std::string& path);

        void set_headers(const headers& headers);
        bool set_timeout_ms(long timeout_ms);
        void set_ignore_ssl_errors(bool ignore);

        // PUT 요청 실행
        response put(const std::string& body, const query_params& query_params = {});

        // PUT 요청 실행 (result_code 반환)
        result_code put(const std::string& body, response& out_resp, const query_params& query_params = {}) noexcept;

        static result_code classify(const response& resp);

    private:
        CURL* curl_ = nullptr;

        std::string scheme_ = "http";
        std::string host_;
        long        port_ = 0;
        std::string path_;

        headers headers_;
        long timeout_ms_ = 30000;
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
