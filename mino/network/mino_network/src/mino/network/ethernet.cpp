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
#ifdef USE_CURL
        curl_global_cleanup();
#endif
        auto ret = close_socket();
        if (ret.has_value()) {
            std::cerr << "Failed: " << ret.value() << std::endl;
            return;
        }
    } // sock::~sock()

    // 포트 사용 여부 검사 구현
    bool is_port_in_use(uint16_t port, transport_protocol proto, const std::string& ip) {
#ifdef _WIN32
        // Winsock이 초기화되어 있지 않을 수도 있으므로 호출 (WSAStartup은 중복 호출 시 카운트 증가)
        auto init_err = init_socket();
        if (init_err.has_value()) {
            return false;
        }
#endif

        int type = (proto == transport_protocol::tcp) ? SOCK_STREAM : SOCK_DGRAM;
        int protocol = (proto == transport_protocol::tcp) ? IPPROTO_TCP : IPPROTO_UDP;

        socket_t s = ::socket(AF_INET, type, protocol);

#ifdef _WIN32
        if (s == INVALID_SOCKET) {
            close_socket();
            return false;
        }
#else
        if (s < 0) {
            return false;
        }
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
#ifdef _WIN32
            ::closesocket(s);
            close_socket();
#else
            ::close(s);
#endif
            return false;
        }

        // bind 시도 (포트 선점 여부를 확인하기 위해 SO_REUSEADDR은 설정하지 않음)
        int res = ::bind(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
        bool in_use = false;

        if (res != 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEADDRINUSE || err == WSAEACCES) {
                in_use = true;
            }
#else
            if (errno == EADDRINUSE || errno == EACCES) {
                in_use = true;
            }
#endif
        }

#ifdef _WIN32
        ::closesocket(s);
        close_socket();
#else
        ::close(s);
#endif

        return in_use;
    } // bool is_port_in_use()...

} // namespace mino::network
