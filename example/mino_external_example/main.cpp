#include <iostream>
#include <string>
#include <exception>

#include "mino/external/json/json.hpp"

int main() {
    namespace mjson = mino::external::json;
    using json_t = mjson::nj;
    using json_ptr_t = mjson::njj;

    // Build a JSON object
    json_t j;
    j["project"]  = "mino";
    j["version"]  = 1.0;
    j["features"] = { "parsing", "serialization", "tests" };
    j["nested"]   = { {"enabled", true}, {"count", 3} };
    j["metadata"] = { {"author", "developer"}, {"license", "MIT"} };

    std::cout << "Constructed JSON:\n" << j.dump(2) << "\n\n";

    // --- Examples using functions from mino/external/json/json.hpp ---

    // 1) Use JSON pointer overloads (njj is an alias to nlohmann::json_pointer<std::string>)
    json_ptr_t ptrEnabled("/nested/enabled");
    bool enabledPtr = mjson::value_or(j, ptrEnabled, false);
    std::cout << "/nested/enabled : " << std::boolalpha << enabledPtr << "\n";

    json_ptr_t ptrCount("/nested/count");
    int countPtr = mjson::value_or<int>(j, ptrCount, 0);
    std::cout << "/nested/count : " << countPtr << "\n\n";

    // 2) Use path-string overloads (path format accepted by the project's helpers)
    bool enabledPath = mjson::value_or_path(j, "/nested/enabled", false);
    double version = mjson::value_or_path(j, "/version", 0.0);
    std::cout << "/nested/enabled : " << std::boolalpha << enabledPath << "\n";
    std::cout << "/version : " << version << "\n\n";

    // 3) Use typed getters with default values
    std::string proj = mjson::get_string(j, "/project", "unknown");
    std::cout << "get_string(/project): " << proj << "\n";

    int count = mjson::get_int(j, "/nested/count", -1);
    std::cout << "get_int(/nested/count): " << count << "\n";

    bool flag = mjson::get_bool(j, "/nested/enabled", false);
    std::cout << "get_bool(/nested/enabled): " << std::boolalpha << flag << "\n";

    double ver = mjson::get_double(j, "/version", 0.0);
    std::cout << "get_double(/version): " << ver << "\n\n";

    // 4) Check existence
    bool hasAuthor = mjson::exists(j, "/metadata/author");
    std::cout << "/metadata/author exists: " << std::boolalpha << hasAuthor << "\n";
    std::string author = mjson::value_or_path(j, "/metadata/author", std::string("n/a"));
    std::cout << "/metadata/author (value_or_path): " << author << "\n\n";

    // 5) Direct node access helper (returns pointer or nullptr)
    const json_t* node = mjson::get_node(j, json_ptr_t("/features/0"));
    if (node) {
        std::cout << "/features/0 : " << node->get<std::string>() << "\n";
    }
    else {
        std::cout << "f/features/0 not found\n";
    }

    return 0;
}
