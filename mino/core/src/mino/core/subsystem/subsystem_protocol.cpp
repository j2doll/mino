#include "mino/core/subsystem/subsystem_protocol.hpp"
#include "mino/core/encoding/base64.hpp"

#include <vector>
#include <sstream>
#include <cstdint>

namespace mino::core::subsystem {

    std::string protocol::encode_base64(const std::string& text) {
        std::vector<uint8_t> bytes(text.begin(), text.end());
        return mino::core::encoding::base64_encode(bytes);
    }

    std::string protocol::decode_base64(const std::string& b64_text) {
        std::vector<uint8_t> bytes;
        if (mino::core::encoding::base64_decode(b64_text, bytes)) {
            return std::string(bytes.begin(), bytes.end());
        }
        return "";
    }

    std::string protocol::serialize(const std::string& command, const std::string& raw_payload) {
        if (raw_payload.empty()) {
            return command;
        }
        return command + " " + encode_base64(raw_payload);
    }

    bool protocol::deserialize(const std::string& line, packet& out_packet) {
        if (line.empty()) {
            return false;
        }

        std::istringstream iss(line);
        iss >> out_packet.command;

        std::string b64_payload;
        if (iss >> b64_payload) {
            out_packet.payload = decode_base64(b64_payload);
        }
        else {
            out_packet.payload.clear();
        }
        return !out_packet.command.empty();
    }

} // namespace mino::core::subsystem
