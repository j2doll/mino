// head_client.hpp
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>

namespace mino::network_openssl::rest {

    class head_client
    {
    public:
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

        struct response
        {
            http_status  status = http_status::unknown;
            long         raw_status_code = 0;
            int          curl_error_code = 0;
            std::string  body; // HEAD 요청은 본문이 없습니다.
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

        head_client();
        ~head_client();

        void set_server(const std::string& scheme,
            const std::string& host,
            long port,
            const std::string& path);

        void set_headers(const headers& headers);
        void set_timeout_ms(long timeout_ms);
        void set_ignore_ssl_errors(bool ignore);

        response head(const query_params& query_params = {});
        result_code head(const query_params& query_params, response& out_resp) noexcept;

        static result_code classify(const response& resp);

    private:
        struct impl;
        std::unique_ptr<impl> impl_;

        std::string scheme_ = "http";
        std::string host_;
        long        port_ = 0;
        std::string path_;

        headers headers_;
        long timeout_ms_ = 30000;
        bool ignore_ssl_errors_ = false;

        static http_status to_http_status(long code);
    };

}
