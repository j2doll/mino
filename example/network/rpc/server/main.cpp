//-----------------------------------------------
// Execute broker before running server and client:
//  1. Start broker ( network / message_broker / broker )
//  2. Start server ( network / rpc / server )
//  3. Start client ( network / rpc / client )
//-----------------------------------------------

#include <iostream>

#include "mino/core/string/string.hpp"
#include "mino/core/log/tinylog/logger.hpp"

// network rpc server
#include "mino/network/ethernet.hpp"
#include "mino/network/rpc/rpc_server_base.hpp"

// RPC 연동을 위한 공통 자료
#include "../rpc_example_common.hpp" 

// custom business log class 
class my_business_server : public mino::network::rpc::rpc_server_base {
public:
    // rpc_example_common.hpp 에서 정의된 요청(Request)과 응답(Response) 타입을 사용
    using req_t = my_app::domain::task_request;
    using res_t = my_app::domain::task_response;

    void initialize_services() { // 서비스 초기화
        register_raw_service( // 서비스 등록
            my_app::domain::service_name, // 서비스 이름
            [this](const nlohmann::json& raw_arg) -> nlohmann::json // 핸들러 함수
            {
                try {
                    auto req = raw_arg.get<req_t>();
                    res_t res = execute_analysis_logic(req);
                    return nlohmann::json(res);
                }
                catch (const nlohmann::json::exception& ex) {
                    res_t res;
                    res.is_success = false;
                    res.message = std::string("Invalid request: ") + ex.what();
                    res.details = { {"node_id", -1} };
                    return nlohmann::json(res);
                }
                catch (const std::exception& ex) {
                    res_t res;
                    res.is_success = false;
                    res.message = std::string("Internal error: ") + ex.what();
                    res.details = { {"node_id", -2} };
                    return nlohmann::json(res);
                }

                res_t res;
                res.is_success = false;
                res.message = "Unknown error occurred during request processing.";
                res.details = { {"node_id", -3} };
                return nlohmann::json(res);
            });
    }

private:
    // 실제 분석 로직을 수행하는 함수 (여기서는 단순히 요청을 확인하고 응답을 생성)
    res_t execute_analysis_logic(const req_t& req) {
        res_t res;
        if (req.command == "analyze") {
            res.is_success = true;
            res.message = "Analysis node response success.";
            res.details = { {"node_id", 1} };
        }
        else {
            res.is_success = false;
            res.message = "Unknown command. Analysis failed.";
            res.details = { {"node_id", -1} };
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

    my_business_server server; // RPC 서버 객체 생성

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto rpc_server_console_sink = std::make_shared<mclt::console_sink>("rpc_server_console");
    auto rpc_server_logger = std::make_shared<mclt::logger>("broker_logger");
    rpc_server_logger->add_sink(rpc_server_console_sink);
    rpc_server_logger->set_level(mclt::log_level::debug);
    mclt::logger::register_logger(rpc_server_logger);

    server.setup_logger(rpc_server_logger); // 로거 설정

    std::string broker_ip = "127.0.0.1";
    unsigned short broker_port = 54321;
    server.set_broker(broker_ip, broker_port); // 브로커 설정
    server.set_startup_timeout(std::chrono::milliseconds(3000)); // 시작 타임아웃 설정
    server.initialize_services(); // 서비스 초기화

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    if (!server.start(tcp_sleep_time)) { // RPC 서버 시작
        // NOTE: broker 와 연결 실패 시, 연결 재시도를 해도 됨.
        rpc_server_logger->error("<bright_yellow>Failed</bright_yellow> to start RPC Server. "
            "Check broker connection and configurations.");
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    rpc_server_logger->info("RPC Server started. <bright_yellow>Press Enter to stop...</bright_yellow>");
    std::cin.get(); // Wait for user input to stop the server

    server.stop(); // RPC 서버 중지

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
