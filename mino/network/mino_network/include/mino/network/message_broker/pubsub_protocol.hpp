#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>

namespace mino::network::message_broker {

    // 메시지 유형을 나타내는 열거형 (subscribe 또는 publish)
    enum class msg_type : uint8_t {
        subscribe = 1,
        publish = 2
    };

    // ASCII 파싱 결과를 담을 편의용 구조체 (기존 바이너리 struct 대체)
    struct  ascii_header {
        msg_type type; // 메시지의 주요 유형(subscribe 또는 publish)
        uint64_t timestamp; // 메시지 생성 시점의 타임스탬프(밀리초 단위)
        std::string topic; // 메시지가 속한 주제(예: "sports", "general" 등)
        std::string msg_kind; // 메시지의 세부 종류(예: "text", "json", "binary", "alert", "log" 등)
        size_t body_len; // 메시지 본문의 길이(바이트 단위)
        size_t header_full_size; // 파싱 완료 후 버퍼를 잘라내기 위한 헤더 전체 문자열 크기
    };

    // 가변 패킷을 ASCII 텍스트 문자열 형식으로 직렬화 조립 함수
     std::string make_packet(
        msg_type type, // 메시지 유형 (subscribe 또는 publish)
        std::string_view topic, // 메시지가 속한 주제
        std::string_view msg_kind, // 메시지의 세부 종류
        std::string_view body = ""); // 메시지 본문 (기본값은 빈 문자열)

    // ASCII 스트림 버퍼로부터 헤더 정보를 추출하는 정적 파싱 함수
     bool parse_ascii_header(
        std::string_view buffer, // 입력 버퍼 (헤더와 본문이 포함된 전체 스트림)
        ascii_header& out_header); // 출력 매개변수로 파싱된 헤더 정보를 담을 구조체 참조

}  