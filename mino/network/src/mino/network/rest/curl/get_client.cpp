#ifdef USE_CURL

#include <sstream>
#include <algorithm>
#include <curl/curl.h>

#include "mino/network/rest/curl/get_client.hpp"

namespace {
    std::once_flag g_curl_global_init_flag;
}

namespace mino::network::rest::curl {

void get_client::global_init()
{
    std::call_once(g_curl_global_init_flag, []() {
        CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (code != CURLE_OK) {
            throw std::runtime_error("curl_global_init failed");
        }
        });
}

size_t get_client::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t real_size = size * nmemb;
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, real_size);
    return real_size;
}

size_t get_client::header_callback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    size_t real_size = size * nitems;
    auto* resp = static_cast<response*>(userdata);
    std::string header_line(buffer, real_size);

    // Remove trailing CRLF
    header_line.erase(std::remove(header_line.end() - 2, header_line.end(), '\r'), header_line.end());
    header_line.erase(std::remove(header_line.end() - 1, header_line.end(), '\n'), header_line.end());

    // Store header line if not empty
    if (!header_line.empty() && header_line.find(':') != std::string::npos) {
        resp->headers.push_back(header_line);

        // Optionally extract Content-Type
        const std::string key = "Content-Type:";
        if (header_line.size() > key.size() &&
            std::equal(key.begin(), key.end(), header_line.begin(),
                [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            // Extract value after "Content-Type:"
            std::string value = header_line.substr(key.size());
            // Trim leading whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            resp->content_type = value;
        }
    }
    return real_size;
}

get_client::get_client()
{
    global_init();

    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("curl_easy_init failed");
    }
}

get_client::~get_client()
{
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

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
}

void get_client::set_headers(const headers& headers)
{
    headers_ = headers;
}

void get_client::set_timeout_ms(long timeout_ms)
{
    if (timeout_ms <= 0) throw std::invalid_argument("timeoutMs must be greater than 0.");
    timeout_ms_ = timeout_ms;
}

void get_client::set_ignore_ssl_errors(bool ignore)
{
    ignore_ssl_errors_ = ignore;
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

std::string get_client::build_url(const query_params& query_params)
{
    if (host_.empty())
        throw std::runtime_error("setServer() must be called first.");

    std::ostringstream oss;

    oss << scheme_ << "://" << host_;

    if (port_ > 0)
        oss << ":" << port_;

    if (!path_.empty()) {
        if (path_.front() != '/')
            oss << "/";
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
                throw std::runtime_error("URL encoding failed");
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

get_client::response get_client::get(const query_params& query_params)
{
    if (!curl_)
        throw std::runtime_error("CURL handle is not initialized.");

    response resp;
    std::string body;

    std::string url = build_url(query_params);

    auto header_lines = build_header_lines(headers_);
    struct curl_slist* header_list = nullptr;
    for (const auto& line : header_lines)
        header_list = curl_slist_append(header_list, line.c_str());

    curl_easy_reset(curl_);

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_ms_);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &body);

    // Set header callback to capture response headers
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &resp);

    if (header_list)
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);

    // ★ SSL certificate verification ON/OFF
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

get_client::result_code get_client::classify(const response& r)
{
    if (!r.error.empty()) {
        if (r.curl_error_code == CURLE_OPERATION_TIMEDOUT)
            return result_code::curl_timeout;

        if (r.curl_error_code == CURLE_PEER_FAILED_VERIFICATION ||
            r.curl_error_code == CURLE_SSL_CONNECT_ERROR)
            return result_code::curl_ssl_error;

        if (r.curl_error_code == CURLE_COULDNT_RESOLVE_HOST ||
            r.curl_error_code == CURLE_COULDNT_CONNECT)
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

get_client::result_code
    get_client::get(const query_params& query_params, response& out_resp) noexcept
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


}  

#endif // USE_CURL
