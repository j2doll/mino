#include <sstream>
#include <algorithm>
#include <map>
#include <chrono>
#include <cctype>

#ifdef USE_OPENSSL
    #ifndef CPPHTTPLIB_OPENSSL_SUPPORT
        #define CPPHTTPLIB_OPENSSL_SUPPORT // HTTPS 지원이 필요한 경우 활성화
    #endif
#endif

#include <httplib.h>

#include "mino/network/rest/httplib/post_client.hpp"

namespace mino::network::rest::httplib {

    struct post_client::impl
    {
        impl() = default;
        ~impl() = default;

        // ClientImpl 대신 상위 공식 인터페이스 클래스인 Client를 사용합니다.
        std::unique_ptr<::httplib::Client> client_impl;

        // helper to build full request path (base path + relative path)
        static std::string build_request_path(const std::string& base_path,
            const std::string& resource_path)
        {
            std::ostringstream oss;
            if (!base_path.empty()) {
                if (base_path.front() != '/')
                    oss << '/';
                oss << base_path;
            }
            if (!resource_path.empty()) {
                if (resource_path.front() != '/' && oss.str().empty())
                    oss << '/';
                else if (resource_path.front() != '/' && !oss.str().empty() && oss.str().back() != '/')
                    oss << '/';
                oss << resource_path;
            }
            auto s = oss.str();
            if (s.empty()) s = "/";
            return s;
        }

        static ::httplib::Headers to_httplib_headers(const headers& hdrs)
        {
            ::httplib::Headers out;
            for (const auto& kv : hdrs) {
                if (!kv.first.empty())
                    out.insert(std::make_pair(kv.first, kv.second));
            }
            return out;
        }
    };

    // Numeric mappings compatible with common libcurl CURLE_* values
    static constexpr int CURL_CODE_OPERATION_TIMEDOUT = 28;
    static constexpr int CURL_CODE_PEER_FAILED_VERIFICATION = 60;
    static constexpr int CURL_CODE_SSL_CONNECT_ERROR = 35;
    static constexpr int CURL_CODE_COULDNT_RESOLVE_HOST = 6;
    static constexpr int CURL_CODE_COULDNT_CONNECT = 7;

    post_client::post_client()
        : impl_(std::make_unique<impl>())
    {
    }

    post_client::~post_client() = default;

