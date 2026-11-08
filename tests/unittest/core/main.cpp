#include <iostream>
#include <string>
#include <optional>

#include "catch_amalgamated.hpp"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOGDI
#      define NOGDI
#   endif
#   ifndef NOUSER
#      define NOUSER
#   endif
#   include <windows.h>
#endif

int main(int argc, char* argv[]) {
#if defined(_WIN32)
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
#endif

    Catch::Session session;

    auto configData = session.configData();
    configData.showSuccessfulTests = true;
    configData.verbosity = Catch::Verbosity::High;
    session.useConfigData(configData);

    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        std::cerr << "Error applying command line arguments. Return code: " << returnCode << std::endl;
        return returnCode;
    }

    int ret = session.run();
    if (ret != 0) {
        std::cerr << "Unit tests failed. Return code: " << ret << std::endl;
    }
    return ret;
}

