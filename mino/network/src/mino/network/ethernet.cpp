#include <system_error>

#include "mino/network/ethernet.hpp"

namespace mino::network {

    std::optional<std::string> init_socket() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::string errorMsg = std::system_category().message(result);
            return errorMsg; // failed
        }
#endif
        return std::nullopt; // success
    }

    std::optional<std::string> close_socket() {
#ifdef _WIN32
        auto result = WSACleanup();
        if (result != 0) {
            std::string errorMsg = std::system_category().message(result);
            return errorMsg; // failed
        }
#endif
        return std::nullopt; // success
    }

    sock::sock() {
        auto ret = init_socket();
        if (ret.has_value()) {
            std::cerr << "Failed: " << ret.value() << std::endl;
            return;
        }
    }

    sock::~sock() {
        auto ret = close_socket();
        if (ret.has_value()) {
            std::cerr << "Failed: " << ret.value() << std::endl;
            return;
        }
    }

} // namespace mino::network