    void post_client::set_server(const std::string& scheme,
        const std::string& host,
        long port,
        const std::string& path)
    {
        if (scheme.empty()) throw std::invalid_argument("scheme is empty.");
        if (host.empty())   throw std::invalid_argument("host is empty.");

        scheme_ = scheme;
        host_ = host;
        port_ = port;
        path_ = path;

        // scheme을 소문자로 안전하게 변환
        std::string scheme_lower = scheme_;
        std::transform(scheme_lower.begin(), scheme_lower.end(), scheme_lower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // scheme://host:port 주소 구문을 만들어 Client 생성 (HTTP/HTTPS 자동 분기 처리 지원)
        std::ostringstream url_oss;
        url_oss << scheme_lower << "://" << host_;
        if (port_ > 0) {
            url_oss << ":" << port_;
        }

        impl_->client_impl = std::make_unique<::httplib::Client>(url_oss.str());

        // 순서 변경 버그 방지: set_server가 나중에 호출되더라도 기존 타임아웃 세팅을 반영함
        set_timeout_ms(timeout_ms_);

        // SSL 검증 옵션 설정 적용
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
#endif
    }

    void post_client::set_headers(const headers& headers)
    {
        headers_ = headers;
    }

    void post_client::set_timeout_ms(long timeout_ms)
    {
        if (timeout_ms <= 0) throw std::invalid_argument("timeoutMs must be greater than 0.");
        timeout_ms_ = timeout_ms;

        if (!impl_->client_impl) return;

        // set connection and read timeouts (seconds, usec)
        time_t sec = static_cast<time_t>(timeout_ms_ / 1000);
        time_t usec = static_cast<time_t>((timeout_ms_ % 1000) * 1000);

        impl_->client_impl->set_connection_timeout(sec, usec);
        impl_->client_impl->set_read_timeout(sec, usec);
        impl_->client_impl->set_write_timeout(sec, usec);
    }

    void post_client::set_ignore_ssl_errors(bool ignore)
    {
        ignore_ssl_errors_ = ignore;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (impl_ && impl_->client_impl) {
            impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
        }
#endif
    }

    std::vector<std::string> post_client::build_header_lines(const headers& headers)
    {
        std::vector<std::string> lines;
        lines.reserve(headers.size());

        for (const auto& kv : headers) {
            if (!kv.first.empty())
                lines.push_back(kv.first + ": " + kv.second);
        }

        return lines;
    }

    post_client::http_status post_client::to_http_status(long code)
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

    // ------------------------------
    // POST (exception version)
    // ------------------------------
    post_client::response post_client::post(const std::string& body_str)
    {
        if (!impl_->client_impl)
            throw std::runtime_error("Client is not initialized. Call set_server() first.");

        response resp;

        // Build request path (base path)
        std::string req_path = impl::build_request_path(path_, std::string());

        // Convert headers
        auto h = impl::to_httplib_headers(headers_);

        // 요청용 Content-Type 추출 (대소문자 무관하게 처리하기 위해 안전하게 검색)
        std::string content_type = "text/plain";
        for (const auto& kv : h) {
            if (kv.first.size() == 12) {
                bool match = std::equal(kv.first.begin(), kv.first.end(), "Content-Type",
                    [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
                if (match) {
                    content_type = kv.second;
                    break;
                }
            }
        }

        // Perform POST with headers + body + content_type
        auto result = impl_->client_impl->Post(req_path, h, body_str, content_type);

        if (!result) {
            // Error occurred
            resp.error = ::httplib::to_string(result.error());
            // Map httplib::Error to curl-like numeric codes for compatibility
            switch (result.error()) {
            case ::httplib::Error::Connection:
            case ::httplib::Error::BindIPAddress:
            case ::httplib::Error::Write:
            case ::httplib::Error::ExceedRedirectCount:
            case ::httplib::Error::Canceled:
            case ::httplib::Error::UnsupportedMultipartBoundaryChars:
            case ::httplib::Error::Compression:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::Read:
            case ::httplib::Error::ConnectionTimeout:
                resp.curl_error_code = CURL_CODE_OPERATION_TIMEDOUT;
                break;
            case ::httplib::Error::SSLConnection:
                resp.curl_error_code = CURL_CODE_SSL_CONNECT_ERROR;
                break;
            case ::httplib::Error::SSLLoadingCerts:
            case ::httplib::Error::SSLServerVerification:
                resp.curl_error_code = CURL_CODE_PEER_FAILED_VERIFICATION;
                break;
            default:
                resp.curl_error_code = 1;
                break;
            }
            return resp;
        }

        // 결과 응답 매핑
        const ::httplib::Response& r = result.value();

        resp.raw_status_code = r.status;
        resp.status = to_http_status(r.status);
        resp.body = r.body;

        // 서버에서 받아온 진짜 응답 헤더(Response Headers) 파싱 및 저장 (버그 수정)
        for (const auto& header : r.headers) {
            resp.headers.push_back(header.first + ": " + header.second);
        }

        // 응답 본문의 Content-Type 추출 및 저장
        if (r.has_header("Content-Type")) {
            resp.content_type = r.get_header_value("Content-Type");
        }

        return resp;
    }

    post_client::result_code post_client::classify(const response& r)
    {
        if (!r.error.empty()) {
            if (r.curl_error_code == CURL_CODE_OPERATION_TIMEDOUT)
                return result_code::curl_timeout;

            if (r.curl_error_code == CURL_CODE_PEER_FAILED_VERIFICATION ||
                r.curl_error_code == CURL_CODE_SSL_CONNECT_ERROR)
                return result_code::curl_ssl_error;

            if (r.curl_error_code == CURL_CODE_COULDNT_RESOLVE_HOST ||
                r.curl_error_code == CURL_CODE_COULDNT_CONNECT)
                return result_code::curl_network_error;

            return result_code::curl_other_error;
        }

        if (r.raw_status_code >= 200 && r.raw_status_code < 300)
            return result_code::ok;

        if (r.raw_status_code == 404)
            return result_code::http_not_found;

        if (r.raw_status_code >= 400 && r.raw_status_code < 500)
            return result_code::http_client_error_4xx;

        if (r.raw_status_code >= 500 && r.raw_status_code < 600)
            return result_code::http_server_error_5xx;

        if (r.raw_status_code >= 300 && r.raw_status_code < 400)
            return result_code::http_redirect_3xx;

        if (r.raw_status_code > 0)
            return result_code::http_other_error;

        return result_code::unknown_error;
    }

    // ------------------------------
    // POST (noexcept version)
    // ------------------------------
    post_client::result_code
        post_client::post(const std::string& body_str, response& out_resp) noexcept
    {
        try {
            out_resp = post(body_str);
            return classify(out_resp);
        }
        catch (const std::exception& ex) {
            out_resp.error = ex.what();
            return result_code::unknown_error;
        }
        catch (...) {
            out_resp.error = "Unknown exception occurred";
            return result_code::unknown_error;
        }
    }

}  
