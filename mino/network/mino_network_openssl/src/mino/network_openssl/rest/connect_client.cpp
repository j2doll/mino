// connect_client.cpp
#include <sstream>
#include <algorithm>
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
