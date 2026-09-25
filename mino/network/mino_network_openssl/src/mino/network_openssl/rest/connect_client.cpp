// connect_client.cpp
#include <sstream>
#include <algorithm>
#include <map>
#include <chrono>
#include <cctype>

#ifdef USE_OPENSSL
#   ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#       define CPPHTTPLIB_OPENSSL_SUPPORT
#   endif
#endif

#include "mino/network_openssl/third-party/httplib/httplib.h"
#include "mino/network_openssl/rest/connect_client.hpp"

namespace mino::network_openssl::rest {

    struct connect_client::impl
    {
        impl() = default;
        ~impl() = default;

        std::unique_ptr<::httplib::Client> client_impl;

        static ::httplib::Headers to_httplib_headers(const headers& hdrs)
        {
            ::httplib::Headers out;
            for (const auto& kv : hdrs) {
                if (!kv.first.empty()) out.insert(std::make_pair(kv.first, kv.second));
            }
            return out;
        }
    };

    static constexpr int CURL_CODE_OPERATION_TIMEDOUT = 28;
    static constexpr int CURL_CODE_PEER_FAILED_VERIFICATION = 60;
    static constexpr int CURL_CODE_SSL_CONNECT_ERROR = 35;
    static constexpr int CURL_CODE_COULDNT_RESOLVE_HOST = 6;
    static constexpr int CURL_CODE_COULDNT_CONNECT = 7;

    connect_client::connect_client() : impl_(std::make_unique<impl>()) {}
    connect_client::~connect_client() = default;

    void connect_client::set_server(const std::string& scheme, const std::string& host, long port, const std::string& path)
    {
        if (scheme.empty() || host.empty()) return;

        scheme_ = scheme;
        host_ = host;
        port_ = port;
        path_ = path;

        std::string scheme_lower = scheme_;
        std::transform(scheme_lower.begin(), scheme_lower.end(), scheme_lower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        std::ostringstream url_oss;
        url_oss << scheme_lower << "://" << host_;
        if (port_ > 0) url_oss << ":" << port_;

        impl_->client_impl = std::make_unique<::httplib::Client>(url_oss.str());
        set_timeout_ms(timeout_ms_);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
#endif
    }

    void connect_client::set_headers(const headers& headers) { headers_ = headers; }

    void connect_client::set_timeout_ms(long timeout_ms)
    {
        if (timeout_ms <= 0) return;
        timeout_ms_ = timeout_ms;
        if (!impl_->client_impl) return;

        time_t sec = static_cast<time_t>(timeout_ms_ / 1000);
        time_t usec = static_cast<time_t>((timeout_ms_ % 1000) * 1000);

        impl_->client_impl->set_connection_timeout(sec, usec);
        impl_->client_impl->set_read_timeout(sec, usec);
        impl_->client_impl->set_write_timeout(sec, usec);
    }

    void connect_client::set_ignore_ssl_errors(bool ignore)
    {
        ignore_ssl_errors_ = ignore;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (impl_ && impl_->client_impl) {
            impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
        }
#endif
    }

    connect_client::http_status connect_client::to_http_status(long code)
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

    connect_client::response connect_client::connect(const std::string& target_authority)
    {
        response resp;
        if (!impl_->client_impl) {
            resp.error = "Client is not initialized. Call set_server() first.";
            return resp;
        }

        std::string target = target_authority;
        if (target.empty()) {
            if (!path_.empty() && path_ != "/") {
                target = path_;
            }
            else {
                target = host_ + ":" + std::to_string(port_ > 0 ? port_ : 80);
            }
        }

        auto h = impl::to_httplib_headers(headers_);

        ::httplib::Request req;
        req.method = "CONNECT";
        req.path = target;
        req.headers = h;

        auto result = impl_->client_impl->send(req);
        if (!result) {
            resp.error = ::httplib::to_string(result.error());
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

        const ::httplib::Response& r = result.value();
        resp.raw_status_code = r.status;
        resp.status = to_http_status(r.status);
        resp.body = r.body;

        for (const auto& header : r.headers) {
            resp.headers.push_back(header.first + ": " + header.second);
        }
        if (r.has_header("Content-Type")) {
            resp.content_type = r.get_header_value("Content-Type");
        }

        return resp;
    }

    connect_client::result_code connect_client::classify(const response& r)
    {
        if (!r.error.empty()) {
            if (r.curl_error_code == CURL_CODE_OPERATION_TIMEDOUT) return result_code::curl_timeout;
            if (r.curl_error_code == CURL_CODE_PEER_FAILED_VERIFICATION || r.curl_error_code == CURL_CODE_SSL_CONNECT_ERROR) return result_code::curl_ssl_error;
            if (r.curl_error_code == CURL_CODE_COULDNT_RESOLVE_HOST || r.curl_error_code == CURL_CODE_COULDNT_CONNECT) return result_code::curl_network_error;
            return result_code::curl_other_error;
        }

        if (r.raw_status_code >= 200 && r.raw_status_code < 300) return result_code::ok;
        if (r.raw_status_code == 404) return result_code::http_not_found;
        if (r.raw_status_code >= 400 && r.raw_status_code < 500) return result_code::http_client_error_4xx;
        if (r.raw_status_code >= 500 && r.raw_status_code < 600) return result_code::http_server_error_5xx;
        if (r.raw_status_code >= 300 && r.raw_status_code < 400) return result_code::http_redirect_3xx;
        if (r.raw_status_code > 0) return result_code::http_other_error;
        return result_code::unknown_error;
    }

    connect_client::result_code connect_client::connect(response& out_resp) noexcept
    {
        out_resp = connect();
        return classify(out_resp);
    }

    connect_client::result_code connect_client::connect(const std::string& target_authority, response& out_resp) noexcept
    {
        out_resp = connect(target_authority);
        return classify(out_resp);
    }

}
