#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace mino::network::rpc {

    // RPC 프로토콜 유틸리티 클래스
    class  rpc_protocol_util {
    public:

        // RPC 요청 메시지 직렬화: 요청 ID, 응답 토픽, 인자 JSON을 받아서 직렬화된 문자열로 반환
        static std::string serialize_request(
            const std::string& req_id, // 요청 ID
            const std::string& res_topic, // 응답 토픽
            const nlohmann::json& arg); // 인자 JSON

        // RPC 응답 메시지 직렬화: 요청 ID, 성공 여부, 오류 코드 문자열, 결과 또는 오류 JSON을 받아서 직렬화된 문자열로 반환
        static std::string serialize_response(
            const std::string& req_id, // 요청 ID
            bool success, // 성공 여부
            const std::string& err_code_str, // 오류 코드 문자열 (성공 시 빈 문자열)
            const nlohmann::json& result_or_err); // 결과 JSON (성공 시 결과, 실패 시 오류 정보)
    };

} 
