#include <iostream>
#include <variant>
#include <string>

#include "mino/core/result/result.hpp"
#include "mino/core/string/to_console_encoding.hpp"

// main() 외부 요소들을 익명 네임스페이스로 캡슐화
namespace {
    namespace mcr = mino::core::result;

    // 테스트용 커스텀 에러 구조체
    struct NetworkError {
        int code;
        std::string message;
    };

    struct AuthError {
        std::string reason;
    };

    // 성공 타입: int 또는 std::string
    using MySuccessTypes = std::variant<int, std::string>;

    // 실패 타입: NetworkError 또는 AuthError
    using MyFailTypes = std::variant<NetworkError, AuthError>;

    // Result 타입 별칭
    using MyResult = mcr::result_container<MySuccessTypes, MyFailTypes>;

    // 테스트용 비즈니스 로직 함수
    MyResult process_request(int id) {
        if (id == 1) {
            return MyResult::success(42); // int 성공
        }
        else if (id == 2) {
            return MyResult::success(std::string("처리 완료: 성공 메시지")); // string 성공
        }
        else if (id == 3) {
            return MyResult::fail(NetworkError{ 404, "서버를 찾을 수 없습니다." }); // NetworkError 실패
        }
        else {
            return MyResult::fail(AuthError{ "접근 권한이 없습니다." }); // AuthError 실패
        }
    }

} // anonymous namespace

int main(int argc, char* argv[]) {
    auto tce = mino::core::string::to_console_encoding;
    auto print = [](const auto&... args) { (std::cout << ... << args) << std::endl; };
    auto eprint = [](const auto&... args) { (std::cerr << ... << args) << std::endl; };
    std::ostream& (*endl)(std::ostream&) = std::endl;

    namespace mcr = mino::core::result;

    print(tce("=== result_container 테스트 시작 ==="));

    for (int id = 1; id <= 4; ++id) {
        print(endl, tce("[테스트 케이스 id = "), id, tce("]"));

        auto res = process_request(id);
        // id:1 이면, 성공(int) 반환
        // id:2 이면, 성공(string) 반환
        // id:3 이면, 실패(NetworkError) 반환
        // id:4 이면, 실패(AuthError) 반환

        // 1. is_success() 검사
        print(tce("성공 여부: "), (res.is_success() ? tce("SUCCESS") : tce("FAIL")) );

        // 2. match() 및 mcr::overload를 이용한 결과 처리
        res.match(
            // 성공 핸들러
            mcr::overload{
                [](int val) { // id:1 일때 호출됨
                    auto tce = mino::core::string::to_console_encoding;
                    std::cout << tce(">> [성공 - int 반환]: ") << val << std::endl;
                },
                [](const std::string& val) { // id:2 일때 호출됨
                    auto tce = mino::core::string::to_console_encoding;
                    std::cout << tce(">> [성공 - string 반환]: ") << tce(val) << std::endl;
                }
            },
            // 실패 핸들러
            mcr::overload{
                [](const NetworkError& err) { // id:3 일때 호출됨
                    auto tce = mino::core::string::to_console_encoding;
                    std::cout
                        << tce(">> [실패 - NetworkError]: 코드(") << err.code
                        << tce("), 메시지(") << tce(err.message)
                        << tce(")") << std::endl;
                },
                [](const AuthError& err) { // id:4 일때 호출됨
                    auto tce = mino::core::string::to_console_encoding;
                    std::cout
                        << tce(">> [실패 - AuthError]: 사유(")
                        << tce(err.reason)
                        << tce(")") << std::endl;
                }
            }
        );
    }

    print(endl, tce("=== 테스트 완료 ==="));

    return 0;
}
