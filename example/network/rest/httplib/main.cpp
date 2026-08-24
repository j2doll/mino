#include <iostream>
#include <string>
#include <vector>

#include "mino/core/string/string.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/rest/httplib/get_client.hpp"
#include "mino/network/rest/httplib/post_client.hpp"

namespace rest_namespace = mino::network::rest::httplib; 

using get_client = rest_namespace::get_client;
void test_get_no_except();
void test_get_except();

using post_client = rest_namespace::post_client;
void test_post_no_except();
void test_post_except();

void print_response_body(
    const std::vector<std::string>& headers,
    const std::string& content_type,
    const std::string& body);

// NOTE:
//  main을 구동하기 전에 다음을 수행한다.
//    python get_server.py
//    python post_server.py
int main(int argc, char* argv[])
{
    mino::network::sock mnsock;

    namespace mcsp = ::mino::core::string::print;
    auto print = [](std::string_view fmt, auto&&... args) {
        mcsp::print(fmt, std::forward<decltype(args)>(args)...);
        };
    auto println = [](std::string_view fmt, auto&&... args) {
        mcsp::println(fmt, std::forward<decltype(args)>(args)...);
        };
    // println("TEST");
    // mcsp::println("TEST");

    println("==============================");
    println("=== GET Request (noexcept) ===");
    test_get_no_except(); // REST API GET 요청 (noexcept 버전)

    println("==============================");
    println("=== GET Request (exception) ===");
    test_get_except(); // REST API GET 요청 (exception 버전)

    println("==============================");
    println("=== POST Request (noexcept) ===");
    test_post_no_except(); // REST API POST 요청 (noexcept 버전)

    println("==============================");
    println("=== POST Request (exception) ===");
    test_post_except(); // REST API POST 요청 (exception 버전)

    return 0;
}

//----------------------------------------------------------------
void print_response_body(
    const std::vector<std::string>& headers,
    const std::string& content_type,
    const std::string& body)
{
    std::cout << "[Response Headers]\n";
    for (const auto& h : headers) {
        std::cout << h << "\n";
    }
    std::cout << "[Body Content-Type: " << content_type << "]\n";
    if (content_type.find("application/json") != std::string::npos) {
        std::cout << "[JSON Body]\n" << body << "\n";
    }
    else if (content_type.find("application/xml") != std::string::npos ||
        content_type.find("text/xml") != std::string::npos) {
        std::cout << "[XML Body]\n" << body << "\n";
    }
    else {
        std::cout << "[Raw Body]\n" << body << "\n";
    }
}

void test_get_no_except()
{
    get_client client;

    // Modified to match the Python server address
    client.set_server("http", "127.0.0.1", 50011, "/get");
    client.set_timeout_ms(5000);

    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"},
        {"Accept",     "application/json"}
        });

    // Ignore SSL certificate errors if needed
    // client.set_ignore_ssl_errors(true);

    get_client::query_params params = {
        {"query", "test"}
    };

    get_client::response resp;
    auto rc = client.get(params, resp); // noexcept version

    std::cout << "ResultCode = " << static_cast<int>(rc) << "\n";

    using result_code = get_client::result_code;
    switch (rc) {
    case result_code::ok:
        std::cout << "[OK] Request succeeded.\n";
        break;
    case result_code::curl_timeout:
        std::cout << "[CURL TIMEOUT] The request timed out.\n";
        break;
    case result_code::curl_ssl_error:
        std::cout << "[CURL SSL ERROR] SSL certificate error.\n";
        break;
    case result_code::curl_network_error:
        std::cout << "[CURL NETWORK ERROR] Network error (host not found or connection failed).\n";
        break;
    case result_code::curl_other_error:
        std::cout << "[CURL OTHER ERROR] Other CURL error.\n";
        break;
    case result_code::http_client_error_4xx:
        std::cout << "[HTTP 4xx] Client error (4xx).\n";
        break;
    case result_code::http_not_found:
        std::cout << "[HTTP 404] Not found.\n";
        break;
    case result_code::http_server_error_5xx:
        std::cout << "[HTTP 5xx] Server error (5xx).\n";
        break;
    case result_code::http_redirect_3xx:
        std::cout << "[HTTP 3xx] Redirect (3xx).\n";
        break;
    case result_code::http_other_error:
        std::cout << "[HTTP OTHER ERROR] Other HTTP error.\n";
        break;
    case result_code::unknown_error:
        std::cout << "[UNKNOWN ERROR] Unknown error occurred.\n";
        break;
    default:
        std::cout << "[UNEXPECTED] Unhandled result code.\n";
        break;
    }

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code
            << ", message=" << resp.error << "\n";
        // [Error] status=0, message=Could not connect to server 
    }
}

