#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <csignal>
#include <memory>
#include <cstdint>
#include <exception>

#include "mino/core/string/string.hpp"
#include "mino/core/daemon/daemon.hpp"
#include "mino/core/log/tinylog/logger.hpp"

#include "mino/external/json/json.hpp"

#include "mino/network/ethernet.hpp"
#include "mino/network/tcp/tcp.hpp"

// mino core, network, and external tests
int test_mino_core();
int test_mino_network();
int test_mino_external();

int main(int argc, char** argv)
{
    mino::network::sock mnsock; // 소켓 초기화 및 종료를 위한 객체 생성

    namespace mcsp = mino::core::string::print;

    mcsp::print("\n========================\n ... Testing Mino Core");
    if ( test_mino_core() != 0 ) {
        mcsp::print("Mino Core test failed.\n");
    }

    mcsp::print("\n========================\n... Testing Mino External");
    if ( test_mino_external() != 0 ) {
        mcsp::print("Mino External test failed.\n");
    }

    mcsp::print("\n========================\n... Testing Mino Network");
    if ( test_mino_network() != 0 ) {
        mcsp::print("Mino Network test failed.\n");
    }

    return 0;
}

//------------------------------------------------

int test_mino_core() {
    namespace mstring = mino::core::string;
    auto to_console_encoding = mstring::to_console_encoding;

    std::string utf8string = "한글";
    auto result = to_console_encoding(utf8string);
    std::cout << "to_console_encoding() : " << result << "\n";

    return 0;
}

//------------------------------------------------

int test_mino_external() {

    namespace mjson = mino::external::json;
    using json_t = nlohmann::json;
    using json_ptr_t = nlohmann::json::json_pointer; // use project's json_pointer specialization

    // Build a JSON object
    json_t j;
    j["project"] = "mino";
    j["version"] = 1.0;
    j["features"] = { "parsing", "serialization", "tests" };
    j["nested"] = { {"enabled", true}, {"count", 3} };
    j["metadata"] = { {"author", "developer"}, {"license", "MIT"} };

    std::cout << "Constructed JSON:\n" << j.dump(2) << "\n\n";

    // --- Examples using functions from mino/external/json/json.hpp ---

    // 1) Use JSON pointer overloads (njj is an alias to nlohmann::json::json_pointer)
    json_ptr_t ptrEnabled("/nested/enabled");
    bool enabledPtr = mjson::value_or(j, ptrEnabled, false);
    std::cout << "/nested/enabled : " << std::boolalpha << enabledPtr << "\n";

    json_ptr_t ptrCount("/nested/count");
    int countPtr = mjson::value_or<int>(j, ptrCount, 0);
    std::cout << "/nested/count : " << countPtr << "\n\n";

    // 2) Use path-string overloads (path format accepted by the project's helpers)
    bool enabledPath = mjson::value_or_path(j, "/nested/enabled", false);
    double version = mjson::value_or_path(j, "/version", 0.0);
    std::cout << "/nested/enabled : " << std::boolalpha << enabledPath << "\n";
    std::cout << "/version : " << version << "\n\n";

    // 3) Use typed getters with default values
    std::string proj = mjson::get_string(j, "/project", "unknown");
    std::cout << "get_string(/project): " << proj << "\n";

    int count = mjson::get_int(j, "/nested/count", -1);
    std::cout << "get_int(/nested/count): " << count << "\n";

    bool flag = mjson::get_bool(j, "/nested/enabled", false);
    std::cout << "get_bool(/nested/enabled): " << std::boolalpha << flag << "\n";

    double ver = mjson::get_double(j, "/version", 0.0);
    std::cout << "get_double(/version): " << ver << "\n\n";

    // 4) Check existence
    bool hasAuthor = mjson::exists(j, "/metadata/author");
    std::cout << "/metadata/author exists: " << std::boolalpha << hasAuthor << "\n";
    std::string author = mjson::value_or_path(j, "/metadata/author", std::string("n/a"));
    std::cout << "/metadata/author (value_or_path): " << author << "\n\n";

    // 5) Direct node access helper (returns pointer or nullptr)
    const json_t* node = mjson::get_node(j, json_ptr_t("/features/0"));
    if (node) {
        std::cout << "/features/0 : " << node->get<std::string>() << "\n";
    }
    else {
        std::cout << "f/features/0 not found\n";
    }

    return 0;
}

//---------------------------------------------------------

void clean_up_resources(mino::network::tcp::tcp_server* tcp_server)
{
    if (tcp_server) {
        tcp_server->quit();
    }
    std::string msg = "\nResources cleaned up. Exiting.\n";
    std::cout << msg;
}

int test_mino_network() {
    using tcp_server = mino::network::tcp::tcp_server;
    using termination_handler = mino::core::daemon::termination_handler;

    auto& handler = termination_handler::get_instance();
    handler.initialize();

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto console_sink = std::make_shared<mclt::console_sink>("tcp_example_console");
    auto logger = std::make_shared<mclt::logger>("mino_tcp_example");
    logger->add_sink(console_sink);
    logger->set_level(mclt::log_level::info);
    mclt::logger::register_logger(logger);

    tcp_server server;

    handler.set_callback([&server]() {
        clean_up_resources(&server);
        std::exit(0);
    });

    server.set_logger(logger);

    // 콜백 설정 — 소켓 타입은 전역 네임스페이스의 socket_t 사용
    server.set_on_connect_callback([&logger](socket_t s, const std::string& msg) {
        logger->info("Client connected (socket={}): {}", static_cast<uint64_t>(s), msg);
        });

    server.set_on_receive_callback([&server, &logger](socket_t s, const std::string& data) {
        logger->info("Received from {}: {}", static_cast<uint64_t>(s), data);
        // 간단 에코 응답
        std::string reply = "Echo: " + data;
        server.send_to_client(s, reply);
        });

    server.set_on_close_callback([&logger](socket_t s, const std::string& reason) {
        logger->info("Client closed (socket={}): {}", static_cast<uint64_t>(s), reason);
        });

    // 서버 시작 (모든 인터페이스 바인드, 포트 18080)
    auto port_number = 18080;
    auto res = server.start("", port_number);
    if (res != tcp_server::start_result::success) {
        logger->error("Failed to start tcp_server (code={})", static_cast<int>(res));
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    logger->info(
        "<yellow>TCP server</yellow> started on port <green>{0}</green>."
        " Press <bright_cyan>Enter</bright_cyan> to stop,"
        " or <bright_cyan>Ctrl-C</bright_cyan> to quit.",
        port_number);

    // 간단한 종료 대기: Enter 입력 시 종료
    std::cin.get();

    logger->info("<magenta>Stopping</magenta> server...");
    server.quit();
    logger->info("Server <bright_white>stopped</bright_white>.");

    return 0;
}



