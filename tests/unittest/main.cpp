#include <iostream>
#include <string>
#include <optional>
#include "catch_amalgamated.hpp"

int main(int argc, char* argv[]) {
    Catch::Session session;

    // 1. 세션 기본 설정 가져오기
    auto configData = session.configData();

    // 2. 성공한 assertion도 상세 출력 (-s 와 동일)
    configData.showSuccessfulTests = true;

    // 3. 상세도 High 설정 (-v high 와 동일)
    configData.verbosity = Catch::Verbosity::High;

    // 4. 변경된 설정 적용
    session.useConfigData(configData);

    // 5. 커맨드라인 인자 파싱 (실행 시 추가로 준 인자가 우선 적용됨)
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        std::cerr << "Error applying command line arguments. Return code: " << returnCode << std::endl;
        return returnCode;
    }

    return session.run();
}

