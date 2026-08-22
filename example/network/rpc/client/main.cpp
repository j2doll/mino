//-----------------------------------------------
// Execute broker before running server and client:
//  1. Start broker ( network / message_broker / broker )
//  2. Start server ( network / rpc / server )
//  3. Start client ( network / rpc / client )
//-----------------------------------------------

#include <iostream>

#include "mino/core/string/string.hpp"
#include "mino/external/log/spd/auto_color_sink.hpp"

// network rpc client
#include "mino/network/ethernet.hpp"
#include "mino/network/rpc/rpc_client_base.hpp"

// RPC 연동을 위한 공통 자료
#include "../rpc_example_common.hpp"

// custom business log class
class my_business_client : public mino::network::rpc::rpc_client_base {
public:
    // rpc_example_common.hpp 에서 정의된 요청(Request)과 응답(Response) 타입을 사용
    using req_t = my_app::domain::task_request;
    using res_t = my_app::domain::task_response;

    using rpc_status = mino::network::rpc::rpc_status;

    std::pair<rpc_status, res_t> request_analysis(
        const req_t& request,
        std::chrono::seconds timeout = std::chrono::seconds(10))
    {
        using rpc_error_code = mino::network::rpc::rpc_error_code;

        res_t final_result;
        ::nlohmann::json req_json;
        rpc_status status;

        // 요청 직렬화 예외 보호.
        try {
            req_json = request;
        }
        catch (const ::nlohmann::json::exception& ex) {
            final_result.is_success = false;
            final_result.message = std::string("JSON serialization error: ") + ex.what();
            return { { rpc_error_code::invalid_argument, final_result.message }, final_result };
        }
        catch (const std::exception& ex) {
            final_result.is_success = false;
            final_result.message = std::string("Unexpected error during request: ") + ex.what();
            return { { rpc_error_code::internal_server_error, final_result.message }, final_result };
        }
        catch (...) {
            final_result.is_success = false;
            final_result.message = "Unknown error during request serialization/call.";
            return { { rpc_error_code::internal_server_error, final_result.message }, final_result };
        }

        // 서버 RPC의 서비스 호출
        auto rpc_result = call_raw(my_app::domain::service_name, req_json, timeout);
        status = rpc_result.first;

        if (status.ok()) {
            try {
                ::nlohmann::json response_json = rpc_result.second;
                final_result = response_json.get<res_t>();
            }
            catch (const ::nlohmann::json::exception& ex) {
                final_result.is_success = false;
                final_result.message = std::string("[Response] JSON conversion error: ") + ex.what();
            }
            catch (const std::exception& ex) {
                final_result.is_success = false;
                final_result.message = std::string("[Response] Unexpected error: ") + ex.what();
            }
            catch (...) {
                final_result.is_success = false;
                final_result.message = "[Response] Unknown error occurred during response processing.";
            }
        }
        else {
            final_result.is_success = false;
            final_result.message = status.message;
        }

        return { status, final_result };
    }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    using req_t = my_app::domain::task_request;

    using rpc_error_code = mino::network::rpc::rpc_error_code;

    my_business_client client;

    // 로거 설정
    namespace mels = mino::external::log::spd;
    auto rpc_client_console_sink = std::make_shared<mels::auto_color_sink<std::mutex>>();
    std::vector<::spdlog::sink_ptr> sinks{ rpc_client_console_sink };
    auto rpc_client_logger = std::make_shared<::spdlog::logger>("rpc_client_logger", sinks.begin(), sinks.end());
    rpc_client_logger->set_level(::spdlog::level::debug);
    client.set_logger(rpc_client_logger);

    // RPC 아이디 설정
    client.set_id("rpc_client_1");

    // 브로커 설정
    std::string broker_ip = "127.0.0.1";
    unsigned short broker_port = 54321;
    client.set_broker(broker_ip, broker_port);

    // 연결 타임아웃 설정
    auto connection_timeout = std::chrono::milliseconds(3000);
    client.set_connection_timeout(connection_timeout);

    // std::chrono::seconds tcp_sleep_time = std::chrono::seconds(60);
    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(10);
    //   pub, sub 2개의 tcp 연결이 내부적으로 존재하므로,
    //   종료(disconnect) 시, tcp_sleep_time * 2배의 시간이 걸린다.
    if (!client.connect(tcp_sleep_time)) { // Broker 와 연결 시도
        // NOTE: broker 와 연결 실패 시, 연결 재시도를 해도 됨.
        rpc_client_logger->error(
            "<orange>Failed</orange> to connect RPC Client to broker. "
            "Check network connectivity and broker status.");
#ifdef _WIN32
        WSACleanup();
#endif
        return (-1);
    }

    req_t task{ "analyze", 100, { 3.14 } }; // RPC 요청 구조체 생성
    auto request_timeout = std::chrono::seconds(5); // RPC 호출 타임아웃 설정
    auto [status, response] = client.request_analysis(task, request_timeout); // RPC 호출

    if (status.ok()) {
        // RPC 호출 성공 시, 응답 결과를 로그에 출력
        rpc_client_logger->info(
            "RPC Call <gold>Success</gold>: "
            "<light_green>{0}</light_green>, <pink>{1}</pink>, <purple>{2}</purple>",
            response.is_success, response.message, response.details.dump());
    }
    else {
        // RPC 호출 실패 시, 오류 코드에 따른 상세 분석과 조치 방안을 로그에 출력
        rpc_client_logger->error("--- RPC <orange>Failure</orange> Analysis Report ---");
        switch (status.code) {
        case rpc_error_code::connection_broken:
            rpc_client_logger->critical(" <yellow>Network channel disconnected.</yellow>"
                " Initializing reconnection timer loop.");
            break;
        case rpc_error_code::timeout:
            rpc_client_logger->warn(" <yellow>Broker or Server is under heavy load.</yellow>"
                " Retrying call after backoff delay.");
            break;
        case rpc_error_code::service_not_found:
            rpc_client_logger->error(" <yellow>Target node does not register this service.</yellow>"
                " Verify interface specs or endpoint versions.");
            break;
        case rpc_error_code::invalid_argument:
            rpc_client_logger->error(" <yellow>Sent struct schema does not match the receiving endpoint struct contract.</yellow>");
            break;
        case rpc_error_code::internal_server_error:
            rpc_client_logger->error(" <yellow>Server threw an unhandled business logic exception.</yellow>"
                " Track server runtime logs.");
            break;
        default:
            rpc_client_logger->error(" <yellow>An unrecognized system error occurred.</yellow>");
            break;
        }
        rpc_client_logger->error(" Detailed Error Message: <gold>{0}</gold>", status.message);
    }

    rpc_client_logger->info("<grey>Client is disconnecting from broker.</grey> Wait a moment...");
    client.disconnect(); // RPC Client 종료
    rpc_client_logger->info("Client disconnected <green>successfully</green>.");

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
