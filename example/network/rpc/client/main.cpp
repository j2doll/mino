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

// network rpc client
#include "mino/network/ethernet.hpp"
#include "mino/network/rpc/rpc_client_base.hpp"

// RPC 연동을 위한 공통 자료
#include "../rpc_example_common.hpp"

// custom business log class
class my_business_client : public mino::network::rpc::rpc_client_base {
public:
    using req_t = my_app::domain::task_request;
    using res_t = my_app::domain::task_response;
    using rpc_status = mino::network::rpc::rpc_status;

    std::pair<rpc_status, res_t> request_analysis(
        const req_t& request,
        std::chrono::seconds timeout = std::chrono::seconds(10))
    {
        using rpc_error_code = mino::network::rpc::rpc_error_code;

        res_t final_result;
        json req_json;
        rpc_status status;

        // 요청 직렬화
        try {
            req_json = to_json(request);
        }
        catch (const std::invalid_argument& ex) {
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

        // 서버 RPC 서비스 호출
        auto rpc_result = call_raw(std::string(my_app::domain::service_name), req_json, timeout);
        status = rpc_result.first;

        if (status.ok()) {
            try {
                const json& response_json = rpc_result.second;
                final_result = parse_response(response_json);
            }
            catch (const std::invalid_argument& ex) {
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

private:
    // task_request DTO -> mino::core::json::value 변환
    json to_json(const req_t& req) {
        json j;
        j["command"] = req.command;
        j["count"] = req.count;

        mino::core::json::array_t arr;
        for (double p : req.parameters) {
            arr.emplace_back(p);
        }
        j["parameters"] = std::move(arr);
        return j;
    }

    // mino::core::json::value -> task_response DTO 역직렬화
    res_t parse_response(const json& val) {
        if (!val.is_object()) {
            throw std::invalid_argument("Response payload is not a JSON object");
        }

        res_t res;
        if (val.has_path("is_success")) {
            res.is_success = const_cast<json&>(val)["is_success"].get_bool();
        }
        if (val.has_path("message")) {
            res.message = const_cast<json&>(val)["message"].get_string();
        }
        if (val.has_path("details")) {
            res.details = const_cast<json&>(val)["details"];
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

    using req_t = my_app::domain::task_request;
    using rpc_error_code = mino::network::rpc::rpc_error_code;

    my_business_client client;

    // tinylog 기반 콘솔 싱크 및 로거 설정
    namespace mclt = mino::core::log::tinylog;
    auto rpc_client_console_sink = std::make_shared<mclt::console_sink>("rpc_client_console");
    auto rpc_client_logger = std::make_shared<mclt::logger>("rpc_client_logger");
    rpc_client_logger->add_sink(rpc_client_console_sink);
    rpc_client_logger->set_level(mclt::log_level::debug);
    mclt::logger::register_logger(rpc_client_logger);

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

    std::chrono::seconds tcp_sleep_time = std::chrono::seconds(10);
    if (!client.connect(tcp_sleep_time)) {
        rpc_client_logger->error(
            "<bright_yellow>Failed</bright_yellow> to connect RPC Client to broker. "
            "Check network connectivity and broker status.");
#ifdef _WIN32
        WSACleanup();
#endif
        return (-1);
    }

    req_t task{ "analyze", 100, { 3.14 } };
    auto request_timeout = std::chrono::seconds(5);
    auto [status, response] = client.request_analysis(task, request_timeout);

    if (status.ok()) {
        std::string details_str = mino::core::json::serializer::serialize(response.details);
        rpc_client_logger->info(
            "RPC Call <yellow>Success</yellow>: "
            "<bright_green>{}</bright_green>, <pink>{}</pink>, <magenta>{}</magenta>",
            response.is_success, response.message, details_str);
    }
    else {
        rpc_client_logger->error("--- RPC <bright_yellow>Failure</bright_yellow> Analysis Report ---");
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
        rpc_client_logger->error(" Detailed Error Message: <yellow>{}</yellow>", status.message);
    }

    rpc_client_logger->info("<gray>Client is disconnecting from broker.</gray> Wait a moment...");
    client.disconnect();
    rpc_client_logger->info("Client disconnected <green>successfully</green>.");

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
