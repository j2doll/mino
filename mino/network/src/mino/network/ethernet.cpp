#include <system_error>

#ifdef USE_CURL
    #include <curl/curl.h>
#endif

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
#ifdef USE_CURL
        auto curl_ret = curl_global_init(CURL_GLOBAL_ALL);
        if (curl_ret != CURLE_OK) {
            std::cerr << "Failed to initialize cURL: " << curl_easy_strerror(curl_ret) << std::endl;
            return;
        }
#endif
    }

    sock::~sock() {
        auto ret = close_socket();
        if (ret.has_value()) {
            std::cerr << "Failed: " << ret.value() << std::endl;
            return;
        }
#ifdef USE_CURL
        curl_global_cleanup();
#endif
    }

} // namespace mino::network
