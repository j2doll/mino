#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/network/ethernet.hpp"

#include "mino/network_curl/rest/rest.hpp"

namespace {
    namespace mcs = mino::core::string;
    namespace mcsp = mino::core::string::print;

    auto tce = mcs::to_console_encoding;
    auto tcev = [](std::string_view sv) { return mcs::to_console_encoding(std::string(sv)); };

    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    auto println = [](std::string_view fmt, auto&&... args) { mcsp::println(fmt, std::forward<decltype(args)>(args)...); };
    auto eprintln = [](std::string_view fmt, auto&&... args) { mcsp::eprintln(fmt, std::forward<decltype(args)>(args)...); };
}

namespace rest_namespace = mino::network_curl::rest;

using get_client = rest_namespace::get_client;
using post_client = rest_namespace::post_client;
using put_client = rest_namespace::put_client;
using patch_client = rest_namespace::patch_client;
using delete_client = rest_namespace::delete_client;
using head_client = rest_namespace::head_client;
using options_client = rest_namespace::options_client;
using trace_client = rest_namespace::trace_client;
using connect_client = rest_namespace::connect_client;

void test_get();
void test_post();
void test_put();
void test_patch();
void test_delete();
void test_head();
void test_options();
void test_trace();
void test_connect();

int main(int argc, char* argv[])
{
    mino::network::sock mnsock;

    println("==============================");
    println("=== 1. GET Request (20011) ===");
    test_get();

    println("\n==============================");
    println("=== 2. POST Request (20012) ===");
    test_post();

    println("\n==============================");
    println("=== 3. PUT Request (20013) ===");
    test_put();

    println("\n==============================");
    println("=== 4. PATCH Request (20014) ===");
    test_patch();

    println("\n==============================");
    println("=== 5. DELETE Request (20015) ===");
    test_delete();

    println("\n==============================");
    println("=== 6. HEAD Request (20016) ===");
    test_head();

    println("\n==============================");
    println("=== 7. OPTIONS Request (20017) ===");
    test_options();

    println("\n==============================");
    println("=== 8. TRACE Request (20018) ===");
    test_trace();

    println("\n==============================");
    println("=== 9. CONNECT Request (20019) ===");
    test_connect();

    return 0;
}

// ---------------------------------------------------
// 공통 응답 출력 함수
// ---------------------------------------------------
void print_response_body(
    const std::vector<std::string>& headers,
    const std::string& content_type,
    const std::string& body)
{
    print("[Response Headers]");
    for (const auto& h : headers) {
        print(h);
    }
    print("[Body Content-Type: ", content_type, "]");
    if (content_type.find("application/json") != std::string::npos) {
        print("[JSON Body]\n", body);
    }
    else if (content_type.find("application/xml") != std::string::npos ||
        content_type.find("text/xml") != std::string::npos) {
        print("[XML Body]\n", body);
    }
    else {
        print("[Raw Body]\n", body);
    }
}

// ---------------------------------------------------
// 1. GET 예제 (Port: 20011)
// ---------------------------------------------------
void test_get()
{
    get_client client;
    assert(client.set_server("http", "127.0.0.1", 20011, "/get"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"},
        {"Accept",     "application/json"}
        });

    // GET 요청 시 Query Parameter를 포함하는 방법
    get_client::query_params params = { {"query", "test"} };
    get_client::response resp;
    auto rc = client.get(params, resp);

    // Body에 JSON 데이터를 포함하여 GET 요청하는 방법
    // std::string json_body = "{\"filter\": {\"keyword\": \"libcurl\", \"limit\": 10}}";
    // 
    // 1. noexcept 버전 호출
    // get_client::response resp;
    // auto rc = client.get({}, json_body, resp);
    // 
    // 2. 예외 버전 호출
    // get_client::response resp = client.get({}, json_body);
    // 

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 2. POST 예제 (Port: 20012)
// ---------------------------------------------------
void test_post()
{
    post_client client;
    assert(client.set_server("http", "127.0.0.1", 20012, "/post"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent",   "CurlRestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body = "{\"message\": \"hello from POST\", \"value\": 100}";
    post_client::response resp;
    auto rc = client.post(json_body, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 3. PUT 예제 (Port: 20013)
// ---------------------------------------------------
void test_put()
{
    put_client client;
    assert(client.set_server("http", "127.0.0.1", 20013, "/resource/1"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent",   "CurlRestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_body = "{\"id\": 1, \"name\": \"updated item\"}";
    put_client::response resp;
    auto rc = client.put(json_body, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 4. PATCH 예제 (Port: 20014)
// ---------------------------------------------------
void test_patch()
{
    patch_client client;
    assert(client.set_server("http", "127.0.0.1", 20014, "/resource/1"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent",   "CurlRestClient/1.0"},
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"}
        });

    std::string json_patch = "{\"name\": \"partially updated item\"}";
    patch_client::response resp;
    auto rc = client.patch(json_patch, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 5. DELETE 예제 (Port: 20015)
// ---------------------------------------------------
void test_delete()
{
    delete_client client;
    assert(client.set_server("http", "127.0.0.1", 20015, "/resource/1"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"},
        {"Accept",     "application/json"}
        });

    delete_client::query_params params = { {"cascade", "true"} };
    delete_client::response resp;
    auto rc = client.del(params, /*body=*/"", resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 6. HEAD 예제 (Port: 20016)
// ---------------------------------------------------
void test_head()
{
    head_client client;
    assert(client.set_server("http", "127.0.0.1", 20016, "/check"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"}
        });

    head_client::response resp;
    auto rc = client.head({}, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print("[HEAD Response Headers]");
        for (const auto& h : resp.headers) {
            print(h);
        }
        print("[Content-Type: ", resp.content_type, "]");
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 7. OPTIONS 예제 (Port: 20017)
// ---------------------------------------------------
void test_options()
{
    options_client client;
    assert(client.set_server("http", "127.0.0.1", 20017, "/api"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"}
        });

    options_client::response resp;
    auto rc = client.options({}, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 8. TRACE 예제 (Port: 20018)
// ---------------------------------------------------
void test_trace()
{
    trace_client client;
    assert(client.set_server("http", "127.0.0.1", 20018, "/trace"));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent",    "CurlRestClient/1.0"},
        {"X-Custom-Echo", "TestingTrace"}
        });

    trace_client::response resp;
    auto rc = client.trace({}, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

// ---------------------------------------------------
// 9. CONNECT 예제 (Port: 20019)
// ---------------------------------------------------
void test_connect()
{
    connect_client client;
    assert(client.set_server("http", "127.0.0.1", 20019, ""));
    assert(client.set_timeout_ms(5000));
    client.set_headers({
        {"User-Agent", "CurlRestClient/1.0"}
        });

    connect_client::response resp;
    auto rc = client.connect({}, resp);

    print("ResultCode = ", static_cast<int>(rc));
    if (resp.is_success()) {
        print_response_body(resp.headers, resp.content_type, resp.body);
    }
    else {
        eprint("[Error] status=", resp.raw_status_code, ", message=", resp.error);
    }
}

