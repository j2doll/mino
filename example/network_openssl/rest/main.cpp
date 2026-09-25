#include <iostream>
#include <string>
#include <vector>

#include "mino/core/string/string.hpp"
#include "mino/network/ethernet.hpp"
#include "mino/network_openssl/rest/rest.hpp"

namespace rest_ns = mino::network_openssl::rest;

using get_client = rest_ns::get_client;
using post_client = rest_ns::post_client;
using put_client = rest_ns::put_client;
using patch_client = rest_ns::patch_client;
using delete_client = rest_ns::delete_client;
using head_client = rest_ns::head_client;
using options_client = rest_ns::options_client;
using trace_client = rest_ns::trace_client;
using connect_client = rest_ns::connect_client;

void test_get_no_except();
void test_get_except();
void test_post_no_except();
void test_post_except();
void test_put_no_except();
void test_put_except();
void test_patch_no_except();
void test_patch_except();
void test_delete_no_except();
void test_delete_except();
void test_head_no_except();
void test_head_except();
void test_options_no_except();
void test_options_except();
void test_trace_no_except();
void test_trace_except();
void test_connect_no_except();
void test_connect_except();

void print_response_body(
    const std::vector<std::string>& headers,
    const std::string& content_type,
    const std::string& body);

// rest_types.hpp의 to_string(result_code)을 호출하여 출력
void print_result_code(rest_ns::result_code rc)
{
    std::cout << "ResultCode = " << static_cast<int>(rc) << "\n";
    std::cout << rest_ns::to_string(rc) << "\n";
}

// rest_types.hpp의 to_string(http_status)을 호출하여 출력
void print_http_status(rest_ns::http_status status, long raw_code)
{
    std::cout << "[EXCEPT] HTTP Status: " << raw_code << "\n";
    std::cout << rest_ns::to_string(status) << "\n";
}

int main(int argc, char* argv[])
{
    mino::network::sock mnsock;

    namespace mcsp = ::mino::core::string::print;
    auto println = [](std::string_view fmt, auto&&... args) {
        mcsp::println(fmt, std::forward<decltype(args)>(args)...);
        };

    println("========================================");
    println("=== GET Request ===");
    test_get_no_except();
    test_get_except();

    println("========================================");
    println("=== POST Request ===");
    test_post_no_except();
    test_post_except();

    println("========================================");
    println("=== PUT Request ===");
    test_put_no_except();
    test_put_except();

    println("========================================");
    println("=== PATCH Request ===");
    test_patch_no_except();
    test_patch_except();

    println("========================================");
    println("=== DELETE Request ===");
    test_delete_no_except();
    test_delete_except();

    println("========================================");
    println("=== HEAD Request ===");
    test_head_no_except();
    test_head_except();

    println("========================================");
    println("=== OPTIONS Request ===");
    test_options_no_except();
    test_options_except();

    println("========================================");
    println("=== TRACE Request ===");
    test_trace_no_except();
    test_trace_except();

    println("========================================");
    println("=== CONNECT Request ===");
    test_connect_no_except();
    test_connect_except();

    return 0;
}

void print_response_body(
    const std::vector<std::string>& headers,
    const std::string& content_type,
    const std::string& body)
{
    std::cout << "[Response Headers]\n";
    for (const auto& h : headers) {
        std::cout << "  " << h << "\n";
    }
    std::cout << "[Body Content-Type: " << content_type << "]\n";
    if (content_type.find("application/json") != std::string::npos) {
        std::cout << "[JSON Body]\n" << body << "\n";
    }
    else if (content_type.find("application/xml") != std::string::npos ||
        content_type.find("text/xml") != std::string::npos) {
        std::cout << "[XML Body]\n" << body << "\n";
    }
    else if (!body.empty()) {
        std::cout << "[Raw Body]\n" << body << "\n";
    }
}

