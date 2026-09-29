#pragma once

#include <string>
#include <vector>
#include <utility>

namespace mino::network_openssl::rest {

    // REST API HTTP 응답 상태 코드
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

    // REST 요청 결과 분류
    enum class result_code
    {
        ok,
        timeout,
        ssl_error,
        network_error,
        other_error,

        http_client_error_4xx,
        http_not_found,
        http_server_error_5xx,
        http_redirect_3xx,
        http_other_error,

        unknown_error
    };

    // REST 응답 구조체
    struct response
    {
        http_status  status = http_status::unknown;
        long         raw_status_code = 0;
        int          error_code = 0; // httplib::Error 원시 값
        std::string  body;
        std::string  error;
        std::vector<std::string> headers;
        std::string  content_type;

        bool is_success() const;
    };

    using query_params = std::vector<std::pair<std::string, std::string>>;
    using headers = std::vector<std::pair<std::string, std::string>>;

    http_status to_http_status(long code);

    // result_code 영어 설명 문자열 반환 함수
    const char* to_string(result_code rc) noexcept;

    // http_status 영어 설명 문자열 반환 함수
    const char* to_string(http_status status) noexcept;

    result_code classify(const response& r);

} // namespace mino::network_openssl::rest
