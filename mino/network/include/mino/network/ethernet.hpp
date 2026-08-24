#pragma once 

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <memory>
#include <mutex>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <algorithm>
#include <optional>
#include <variant>
#include <any>

#include <cstring>
#include <ctime>

// Operating system dependent parts
 
#if defined(__linux__)
    #define USE_PTHREAD
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        // Mac OS
        #define USE_PTHREAD
    #elif TARGET_OS_IPHONE
        // iOS
        #define USE_PTHREAD
    #endif
#elif defined(__ANDROID__)
    // Android
    #define USE_PTHREAD
#elif defined(__FreeBSD__)
    // FreeBSD
    #define USE_PTHREAD
#elif defined(__NetBSD__)
    // NetBSD
    #define USE_PTHREAD
#elif defined(__OpenBSD__)
    // OpenBSD
    #define USE_PTHREAD
#elif defined(__DragonFly__)
    // DragonFly BSD
    #define USE_PTHREAD
#elif defined(__sun)
    #ifdef __SVR4
        //  Solaris
        #define USE_PTHREAD
    #else
        // SunOS
        #define USE_PTHREAD
    #endif
#elif defined(__QNX__)
    // QNX
    #define USE_PTHREAD
#elif defined(_SCO_DS)
    // SCO Unix
    #define USE_PTHREAD
#elif defined(__minix)
    // Minix
    #define USE_PTHREAD
#elif defined(__VMS)
    // VMS (OpenVMS)
    #define USE_PTHREAD
#elif defined(__OS2__)
    // OS/2
    #define USE_PTHREAD
#elif defined(__HAIKU__)
    // Haiku OS
    #define USE_PTHREAD
#elif defined(__sgi)
    // IRIX
    #define USE_PTHREAD
#elif defined(__BEOS__)
    // BeOS
    #define USE_PTHREAD
#elif defined(__amigaos__)
    // AmigaOS
    #define USE_PTHREAD
#elif defined(__MVS__)
    // IBM z/OS
    #define USE_PTHREAD
#endif

#ifdef USE_PTHREAD
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <netdb.h>

    using socket_t = int;
#endif

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
    using socket_t = SOCKET;
#endif

namespace mino::network {
    std::optional<std::string> init_socket();
    std::optional<std::string> close_socket();

    class sock {
    public:
        sock();
        ~sock();
    };

} // namespace mino::network


