#pragma once

#include <string>

namespace mino::core::subsystem {

    struct packet {
        std::string command;
        std::string payload; // Decoded UTF-8 raw data
    };

    class protocol {
    public:
        // UTF-8 string -> Base64 string
        static std::string encode_base64(const std::string& text);

        // Base64 string -> UTF-8 string
        static std::string decode_base64(const std::string& b64_text);

        // Serialize packet: "COMMAND BASE64_PAYLOAD"
        static std::string serialize(const std::string& command, const std::string& raw_payload);

        // Deserialize packet line
        static bool deserialize(const std::string& line, packet& out_packet);
    };

} // namespace mino::core::subsystem
