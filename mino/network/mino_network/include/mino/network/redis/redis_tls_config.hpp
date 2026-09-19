#pragma once

#include <string>

namespace mino::network::redis {

    struct redis_tls_config {
        bool verify_peer{ false };
        std::string ca_cert_file;
        std::string ca_cert_path;
        std::string sni_hostname;
        std::string client_cert_file;
        std::string client_key_file;
    };

} // namespace mino::network::redis
