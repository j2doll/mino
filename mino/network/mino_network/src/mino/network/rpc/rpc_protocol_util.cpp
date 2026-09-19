#include "mino/network/rpc/rpc_protocol_util.hpp"
#include "mino/core/json/json.hpp"

namespace mino::network::rpc {

    std::string rpc_protocol_util::serialize_request(
        const std::string& req_id,
        const std::string& res_topic,
        const mino::core::json::value& arg)
    {
        mino::core::json::value j;
        j["request_id"] = req_id;
        j["response_topic"] = res_topic;
        j["argument"] = arg;
        return mino::core::json::serializer::serialize(j);
    }

    std::string rpc_protocol_util::serialize_response(
        const std::string& req_id,
        bool success,
        const std::string& err_code_str,
        const mino::core::json::value& result_or_err)
    {
        mino::core::json::value j;
        j["request_id"] = req_id;
        j["success"] = success;
        j["error_code"] = err_code_str;
        j["result"] = result_or_err;
        return mino::core::json::serializer::serialize(j);
    }

}