// -----------------------------------------------------------------
// GET: get_server.py (Port: 20011, Path: /get)
// -----------------------------------------------------------------
void test_get_no_except()
{
    std::cout << "\n--- GET (noexcept) ---\n";
    get_client client;
    client.set_server("http", "127.0.0.1", 20011, "/get");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"},
        {"Accept",     "application/json"}
        });

    get_client::response resp;
    auto rc = client.get({ {"query", "cpp-httplib-test"} }, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_get_except()
{
    std::cout << "\n--- GET (except) ---\n";
    get_client client;
    client.set_server("http", "127.0.0.1", 20011, "/get");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"},
        {"Accept",     "application/json"}
        });

    try {
        auto resp = client.get({ {"query", "cpp-httplib-test"} });
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// POST: post_server.py (Port: 20012, Path: /post)
// -----------------------------------------------------------------
void test_post_no_except()
{
    std::cout << "\n--- POST (noexcept) ---\n";
    post_client client;
    client.set_server("http", "127.0.0.1", 20012, "/post");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body = "{\"action\": \"create\", \"id\": 100, \"value\": \"test_post\"}";
    post_client::response resp;
    auto rc = client.post(json_body, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_post_except()
{
    std::cout << "\n--- POST (except) ---\n";
    post_client client;
    client.set_server("http", "127.0.0.1", 20012, "/post");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    try {
        std::string json_body = "{\"action\": \"create\", \"id\": 100, \"value\": \"test_post\"}";
        auto resp = client.post(json_body);
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// PUT: put_server.py (Port: 20013, Path: /resource/1)
// -----------------------------------------------------------------
void test_put_no_except()
{
    std::cout << "\n--- PUT (noexcept) ---\n";
    put_client client;
    client.set_server("http", "127.0.0.1", 20013, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body = "{\"id\": 1, \"name\": \"updated item\", \"version\": 2}";
    put_client::response resp;
    auto rc = client.put(json_body, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_put_except()
{
    std::cout << "\n--- PUT (except) ---\n";
    put_client client;
    client.set_server("http", "127.0.0.1", 20013, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    try {
        std::string json_body = "{\"id\": 1, \"name\": \"updated item\", \"version\": 2}";
        auto resp = client.put(json_body);
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// PATCH: patch_server.py (Port: 20014, Path: /resource/1)
// -----------------------------------------------------------------
void test_patch_no_except()
{
    std::cout << "\n--- PATCH (noexcept) ---\n";
    patch_client client;
    client.set_server("http", "127.0.0.1", 20014, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body = "{\"status\": \"active\", \"updated_by\": \"admin\"}";
    patch_client::response resp;
    auto rc = client.patch(json_body, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_patch_except()
{
    std::cout << "\n--- PATCH (except) ---\n";
    patch_client client;
    client.set_server("http", "127.0.0.1", 20014, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    try {
        std::string json_body = "{\"status\": \"active\", \"updated_by\": \"admin\"}";
        auto resp = client.patch(json_body);
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// DELETE: delete_server.py (Port: 20015, Path: /resource/1)
// -----------------------------------------------------------------
void test_delete_no_except()
{
    std::cout << "\n--- DELETE (noexcept) ---\n";
    delete_client client;
    client.set_server("http", "127.0.0.1", 20015, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    delete_client::query_params q_params = { {"mode", "cascade"} };
    std::string json_body = "{\"confirm\": true, \"reason\": \"cleanup\"}";

    delete_client::response resp;
    auto rc = client.del(q_params, json_body, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_delete_except()
{
    std::cout << "\n--- DELETE (except) ---\n";
    delete_client client;
    client.set_server("http", "127.0.0.1", 20015, "/resource/1");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",   "RestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    try {
        delete_client::query_params q_params = { {"mode", "cascade"} };
        std::string json_body = "{\"confirm\": true, \"reason\": \"cleanup\"}";

        auto resp = client.del(q_params, json_body);
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// HEAD: head_server.py (Port: 20016, Path: /check)
// -----------------------------------------------------------------
void test_head_no_except()
{
    std::cout << "\n--- HEAD (noexcept) ---\n";
    head_client client;
    client.set_server("http", "127.0.0.1", 20016, "/check");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    head_client::response resp;
    auto rc = client.head({}, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_head_except()
{
    std::cout << "\n--- HEAD (except) ---\n";
    head_client client;
    client.set_server("http", "127.0.0.1", 20016, "/check");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    try {
        auto resp = client.head();
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// OPTIONS: options_server.py (Port: 20017, Path: /api)
// -----------------------------------------------------------------
void test_options_no_except()
{
    std::cout << "\n--- OPTIONS (noexcept) ---\n";
    options_client client;
    client.set_server("http", "127.0.0.1", 20017, "/api");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    options_client::response resp;
    auto rc = client.options({}, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_options_except()
{
    std::cout << "\n--- OPTIONS (except) ---\n";
    options_client client;
    client.set_server("http", "127.0.0.1", 20017, "/api");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    try {
        auto resp = client.options();
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// TRACE: trace_server.py (Port: 20018, Path: /trace)
// -----------------------------------------------------------------
void test_trace_no_except()
{
    std::cout << "\n--- TRACE (noexcept) ---\n";
    trace_client client;
    client.set_server("http", "127.0.0.1", 20018, "/trace");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",    "RestClient/1.0"},
        {"X-Custom-Echo", "TraceVerificationHeader"}
        });

    trace_client::response resp;
    auto rc = client.trace({}, resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_trace_except()
{
    std::cout << "\n--- TRACE (except) ---\n";
    trace_client client;
    client.set_server("http", "127.0.0.1", 20018, "/trace");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent",    "RestClient/1.0"},
        {"X-Custom-Echo", "TraceVerificationHeader"}
        });

    try {
        auto resp = client.trace();
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}

// -----------------------------------------------------------------
// CONNECT: connect_server.py (Port: 20019)
// -----------------------------------------------------------------
void test_connect_no_except()
{
    std::cout << "\n--- CONNECT (noexcept) ---\n";
    connect_client client;
    client.set_server("http", "127.0.0.1", 20019, "");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    connect_client::response resp;
    auto rc = client.connect("127.0.0.1:20019", resp);
    print_result_code(rc);

    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
    }
}

void test_connect_except()
{
    std::cout << "\n--- CONNECT (except) ---\n";
    connect_client client;
    client.set_server("http", "127.0.0.1", 20019, "");
    client.set_timeout_ms(5000);
    client.set_headers({
        {"User-Agent", "RestClient/1.0"}
        });

    try {
        auto resp = client.connect("127.0.0.1:20019");
        print_http_status(resp.status, resp.raw_status_code);
        if (resp.is_success()) {
            print_response_body(resp.headers, resp.content_type, resp.body);
        }
        else {
            std::cout << "[Error] status=" << resp.raw_status_code << ", message=" << resp.error << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "[Exception] " << ex.what() << "\n";
    }
}
