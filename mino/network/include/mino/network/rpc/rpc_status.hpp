#pragma once

#include <string>

namespace mino::network::rpc {

    enum class rpc_error_code { // RPC 오류 코드
        success = 0,           // 성공
        connection_broken,     // 연결 끊김    
        timeout,               // 타임아웃     
        service_not_found,     // 서비스 없음     
        invalid_argument,      // 잘못된 인자  
        internal_server_error, // 내부 서버 오류 
        unknown_error          // 알 수 없는 오류
    };

    struct  rpc_status { // RPC 상태 구조체
        rpc_error_code code{ rpc_error_code::success };
        std::string message; // 메시지 (성공 시 빈 문자열)
        bool ok() const { return code == rpc_error_code::success; } // 성공 여부 확인
    };

} 