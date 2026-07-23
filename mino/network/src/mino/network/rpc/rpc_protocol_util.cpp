#include <nlohmann/json.hpp>

#include "mino/network/rpc/rpc.hpp"

namespace mino::network::rpc {

    std::string rpc_protocol_util::serialize_request(const std::string& req_id, const std::string& res_topic, const nlohmann::json& arg) {
        nlohmann::json j;
        j["request_id"] = req_id;
        j["response_topic"] = res_topic;
        j["argument"] = arg;
        return j.dump();
    }

    std::string rpc_protocol_util::serialize_response(const std::string& req_id, bool success, const std::string& err_code_str, const nlohmann::json& result_or_err) {
        nlohmann::json j;
        j["request_id"] = req_id;
        j["success"] = success;
        j["error_code"] = err_code_str;
        j["result"] = result_or_err;
        return j.dump();
    }

} 