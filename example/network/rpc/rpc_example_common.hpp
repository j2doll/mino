#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "mino/core/json/json.hpp"

namespace my_app::domain {

    inline constexpr std::string_view service_name = "task_service";

    struct task_request {
        std::string command;
        int count{ 0 };
        std::vector<double> parameters;
    }; 

    struct task_response {
        bool is_success{ false };
        std::string message;
        mino::core::json::value details;
    };

} // namespace my_app::domain
