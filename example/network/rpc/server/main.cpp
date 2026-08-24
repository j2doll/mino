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

#include "mino/network/ethernet.hpp"
#include "mino/network/rpc/rpc_server_base.hpp"

// RPC 연동을 위한 공통 자료
#include "../rpc_example_common.hpp" 

// custom business log class 
class my_business_server : public mino::network::rpc::rpc_server_base {
public:
    using req_t = my_app::domain::task_request;  // RPC 요청 구조체
    using res_t = my_app::domain::task_response; // RPC 응답 구조체

    // 서비스 등록 및 핸들러 설정
    void initialize_services() {
        register_raw_service(
            std::string(my_app::domain::service_name), // RPC 서비스 이름: "task_service"
            [this](const json& raw_arg) -> json 
            {
                try {
                    req_t req = parse_request(raw_arg); // json -> task_request 변환
                    res_t res = execute_analysis_logic(req); // 실제 RPC 로직 수행
                    return to_json(res); // task_response -> json 변환
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
    // json -> task_request (RPC 요청 구조체) 변환
    req_t parse_request(const json& val) {
        if (!val.is_object()) {
            throw std::invalid_argument("Request payload is not a JSON object");
        }

        req_t req;

        // task_request 의 필드 "command" 처리
        if (val.has_path("command")) {
            req.command = const_cast<json&>(val)["command"].get_string();
        }
        else {
            throw std::invalid_argument("Missing required field: 'command'");
        }

        // task_request 의 필드 "count" 처리
        if (val.has_path("count")) {
            req.count = static_cast<int>(const_cast<json&>(val)["count"].get_number());
        }

        // task_request 의 필드 "parameters" 처리
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

    // task_response (RPC 응답 구조체) -> json 변환
    json to_json(const res_t& res) {
        json j;
        j["is_success"] = res.is_success;
        j["message"] = res.message;
        j["details"] = res.details;
        return j;
    }

    // 실제 RPC 로직
    res_t execute_analysis_logic(const req_t& req) {
        res_t res;
        if (req.command == "analyze") {
            res.is_success = true;
            res.message = "Analysis node response success.";
            res.details["node_id"] = 1;
            return res; 
        }
        res.is_success = false;
        res.message = "Unknown command. Analysis failed.";
        res.details["node_id"] = -1;

        return res;
    }
};

// main 을 구동하기 전에 message_broker/broker 를 먼저 구동해야 합니다.
int main(int argc, char* argv[]) {
    mino::network::sock mnsock;

    // RPC 서버 인스턴스 생성
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
    if (!server.start(tcp_sleep_time)) { // RPC 서버 시작 시도
        rpc_server_logger->error(
            "<bright_yellow>Failed</bright_yellow> to start RPC Server. "
            "Check broker connection and configurations.");
        return -1;
    }

    rpc_server_logger->info("RPC Server started. "
        "<bright_yellow>Press Enter to stop...</bright_yellow>");
    std::cin.get(); // 키 입력 대기
    // NOTE: 키 입력 댁 ㅣ대신 loop를 이용하여 서버 구성도 가능.

    server.stop(); // RPC 서버 중지

    return 0;
}
