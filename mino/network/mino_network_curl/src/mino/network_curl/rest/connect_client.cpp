#ifdef USE_CURL

#include <sstream>
#include <algorithm>
#include <curl/curl.h>

#include "mino/network_curl/rest/util.hpp" 
#include "mino/network_curl/rest/connect_client.hpp"

namespace {
    std::once_flag g_curl_global_init_flag;
}

namespace mino::network_curl::rest {

    void connect_client::global_init()
    {
        std::call_once(g_curl_global_init_flag, []() {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            });
    }

    size_t connect_client::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        size_t real_size = size * nmemb;
        auto* body = static_cast<std::string*>(userdata);
        body->append(ptr, real_size);
        return real_size;
    }

    size_t connect_client::header_callback(char* buffer, size_t size, size_t nitems, void* userdata)
    {
        size_t real_size = size * nitems;
        auto* resp = static_cast<response*>(userdata);
        std::string header_line(buffer, real_size);

        if (header_line.size() >= 2) {
            header_line.erase(std::remove(header_line.end() - 2, header_line.end(), '\r'), header_line.end());
        }
        if (header_line.size() >= 1) {
            header_line.erase(std::remove(header_line.end() - 1, header_line.end(), '\n'), header_line.end());
        }

        if (!header_line.empty() && header_line.find(':') != std::string::npos) {
            resp->headers.push_back(header_line);

            const std::string key = "Content-Type:";
            if (header_line.size() > key.size() &&
                std::equal(key.begin(), key.end(), header_line.begin(),
                    [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                std::string value = header_line.substr(key.size());
                value.erase(0, value.find_first_not_of(" \t"));
                resp->content_type = value;
            }
        }
        return real_size;
    }

    connect_client::connect_client()
    {
        global_init();
        curl_ = curl_easy_init();
    }

    connect_client::~connect_client()
    {
        if (curl_) {
            curl_easy_cleanup(curl_);
            curl_ = nullptr;
        }
    }

    bool connect_client::set_server(
        const std::string& scheme,
        const std::string& host,
        long port,
        const std::string& path)
    {
        // 호스트 및 포트 유효 범위 기본 검증
        if (host.empty() || port < 0 || port > 65535) {
            return false;
        }

        // (1) scheme 검증: 대소문자 구분 없이 "http" 또는 "https"만 허용
        std::string lower_scheme = scheme;
        std::transform(lower_scheme.begin(), lower_scheme.end(), lower_scheme.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lower_scheme != "http" && lower_scheme != "https") {
            return false;
        }

        // (2) path 검증: 한글, 공백, URL 비허용 문자 차단
        if (!path.empty() && !is_valid_url_path(path)) {
            return false;
        }

        // 모든 검증을 통과한 경우에만 멤버 변수에 저장
        scheme_ = std::move(lower_scheme);
        host_ = host;
        port_ = port;
        path_ = path;

        return true;
    }

    void connect_client::set_headers(const headers& headers)
    {
        headers_ = headers;
    }

    bool connect_client::set_timeout_ms(long timeout_ms)
    {
        if (timeout_ms <= 0)
            return false;
        timeout_ms_ = timeout_ms;
        return true;
    }

    void connect_client::set_ignore_ssl_errors(bool ignore)
    {
        ignore_ssl_errors_ = ignore;
    }

    std::vector<std::string> connect_client::build_header_lines(const headers& headers)
    {
        std::vector<std::string> lines;
        lines.reserve(headers.size());
        for (const auto& kv : headers) {
            if (!kv.first.empty())
                lines.push_back(kv.first + ": " + kv.second);
        }
        return lines;
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

    std::string connect_client::build_url(const query_params& query_params)
    {
        if (host_.empty()) return "";

        std::ostringstream oss;
        oss << scheme_ << "://" << host_;
        if (port_ > 0) oss << ":" << port_;
        if (!path_.empty()) {
            if (path_.front() != '/') oss << "/";
            oss << path_;
        }

        if (!query_params.empty()) {
            oss << "?";
            bool first = true;
            for (const auto& kv : query_params) {
                char* k = curl_easy_escape(curl_, kv.first.c_str(), static_cast<int>(kv.first.size()));
                char* v = curl_easy_escape(curl_, kv.second.c_str(), static_cast<int>(kv.second.size()));
                if (!k || !v) {
                    if (k) curl_free(k);
                    if (v) curl_free(v);
                    return "";
                }
                if (!first) oss << "&";
                first = false;
                oss << k << "=" << v;
                curl_free(k);
                curl_free(v);
            }
        }
        return oss.str();
    }

    connect_client::response connect_client::connect(const query_params& query_params)
    {
        response resp;
        if (!curl_) {
            resp.error = "CURL handle is not initialized.";
            return resp;
        }

        std::string body;
        std::string url = build_url(query_params);
        if (url.empty()) {
            resp.error = "Failed to build URL (host is empty or URL encoding failed).";
            return resp;
        }

        auto header_lines = build_header_lines(headers_);
        struct curl_slist* header_list = nullptr;
        for (const auto& line : header_lines)
            header_list = curl_slist_append(header_list, line.c_str());

        curl_easy_reset(curl_);

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "CONNECT");
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_ms_);
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &body);

        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &resp);

        if (header_list)
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);

        if (ignore_ssl_errors_) {
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else {
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        }

        CURLcode code = curl_easy_perform(curl_);
        if (header_list) curl_slist_free_all(header_list);

        if (code != CURLE_OK) {
            resp.error = curl_easy_strerror(code);
            resp.curl_error_code = static_cast<int>(code);
            return resp;
        }

        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        resp.raw_status_code = http_code;
        resp.status = to_http_status(http_code);
        resp.body = std::move(body);

        return resp;
    }

    connect_client::result_code connect_client::classify(const response& r)
    {
        if (!r.error.empty()) {
            if (r.curl_error_code == CURLE_OPERATION_TIMEDOUT) return result_code::curl_timeout;
            if (r.curl_error_code == CURLE_PEER_FAILED_VERIFICATION || r.curl_error_code == CURLE_SSL_CONNECT_ERROR)
                return result_code::curl_ssl_error;
            if (r.curl_error_code == CURLE_COULDNT_RESOLVE_HOST || r.curl_error_code == CURLE_COULDNT_CONNECT)
                return result_code::curl_network_error;
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

    connect_client::result_code
        connect_client::connect(const query_params& query_params, response& out_resp) noexcept
    {
        out_resp = connect(query_params);
        return classify(out_resp);
    }

}

#endif // USE_CURL
