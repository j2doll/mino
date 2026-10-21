#include "test_common.hpp"

#include <string>
#include <vector>
#include <array>
#include <list>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <tuple>

namespace {
    // 사용자 정의 구조체 예제
    struct player_stat {
        std::string name;
        int hp;
        double weight;
    };

    // 사용자 정의 구조체에 대한 operator<< 오버로딩
    static std::ostream& operator<<(std::ostream& os, const player_stat& p) {
        os << p.name << ":" << p.hp << ":" << p.weight;
        return os;
    }

    //-----------------------------------------------
    // 사용자 정의 Non-POD 클래스 예시
    class Character {
    public:
        Character(std::string name, int hp, int mp)
            : name_(std::move(name)), hp_(hp), mp_(mp) {
        }
        virtual ~Character() = default;

        // operator<<가 정의되어 있으면 format의 NamedArg가 자동으로 인식하여 출력합니다.
        friend std::ostream& operator<<(std::ostream& os, const Character& c) {
            return os << c.name_ << "[HP: " << c.hp_ << "/MP: " << c.mp_ << "]";
        }

    private:
        std::string name_;
        int hp_;
        int mp_;
    };


} // namespace

void test_format() {
    namespace mcs = mino::core::string;
    namespace mcsf = mino::core::string::format;
    namespace mcsp = mino::core::string::print;

    TEST_SECTION("string::format examples");

    /////////////////////////////////////////////////////
    // 1. 기본 산술 및 불리언 타입
    int count = 42;
    double pi = 3.14159;
    bool is_active = true;
    auto s1 = mcsf::format("정수: {}, 부동소수점: {}, 불리언: {}\n", count, pi, is_active);
    TEST_CHECK(s1 == "정수: 42, 부동소수점: 3.14159, 불리언: 1\n");

    /////////////////////////////////////////////////////
    // 2. 문자열 타입
    const char* c_str = "C-Style String";
    std::string cpp_str = "std::string";
    std::string_view sv = "std::string_view";
    auto s2 = mcsf::format("문자열 모음: {}, {}, {}\n", c_str, cpp_str, sv);
    TEST_CHECK(s2.find("C-Style String") != std::string::npos);
    TEST_CHECK(s2.find("std::string") != std::string::npos);
    TEST_CHECK(s2.find("std::string_view") != std::string::npos);

    /////////////////////////////////////////////////////
    // 3. 순차 컨테이너
    std::vector<int> vec = { 1, 2, 3 };
    std::array<int, 3> arr = { 4, 5, 6 };
    std::list<std::string> lst = { "alpha", "beta", "gamma" };
    std::deque<double> deq = { 1.1, 2.2, 3.3 };
    auto s3v = mcsf::format("std::vector:  {}\n", vec);
    auto s3a = mcsf::format("std::array:   {}\n", arr);
    auto s3l = mcsf::format("std::list:    {}\n", lst);
    auto s3d = mcsf::format("std::deque:   {}\n", deq);
    TEST_CHECK(s3v == "std::vector:  [1, 2, 3]\n");
    TEST_CHECK(s3a == "std::array:   [4, 5, 6]\n");
    TEST_CHECK(s3l == "std::list:    [alpha, beta, gamma]\n");
    TEST_CHECK(s3d == "std::deque:   [1.1, 2.2, 3.3]\n");

    /////////////////////////////////////////////////////
    // 4. 집합 컨테이너
    std::set<int> ordered_set = { 30, 10, 20 };
    std::unordered_set<std::string> hash_set = { "apple", "banana" };
    auto s4o = mcsf::format("std::set:           {}\n", ordered_set);
    auto s4h = mcsf::format("std::unordered_set: {}\n", hash_set);
    TEST_CHECK(s4o == "std::set:           [10, 20, 30]\n");
    TEST_CHECK(s4h.find("apple") != std::string::npos);
    TEST_CHECK(s4h.find("banana") != std::string::npos);

    /////////////////////////////////////////////////////
    // 5. 키-값 매핑 컨테이너
    std::map<std::string, int> scores = { {"Alice", 95}, {"Bob", 88} };
    std::unordered_map<int, std::string> lookup = { {1, "One"}, {2, "Two"} };
    auto s5m = mcsf::format("std::map:           {}\n", scores);
    auto s5u = mcsf::format("std::unordered_map: {}\n", lookup);
    TEST_CHECK(s5m == "std::map:           [(Alice: 95), (Bob: 88)]\n");
    TEST_CHECK(s5u.find("One") != std::string::npos);
    TEST_CHECK(s5u.find("Two") != std::string::npos);

    /////////////////////////////////////////////////////
    // 6. std::pair & std::tuple
    std::pair<std::string, int> entry = { "ItemA", 1500 };
    std::tuple<int, std::string, double, bool> complex_tuple = { 101, "Server-01", 99.8, true };
    auto s6p = mcsf::format("std::pair:  {}\n", entry);
    auto s6t = mcsf::format("std::tuple: {}\n", complex_tuple);
    TEST_CHECK(s6p == "std::pair:  (ItemA: 1500)\n");
    TEST_CHECK(s6t == "std::tuple: (101, Server-01, 99.8, 1)\n");

    /////////////////////////////////////////////////////
    // 7. 중첩 컨테이너
    std::vector<std::vector<int>> matrix = { {1, 2}, {3, 4}, {5, 6} };
    std::map<std::string, std::vector<int>> grouped_data = {
        {"group_a", {10, 20}},
        {"group_b", {30, 40, 50}}
    };
    auto s7m = mcsf::format("행렬 (2D Vector): {}\n", matrix);
    auto s7g = mcsf::format("중첩 맵 (Map<K, Vec<V>>): {}\n", grouped_data);
    TEST_CHECK(s7m == "행렬 (2D Vector): [[1, 2], [3, 4], [5, 6]]\n");
    TEST_CHECK(s7g == "중첩 맵 (Map<K, Vec<V>>): [(group_a: [10, 20]), (group_b: [30, 40, 50])]\n");

    /////////////////////////////////////////////////////
    // 8. 커스텀 자료구조
    player_stat main_player{ "Shadow", 75, 68.5 };
    std::vector<player_stat> party = {
        {"Healer_Kim", 72, 80.0},
        {"Tanker_Lee", 76, 55.4}
    };
    auto s8s = mcsf::format("단일 객체: {}\n", main_player);
    auto s8v = mcsf::format("객체 벡터: {}\n", party);
    TEST_CHECK(s8s == "단일 객체: Shadow:75:68.5\n");
    TEST_CHECK(s8v == "객체 벡터: [Healer_Kim:72:80, Tanker_Lee:76:55.4]\n");

    /////////////////////////////////////////////////////
    // 9. 문법 오류/인자 개수 불일치 발생 시 (fallback 동작 확인)
    auto s9a = mcsf::format("인자 부족: {}, {}, {}\n", 1);
    TEST_CHECK(s9a == "인자 부족: 1, {missing_arg}, {missing_arg}\n");

    auto s9b = mcsf::format("인자 초과: {}\n", 1, 2, 3);
    TEST_CHECK(s9b == "인자 초과: 1\n {extra_args_omitted}");

    auto s9c = mcsf::format("잘못된 인자: {abc}\n", 1);
    TEST_CHECK(s9c == "잘못된 인자: {invalid_syntax}abc{unmatched_bracket}\n {extra_args_omitted}");

    /////////////////////////////////////////////////////
    // 10. 이름 기반 플레이스홀더({key}) 포맷팅 예제 (formatss 사용)
    std::string str1 = "good";
    std::string res1 = mcsf::formatss("Hello {name}, {world}",
        {
            {"name", "John"},
            {"world", str1}
        }
    );
    TEST_CHECK(res1 == "Hello John, good");

    /////////////////////////////////////////////////////
    // 11. Non-POD 클래스, int, double, bool 혼합 예제 (formatss 사용)
    int level = 50;
    double critical_rate = 74.25;
    bool is_online = true;
    Character hero("Mage", 1200, 3500);

    std::string res2 = mcsf::formatss(
        "상태: {player} | 레벨: {lv} | 치명타: {crit}% | 접속여부: {online}",
        {
            {"player", hero},            // Non-POD 클래스 (operator<< 지원)
            {"lv", level},               // int
            {"crit", critical_rate},     // double
            {"online", is_online}        // bool (write_arg를 통해 "true"로 출력)
        }
    );
    TEST_CHECK(res2 == "상태: Mage[HP: 1200/MP: 3500] | 레벨: 50 | 치명타: 74.25% | 접속여부: true");

    /////////////////////////////////////////////////////
    // 12. JSON 포맷 예제 (중괄호 이스케이프 처리, formatss 사용)
    // - {{ 는 단일 { 로 치환
    // - }} 는 단일 } 로 치환
    // - 전달되지 않은 키({unknown})는 원문 그대로 유지됨
    std::string res3 = mcsf::formatss(
        "JSON 포맷: {{ \"id\": \"{id}\", \"etc\": \"{unknown}\" }}",
        { {"id", "user_1024"} }
    );
    TEST_CHECK(res3 == "JSON 포맷: { \"id\": \"user_1024\", \"etc\": \"{unknown}\" }");

    /////////////////////////////////////////////////////
    // 13. 환경 변수 기반 포맷 예제 (std::unordered_map 사용, formatss 사용)
    std::unordered_map<std::string, std::string> env_vars = {
        {"HOST", "127.0.0.1"},
        {"PORT", "8080"},
        {"ENV", "Production"}
    };

    std::string res4 = mcsf::formatss(
        "Server starting on http://{HOST}:{PORT} (Mode: {ENV})",
        env_vars
    );
    TEST_CHECK(res4 == "Server starting on http://127.0.0.1:8080 (Mode: Production)");

    /////////////////////////////////////////////////////
    // 14. std::map 기반 포맷 예제 (formatss 사용)
    std::map<std::string, int> user_scores = {
        {"Alice", 95},
        {"Bob", 88}
    };

    // std::map을 formatss에 직접 전달
    std::string result = mcsf::formatss("Alice: {Alice}, Bob: {Bob}", user_scores);
    // 결과 -> "Alice: 95, Bob: 88"
    TEST_CHECK(result == "Alice: 95, Bob: 88");

    /////////////////////////////////////////////////////
    // 15. 빈 괄호 {} 기반의 기존 포맷팅도 오버로딩을 통해 문제없이 동작합니다.
    std::vector<int> numbers = { 1, 2, 3 };
    std::string res5 = mcsf::format("List: {}, Size: {}", numbers, numbers.size());
    TEST_CHECK(res5 == "List: [1, 2, 3], Size: 3");

    mcsp::print(">>> [SUCCESS] test_format_examples() passed.");
}
