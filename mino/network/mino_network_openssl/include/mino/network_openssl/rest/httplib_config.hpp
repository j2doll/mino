#pragma once

#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef USE_OPENSSL
#   ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#       define CPPHTTPLIB_OPENSSL_SUPPORT
#   endif
#   include <openssl/opensslv.h>
#   if OPENSSL_VERSION_NUMBER <= 0x101010ffL
//  OpenSSL 1.1.1 및 그 이하 버전이면, v0.14 버전의 httplib.h를 포함
#       include "httplib/v0_14/httplib.h"
#   else
//  OpenSSL 3.0 이상이면, 최신 httplib.h를 포함
#       include "httplib/lastest/httplib.h"
#   endif
#else
//  OpenSSL을 사용하지 않는 경우, 최신 httplib.h를 포함
#   include "httplib/lastest/httplib.h"
#endif

