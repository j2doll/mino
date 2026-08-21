#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace my_app::domain {

    // RPC 서비스 이름 정의
    constexpr const char* service_name = "task_analysis_service";

    // "task_analysis_service" RPC 서비스의 요청(Request) 인자 
    struct task_request {
        std::string         command;
        int                 target_id;
        std::vector<double> parameters;
    };

    // 직렬화: task_request -> json
    inline void to_json(nlohmann::json& j, const task_request& p) {
        j = nlohmann::json{
            {"command", p.command},
            {"target_id", p.target_id},
            {"parameters", p.parameters}
        };
    }

    // 역직렬화: json -> task_request
    inline void from_json(const nlohmann::json& j, task_request& p) {
        j.at("command").get_to(p.command);
        j.at("target_id").get_to(p.target_id);
        j.at("parameters").get_to(p.parameters);
    }

    // "task_analysis_service" RPC 서비스의 응답(Response) 결과  
    struct task_response {
        bool           is_success;
        std::string    message;
        nlohmann::json details;
    };

    // 직렬화: task_response -> json
    inline void to_json(nlohmann::json& j, const task_response& p) {
        j = nlohmann::json{
            {"is_success", p.is_success},
            {"message", p.message},
            {"details", p.details}
        };
    }

    // 역직렬화: json -> task_response
    inline void from_json(const nlohmann::json& j, task_response& p) {
        j.at("is_success").get_to(p.is_success);
        j.at("message").get_to(p.message);
        // assign directly for nlohmann::json field (get_to isn't selected for basic_json)
        p.details = j.at("details");
    }

} // namespace my_app::domain

//---------------------------------------------------------------
// nlohmann::json 라이브러리 3.9.0 이상에서는 쉽게 매크로로 설정 가능.
//---------------------------------------------------------------
// namespace my_app::domain {
// 
//     // RPC 서비스 이름 정의
//     constexpr const char* service_name = "task_analysis_service";
// 
//     // "task_analysis_service" RPC 서버스의 요청(Request) 인자 
//     struct task_request {
//         std::string         command;
//         int                 target_id;
//         std::vector<double> parameters;
//     };
//     NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(task_request, command, target_id, parameters)
//         // 구조체 직렬화 처리를 놀먼JSON 직렬화 매크로를 사용하여 정의.   
// 
//         // "task_analysis_service" RPC 서버스의 응답(Response) 결과  
//         struct task_response {
//         bool                is_success;
//         std::string         message;
//         nlohmann::json      details;
//     };
//     NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(task_response, is_success, message, details)
// 
// } // namespace my_app::domain
//---------------------------------------------------------------


