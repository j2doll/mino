// options_client.cpp
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef USE_OPENSSL
#   ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#       define CPPHTTPLIB_OPENSSL_SUPPORT
#   endif
#endif

#include "mino/network_openssl/third-party/httplib/httplib.h"
#include "mino/network_openssl/rest/options_client.hpp"

namespace mino::network_openssl::rest {

    struct options_client::impl
    {
        impl() = default;
        ~impl() = default;

        std::unique_ptr<::httplib::Client> client_impl;

        static std::string build_request_path(const std::string& base_path, const std::string& resource_path)
        {
            std::ostringstream oss;
            if (!base_path.empty()) {
                if (base_path.front() != '/') oss << '/';
                oss << base_path;
            }
            if (!resource_path.empty()) {
                if (resource_path.front() != '/' && oss.str().empty()) oss << '/';
                else if (resource_path.front() != '/' && !oss.str().empty() && oss.str().back() != '/') oss << '/';
                oss << resource_path;
            }
            auto s = oss.str();
            return s.empty() ? "/" : s;
        }

        static ::httplib::Headers to_httplib_headers(const headers& hdrs)
        {
            ::httplib::Headers out;
            for (const auto& kv : hdrs) {
                if (!kv.first.empty()) out.insert(std::make_pair(kv.first, kv.second));
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

    options_client::options_client() : impl_(std::make_unique<impl>()) {}
    options_client::~options_client() = default;

    void options_client::set_server(const std::string& scheme, const std::string& host, long port, const std::string& path)
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

    void options_client::set_headers(const headers& headers) { headers_ = headers; }

    void options_client::set_timeout_ms(long timeout_ms)
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

    void options_client::set_ignore_ssl_errors(bool ignore)
    {
        ignore_ssl_errors_ = ignore;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (impl_ && impl_->client_impl) {
            impl_->client_impl->enable_server_certificate_verification(!ignore_ssl_errors_);
        }
#endif
    }

    options_client::response options_client::options(const query_params& q_params)
    {
        response resp;
        if (!impl_->client_impl) {
            resp.error = "Client is not initialized. Call set_server() first.";
            return resp;
        }

        std::string req_path = impl::build_request_path(path_, std::string());
        auto params = impl::to_httplib_params(q_params);
        if (!params.empty()) {
            req_path = ::httplib::append_query_params(req_path, params);
        }

        auto h = impl::to_httplib_headers(headers_);
        auto result = impl_->client_impl->Options(req_path, h);

        if (!result) {
            resp.error = ::httplib::to_string(result.error());
            resp.error_code = static_cast<int>(result.error());
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

    options_client::result_code options_client::options(const query_params& q_params, response& out_resp) noexcept
    {
        out_resp = options(q_params);
        return classify(out_resp);
    }

}
