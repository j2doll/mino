#pragma once

#include <string>
#include <vector>
#include <utility>

// cpp-httplib 에러 열거형 전방 선언 호환
#include "mino/network_openssl/third-party/httplib/httplib.h"

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
        std::string content_type;

        bool is_success() const
        {
            if (!error.empty()) return false;
            return (raw_status_code >= 200 && raw_status_code < 300);
        }
    };

    using query_params = std::vector<std::pair<std::string, std::string>>;
    using headers = std::vector<std::pair<std::string, std::string>>;

    inline http_status to_http_status(long code)
    {
        switch (code) {
        case 200: return http_status::ok;
        case 201: return http_status::created;
        case 204: return http_status::no_content;
        case 400: return http_status::bad_request;
        case 401: return http_status::unauthorized;
        case 403: return http_status::forbidden;
        case 404: return http_status::not_found;
        case 500: return http_status::internal_server_error;
        case 502: return http_status::bad_gateway;
        case 503: return http_status::service_unavailable;
        default:  return http_status::unknown;
        }
    }

    // result_code 영어 설명 문자열 반환 함수
    constexpr const char* to_string(result_code rc) noexcept
    {
        switch (rc) {
        case result_code::ok:                    return "[OK] Request succeeded.";
        case result_code::timeout:               return "[TIMEOUT] The request timed out.";
        case result_code::ssl_error:             return "[SSL ERROR] SSL certificate error.";
        case result_code::network_error:         return "[NETWORK ERROR] Network error (host not found or connection failed).";
        case result_code::other_error:           return "[OTHER ERROR] Other network/client error.";
        case result_code::http_client_error_4xx: return "[HTTP 4xx] Client error (4xx).";
        case result_code::http_not_found:        return "[HTTP 404] Not found.";
        case result_code::http_server_error_5xx: return "[HTTP 5xx] Server error (5xx).";
        case result_code::http_redirect_3xx:     return "[HTTP 3xx] Redirect (3xx).";
        case result_code::http_other_error:      return "[HTTP OTHER ERROR] Other HTTP error.";
        case result_code::unknown_error:
        default:                                 return "[UNKNOWN ERROR] Unknown error occurred.";
        }
    }

    // http_status 영어 설명 문자열 반환 함수
    constexpr const char* to_string(http_status status) noexcept
    {
        switch (status) {
        case http_status::ok:                    return "[HTTP 200 OK] Success.";
        case http_status::created:               return "[HTTP 201 Created] Resource created.";
        case http_status::no_content:            return "[HTTP 204 No Content] Success, no content.";
        case http_status::bad_request:           return "[HTTP 400 Bad Request] Client error.";
        case http_status::unauthorized:          return "[HTTP 401 Unauthorized] Authentication required.";
        case http_status::forbidden:             return "[HTTP 403 Forbidden] Access denied.";
        case http_status::not_found:             return "[HTTP 404 Not Found] Resource not found.";
        case http_status::internal_server_error: return "[HTTP 500 Internal Server Error] Server error.";
        case http_status::bad_gateway:           return "[HTTP 502 Bad Gateway] Bad gateway.";
        case http_status::service_unavailable:   return "[HTTP 503 Service Unavailable] Service unavailable.";
        case http_status::unknown:
        default:                                 return "[HTTP UNKNOWN] Unknown HTTP status.";
        }
    }

    inline result_code classify(const response& r)
    {
        if (!r.error.empty()) {
            auto err = static_cast<::httplib::Error>(r.error_code);
            switch (err) {
            case ::httplib::Error::ConnectionTimeout:
            case ::httplib::Error::Read:
                return result_code::timeout;
            case ::httplib::Error::SSLConnection:
            case ::httplib::Error::SSLLoadingCerts:
            case ::httplib::Error::SSLServerVerification:
                return result_code::ssl_error;
            case ::httplib::Error::Connection:
            case ::httplib::Error::BindIPAddress:
            case ::httplib::Error::ProxyConnection:
                return result_code::network_error;
            default:
                return result_code::other_error;
            }
        }

        if (r.raw_status_code >= 200 && r.raw_status_code < 300) return result_code::ok;
        if (r.raw_status_code == 404) return result_code::http_not_found;
        if (r.raw_status_code >= 400 && r.raw_status_code < 500) return result_code::http_client_error_4xx;
        if (r.raw_status_code >= 500 && r.raw_status_code < 600) return result_code::http_server_error_5xx;
        if (r.raw_status_code >= 300 && r.raw_status_code < 400) return result_code::http_redirect_3xx;
        if (r.raw_status_code > 0) return result_code::http_other_error;

        return result_code::unknown_error;
    }

}