void test_get_except()
{
    get_client client;

    // Modified to match the Python server address
    client.set_server("http", "127.0.0.1", 50011, "/get");
    client.set_timeout_ms(5000);

    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"},
        {"Accept",     "application/json"}
        });

    // Ignore SSL certificate errors if needed
    // client.set_ignore_ssl_errors(true);

    get_client::query_params params = {
        {"query", "test"}
    };

    try {
        // This version may throw exceptions
        get_client::response resp = client.get(params);

        using http_status = get_client::http_status;
        std::cout << "[EXCEPT] Request succeeded.\n";
        std::cout << "HTTP " << resp.raw_status_code << "\n";

        switch (resp.status) {
        case http_status::ok:
            std::cout << "[HTTP 200 OK] Success.\n";
            break;
        case http_status::created:
            std::cout << "[HTTP 201 Created] Resource created.\n";
            break;
        case http_status::no_content:
            std::cout << "[HTTP 204 No Content] Success, no content.\n";
            break;
        case http_status::bad_request:
            std::cout << "[HTTP 400 Bad Request] Client error.\n";
            break;
        case http_status::unauthorized:
            std::cout << "[HTTP 401 Unauthorized] Authentication required.\n";
            break;
        case http_status::forbidden:
            std::cout << "[HTTP 403 Forbidden] Access denied.\n";
            break;
        case http_status::not_found:
            std::cout << "[HTTP 404 Not Found] Resource not found.\n";
            break;
        case http_status::internal_server_error:
            std::cout << "[HTTP 500 Internal Server Error] Server error.\n";
            break;
        case http_status::bad_gateway:
            std::cout << "[HTTP 502 Bad Gateway] Bad gateway.\n";
            break;
        case http_status::service_unavailable:
            std::cout << "[HTTP 503 Service Unavailable] Service unavailable.\n";
            break;
        case http_status::unknown:
        default:
            std::cout << "[HTTP UNKNOWN] Unknown HTTP status.\n";
            break;
        }

        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code
                << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[EXCEPT] Exception: " << ex.what() << "\n";
    }
    catch (...) {
        std::cout << "[EXCEPT] Unknown exception occurred.\n";
    }
}



//---------------------------------------------------


// ---------------------------------------------------
// POST example (noexcept version)
//  - Handles all result_code cases
// ---------------------------------------------------
void test_post_no_except()
{
    post_client client;

    // Example: Assume /post endpoint of a Python server
    client.set_server("http", "127.0.0.1", 50012, "/post");
    client.set_timeout_ms(5000);

    client.set_headers({
        {"User-Agent",   "CurlRestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    // JSON data to send
    std::string json_body =
        "{"
        "\"message\": \"hello from POST (escape)\","
        "\"value\": 123"
        "}";

    post_client::response resp;
    auto rc = client.post(json_body, resp); // Use noexcept version

    std::cout << "ResultCode = " << static_cast<int>(rc) << "\n";

    using result_code = post_client::result_code;

    switch (rc) {
    case result_code::ok:
        std::cout << "[OK] POST request succeeded.\n";
        break;
    case result_code::curl_timeout:
        std::cout << "[CURL TIMEOUT] The request timed out.\n";
        break;
    case result_code::curl_ssl_error:
        std::cout << "[CURL SSL ERROR] SSL certificate error occurred.\n";
        break;
    case result_code::curl_network_error:
        std::cout << "[CURL NETWORK ERROR] Network error such as host not found or connection failed.\n";
        break;
    case result_code::curl_other_error:
        std::cout << "[CURL OTHER ERROR] Other CURL error.\n";
        break;
    case result_code::http_client_error_4xx:
        std::cout << "[HTTP 4xx] Client-side error (4xx).\n";
        break;
    case result_code::http_not_found:
        std::cout << "[HTTP 404] Endpoint not found.\n";
        break;
    case result_code::http_server_error_5xx:
        std::cout << "[HTTP 5xx] Server-side error (5xx).\n";
        break;
    case result_code::http_redirect_3xx:
        std::cout << "[HTTP 3xx] Redirect response (3xx).\n";
        break;
    case result_code::http_other_error:
        std::cout << "[HTTP OTHER ERROR] Other HTTP error.\n";
        break;
    case result_code::unknown_error:
        std::cout << "[UNKNOWN ERROR] An unknown error occurred.\n";
        break;
    default:
        std::cout << "[UNEXPECTED] Unhandled result code.\n";
        break;
    }

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code
            << ", message=" << resp.error << "\n";
    }
}

// ---------------------------------------------------
// POST example (exception version)
//  - Handles all http_status cases
// ---------------------------------------------------
void test_post_except()
{
    post_client client;

    // Example: Assume /post endpoint of a Python server
    client.set_server("http", "127.0.0.1", 50012, "/post");
    client.set_timeout_ms(5000);

    client.set_headers({
        {"User-Agent",   "CurlRestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body =
        "{"
        "\"message\": \"hello from POST (except)\","
        "\"value\": 123"
        "}";

    try {
        // POST version that throws exceptions
        post_client::response resp = client.post(json_body);

        using http_status = post_client::http_status;

        std::cout << "[EXCEPT] POST request completed successfully.\n";
        std::cout << "HTTP " << resp.raw_status_code << "\n";

        switch (resp.status) {
        case http_status::ok:
            std::cout << "[HTTP 200 OK] Success.\n";
            break;
        case http_status::created:
            std::cout << "[HTTP 201 Created] Resource created.\n";
            break;
        case http_status::no_content:
            std::cout << "[HTTP 204 No Content] Success but no response body.\n";
            break;
        case http_status::bad_request:
            std::cout << "[HTTP 400 Bad Request] Bad request.\n";
            break;
        case http_status::unauthorized:
            std::cout << "[HTTP 401 Unauthorized] Authentication required.\n";
            break;
        case http_status::forbidden:
            std::cout << "[HTTP 403 Forbidden] Access denied.\n";
            break;
        case http_status::not_found:
            std::cout << "[HTTP 404 Not Found] Resource not found.\n";
            break;
        case http_status::internal_server_error:
            std::cout << "[HTTP 500 Internal Server Error] Internal server error.\n";
            break;
        case http_status::bad_gateway:
            std::cout << "[HTTP 502 Bad Gateway] Gateway error.\n";
            break;
        case http_status::service_unavailable:
            std::cout << "[HTTP 503 Service Unavailable] Service unavailable.\n";
            break;
        case http_status::unknown:
        default:
            std::cout << "[HTTP UNKNOWN] Unknown HTTP status code.\n";
            break;
        }

        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code
                << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[EXCEPT] Exception occurred: " << ex.what() << "\n";
    }
    catch (...) {
        std::cout << "[EXCEPT] An unknown exception occurred.\n";
    }
}





