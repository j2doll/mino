//-----------------------------------------------
// Execute broker before running server and client:
//  1. Start broker ( network / message_broker / broker )
//  2. Start server ( network / rpc / server )
//  3. Start client ( network / rpc / client )
//-----------------------------------------------

#include <iostream>
#include <stdexcept>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"
#include "mino/core/json/json.hpp"

// network rpc server
#include "mino/network/ethernet.hpp"
#include "mino/network/rpc/rpc_server_base.hpp"

// RPC 연동을 위한 공통 자료
#include "../rpc_example_common.hpp" 

// custom business log class 
class my_business_server : public mino::network::rpc::rpc_server_base {
public:
    using req_t = my_app::domain::task_request;
    using res_t = my_app::domain::task_response;

    void initialize_services() {
        register_raw_service(
            std::string(my_app::domain::service_name),
            [this](const json& raw_arg) -> json
            {
                try {
                    req_t req = parse_request(raw_arg);
                    res_t res = execute_analysis_logic(req);
                    return to_json(res);
                }
                catch (const std::invalid_argument& ex) {
                    res_t res;
                    res.is_success = false;
                    res.message = std::string("Invalid request: ") + ex.what();
                    res.details["node_id"] = -1;
                    return to_json(res);
                }
                catch (const std::exception& ex) {
                    res_t res;
                    res.is_success = false;
                    res.message = std::string("Internal error: ") + ex.what();
                    res.details["node_id"] = -2;
                    return to_json(res);
                }

                res_t res;
                res.is_success = false;
                res.message = "Unknown error occurred during request processing.";
                res.details["node_id"] = -3;
                return to_json(res);
            });
    }

private:
    // mino::core::json::value -> task_request DTO 변환
    req_t parse_request(const json& val) {
        if (!val.is_object()) {
            throw std::invalid_argument("Request payload is not a JSON object");
        }

        req_t req;
        if (val.has_path("command")) {
            req.command = const_cast<json&>(val)["command"].get_string();
        }
        else {
            throw std::invalid_argument("Missing required field: 'command'");
        }

        if (val.has_path("count")) {
            req.count = static_cast<int>(const_cast<json&>(val)["count"].get_number());
        }

        if (val.has_path("parameters")) {
            auto& params_val = const_cast<json&>(val)["parameters"];
            if (params_val.is_array()) {
                const auto& arr = std::get<mino::core::json::array_t>(params_val.data);
                for (const auto& item : arr) {
                    req.parameters.push_back(item.get_number());
                }
            }
        }
        return req;
    }

    // task_response DTO -> mino::core::json::value 변환
    json to_json(const res_t& res) {
        json j;
        j["is_success"] = res.is_success;
        j["message"] = res.message;
        j["details"] = res.details;
        return j;
    }

    // 실제 분석 로직
    res_t execute_analysis_logic(const req_t& req) {
        res_t res;
        if (req.command == "analyze") {
            res.is_success = true;
            res.message = "Analysis node response success.";
            res.details["node_id"] = 1;
        }
        else {
            res.is_success = false;
            res.message = "Unknown command. Analysis failed.";
            res.details["node_id"] = -1;
        }
        return res;
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    my_business_server server;

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto rpc_server_console_sink = std::make_shared<mclt::console_sink>("rpc_server_console");
    auto rpc_server_logger = std::make_shared<mclt::logger>("broker_logger");
    rpc_server_logger->add_sink(rpc_server_console_sink);
    rpc_server_logger->set_level(mclt::log_level::debug);
    mclt::logger::register_logger(rpc_server_logger);

    server.setup_logger(rpc_server_logger);

    std::string broker_ip = "127.0.0.1";
    unsigned short broker_port = 54321;
    server.set_broker(broker_ip, broker_port);
    server.set_startup_timeout(std::chrono::milliseconds(3000));
    server.initialize_services();

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!server.start(tcp_sleep_time)) {
        rpc_server_logger->error("<bright_yellow>Failed</bright_yellow> to start RPC Server. "
            "Check broker connection and configurations.");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    rpc_server_logger->info("RPC Server started. <bright_yellow>Press Enter to stop...</bright_yellow>");
    std::cin.get();

    server.stop();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
