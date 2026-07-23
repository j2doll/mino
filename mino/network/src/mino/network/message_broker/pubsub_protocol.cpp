
#include "mino/network/message_broker/pubsub_protocol.hpp"

namespace mino::network::message_broker {

    // 가변 패킷을 ASCII 텍스트 문자열 형식으로 직렬화 조립 함수
    std::string make_packet(
        msg_type type,
        std::string_view topic,
        std::string_view msg_kind,
        std::string_view body)
    {
        auto now = std::chrono::system_clock::now();
        uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        std::string packet;
        packet.reserve(256 + body.size());

        // 각 필드를 명확하게 "키:값\n" 형태의 ASCII 문장으로 구성
        packet.append("TYPE:").append(std::to_string(static_cast<int>(type))).append("\n");
        packet.append("TIMESTAMP:").append(std::to_string(ts)).append("\n");
        packet.append("TOPIC:").append(topic).append("\n");
        packet.append("KIND:").append(msg_kind).append("\n");
        packet.append("BODY_LEN:").append(std::to_string(body.size())).append("\n");
        packet.append("\n"); // 헤더의 끝을 알리는 빈 줄(Delimiter) 추가

        if (!body.empty()) {
            packet.append(body);
        }
        return packet;
    }

    // ASCII 스트림 버퍼로부터 헤더 정보를 추출하는 정적 파싱 함수
    bool parse_ascii_header(std::string_view buffer, ascii_header& out_header)
    {
        // 헤더와 바디 경계선인 "\n\n" 스트링 서치
        size_t header_end = buffer.find("\n\n");
        if (header_end == std::string_view::npos) {
            return false; // 아직 헤더의 끝까지 데이터가 안 들어옴
        }

        out_header.header_full_size = header_end + 2; // "\n\n"을 포함한 헤더 스캔 크기
        std::string_view header_view = buffer.substr(0, header_end);

        // 한 줄씩 파싱 수행
        size_t pos = 0;
        while (pos < header_view.size()) {
            size_t next_line = header_view.find('\n', pos);
            if (next_line == std::string_view::npos) {
                next_line = header_view.size();
            }

            std::string_view line = header_view.substr(pos, next_line - pos);
            pos = next_line + 1;

            size_t colon = line.find(':');
            if (colon == std::string_view::npos) {
                continue;
            }

            std::string_view key = line.substr(0, colon);
            std::string_view val = line.substr(colon + 1);

            if (key == "TYPE") {
                out_header.type = static_cast<msg_type>(std::stoi(std::string(val)));
            }
            else if (key == "TIMESTAMP") {
                out_header.timestamp = std::stoull(std::string(val));
            }
            else if (key == "TOPIC") {
                out_header.topic = std::string(val);
            }
            else if (key == "KIND") {
                out_header.msg_kind = std::string(val);
            }
            else if (key == "BODY_LEN") {
                out_header.body_len = std::stoul(std::string(val));
            }
        }
        return true;
    }


}  