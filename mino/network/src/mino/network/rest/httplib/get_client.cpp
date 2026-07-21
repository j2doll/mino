#include <algorithm>
#include <map>
#include <chrono>

#ifdef USE_OPENSSL
    #ifndef CPPHTTPLIB_OPENSSL_SUPPORT
        #define CPPHTTPLIB_OPENSSL_SUPPORT // HTTPS 지원이 필요한 경우 활성화
    #endif
#endif

#include <httplib.h>

#include "mino/network/rest/httplib/get_client.hpp"

namespace mino::network::rest::httplib {

    struct get_client::impl
    {
        impl() = default;
        ~impl() = default;

        std::unique_ptr<::httplib::ClientImpl> client_impl;

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

        static ::httplib::Params to_httplib_params(const query_params& params)
        {
            ::httplib::Params out;
            for (const auto& kv : params) {
                out.insert(std::make_pair(kv.first, kv.second));
            }
            return out;
        }
    };

    // Numeric mappings compatible with common libcurl CURLE_* values so 기존 classify 코드와 호환되게 설정
    // (값은 libcurl의 정의와 동일하게 사용)
    static constexpr int CURL_CODE_OPERATION_TIMEDOUT = 28; // CURLE_OPERATION_TIMEDOUT
    static constexpr int CURL_CODE_PEER_FAILED_VERIFICATION = 60; // CURLE_PEER_FAILED_VERIFICATION
    static constexpr int CURL_CODE_SSL_CONNECT_ERROR = 35; // CURLE_SSL_CONNECT_ERROR
    static constexpr int CURL_CODE_COULDNT_RESOLVE_HOST = 6; // CURLE_COULDNT_RESOLVE_HOST
    static constexpr int CURL_CODE_COULDNT_CONNECT = 7; // CURLE_COULDNT_CONNECT

    get_client::get_client()
        : impl_(std::make_unique<impl>())
    {
    }

    get_client::~get_client() = default;

    void get_client::set_server(const std::string& scheme,
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

        // Create client_impl according to host/port.
        // ClientImpl has constructors that accept host and port.
        if (port_ > 0) {
            impl_->client_impl = std::make_unique<::httplib::ClientImpl>(host_, static_cast<int>(port_));
        }
        else {
            impl_->client_impl = std::make_unique<::httplib::ClientImpl>(host_);
        }

        // apply ssl verification setting if supported
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
#endif
    }

    void get_client::set_headers(const headers& headers)
    {
        headers_ = headers;
    }

    void get_client::set_timeout_ms(long timeout_ms)
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

    void get_client::set_ignore_ssl_errors(bool ignore)
    {
        ignore_ssl_errors_ = ignore;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (impl_ && impl_->client_impl) {
            impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
        }
#endif
    }

    std::vector<std::string> get_client::build_header_lines(const headers& headers)
    {
        std::vector<std::string> lines;
        lines.reserve(headers.size());

        for (const auto& kv : headers) {
            if (!kv.first.empty())
                lines.push_back(kv.first + ": " + kv.second);
        }

        return lines;
    }

    get_client::http_status get_client::to_http_status(long code)
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

    get_client::response get_client::get(const query_params& query_params)
    {
        if (!impl_->client_impl)
            throw std::runtime_error("Client is not initialized. Call set_server() first.");

        response resp;

        // Build request path (base path + resource)
        std::string req_path = impl::build_request_path(path_, std::string());

        // Convert params and headers
        auto params = impl::to_httplib_params(query_params);
        auto h = impl::to_httplib_headers(headers_);

        // Perform GET
        auto result = impl_->client_impl->Get(req_path, params, h);

        if (!result) {
            // Error occurred
            resp.error = ::httplib::to_string(result.error());
            // Map httplib::Error to curl-like numeric codes for compatibility
            switch (result.error()) {
            case ::httplib::Error::Connection:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::BindIPAddress:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::Read:
                resp.curl_error_code = CURL_CODE_OPERATION_TIMEDOUT; // best-effort
                break;
            case ::httplib::Error::Write:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::ExceedRedirectCount:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::Canceled:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::SSLConnection:
                resp.curl_error_code = CURL_CODE_SSL_CONNECT_ERROR;
                break;
            case ::httplib::Error::SSLLoadingCerts:
                resp.curl_error_code = CURL_CODE_PEER_FAILED_VERIFICATION;
                break;
            case ::httplib::Error::SSLServerVerification:
                resp.curl_error_code = CURL_CODE_PEER_FAILED_VERIFICATION;
                break;
            case ::httplib::Error::UnsupportedMultipartBoundaryChars:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::Compression:
                resp.curl_error_code = CURL_CODE_COULDNT_CONNECT;
                break;
            case ::httplib::Error::ConnectionTimeout:
                resp.curl_error_code = CURL_CODE_OPERATION_TIMEDOUT;
                break;
            default:
                resp.curl_error_code = 1;
                break;
            }
            return resp;
        }

        const ::httplib::Response& r = result.value();

        resp.raw_status_code = r.status;
        resp.status = to_http_status(r.status);
        resp.body = r.body;

        // headers: convert multimap to vector lines and find Content-Type
        for (const auto& kv : r.headers) {
            std::string line = kv.first + ": " + kv.second;
            resp.headers.push_back(line);
            if (!resp.content_type.empty()) continue;
            const std::string key = "Content-Type";
            if (kv.first.size() >= key.size() &&
                std::equal(key.begin(), key.end(), kv.first.begin(),
                    [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                resp.content_type = kv.second;
            }
        }

        return resp;
    }

    get_client::result_code get_client::get(const query_params& query_params, response& out_resp) noexcept
    {
        try {
            out_resp = get(query_params);
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

    get_client::result_code get_client::classify(const response& r)
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

} // namespace mino::network::rest::httplib
