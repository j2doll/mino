#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <string>
#include <cassert>

#include "mino/core/string/string.hpp"
#include "mino/core/json/json.hpp"

void test_value_constructors_and_types() {
    namespace mjson = mino::core::json;
    using value = mjson::value;
    using array_t = mjson::array_t;
    using object_t = mjson::object_t;
    using value_type = mjson::value_type;
    
    // 1. Default constructor (Null)
    value v_null;
    assert(v_null.is_null());
    assert(v_null.get_type() == value_type::null_type);

    // 2. Bool constructor
    value v_bool(true);
    assert(v_bool.is_bool());
    assert(v_bool.get_type() == value_type::boolean_type);
    assert(v_bool.get_bool() == true);

    // 3. Double constructor
    value v_double(3.14);
    assert(v_double.is_number());
    assert(v_double.get_type() == value_type::number_type);
    assert(v_double.get_number() == 3.14);

    // 4. Int constructor
    value v_int(42);
    assert(v_int.is_number());
    assert(v_int.get_number() == 42.0);

    // 5. const std::string& constructor
    std::string str = "hello";
    value v_str_lval(str);
    assert(v_str_lval.is_string());
    assert(v_str_lval.get_type() == value_type::string_type);
    assert(v_str_lval.get_string() == "hello");

    // 6. std::string&& constructor
    value v_str_rval(std::string("world"));
    assert(v_str_rval.is_string());
    assert(v_str_rval.get_string() == "world");

    // 7. const char* constructor
    value v_cstr("const char test");
    assert(v_cstr.is_string());
    assert(v_cstr.get_string() == "const char test");

    // 8. const array_t& constructor
    array_t arr_lval = { value(1.0), value("item") };
    value v_arr_lval(arr_lval);
    assert(v_arr_lval.is_array());
    assert(v_arr_lval.get_type() == value_type::array_type);

    // 9. array_t&& constructor
    value v_arr_rval(array_t{ value(true), value(false) });
    assert(v_arr_rval.is_array());

    // 10. const object_t& constructor
    object_t obj_lval = { {"key", value("value")} };
    value v_obj_lval(obj_lval);
    assert(v_obj_lval.is_object());
    assert(v_obj_lval.get_type() == value_type::object_type);

    // 11. object_t&& constructor
    value v_obj_rval(object_t{ {"a", value(1.0)} });
    assert(v_obj_rval.is_object());

    // 12. Public member variable access test (data)
    value v_data_test;
    v_data_test.data = 100.0;
    assert(v_data_test.is_number());
    assert(std::get<double>(v_data_test.data) == 100.0);

    std::cout << "[PASS] value constructors and type checks\n";
}

void test_value_getters_and_operators() {
    namespace mjson = mino::core::json;
    using value = mjson::value;
    using array_t = mjson::array_t;
    using object_t = mjson::object_t;

    // Default fallback getters test
    value v_null;
    assert(v_null.get_number(99.9) == 99.9);
    assert(v_null.get_bool(true) == true);
    assert(v_null.get_string("fallback") == "fallback");

    // operator[](size_t index)
    value v_arr(array_t{ value("index_0"), value(123.0) });
    assert(v_arr[0].get_string() == "index_0");
    assert(v_arr[1].get_number() == 123.0);

    // operator[](const std::string& key)
    value v_obj(object_t{ {"name", value("mino")}, {"age", value(20.0)} });
    assert(v_obj["name"].get_string() == "mino");
    assert(v_obj["age"].get_number() == 20.0);

    std::cout << "[PASS] value getters and operators\n";
}

void test_parser() {
    namespace mjson = mino::core::json;
    using value = mjson::value;
    using array_t = mjson::array_t;
    using object_t = mjson::object_t;
    using parser = mjson::parser;

    std::string json_src = R"({
        "number": 123.45,
        "bool": true,
        "null_val": null,
        "str": "testing parser",
        "array": [1, false, "element"],
        "object": {
            "nested_key": "nested_value"
        }
    })";

    value parsed = parser::parse(json_src);
    assert(parsed.is_object());

    assert(parsed["number"].is_number() && parsed["number"].get_number() == 123.45);

    assert(parsed["bool"].is_bool() && parsed["bool"].get_bool() == true);

    assert(parsed["null_val"].is_null());

    assert(parsed["str"].is_string() && parsed["str"].get_string() == "testing parser");

    assert(parsed["array"].is_array());
    if (parsed["array"].is_array()) {
        auto& arr = std::get<array_t>(parsed["array"].data);
        size_t count = arr.size();
        assert(count == 3);
    }
    assert(parsed["array"][0].is_number() && parsed["array"][0].get_number() == 1.0);

    assert(parsed["object"].is_object());
    if (parsed["object"].is_object()) {
        auto& obj = std::get<object_t>(parsed["object"].data);
        assert(obj.find("nested_key") != obj.end()); // Check if the key('nested_key') exists
        assert(obj["nested_key"].is_string() && obj["nested_key"].get_string() == "nested_value");

        assert(parsed["object"]["nested_key"].is_string() && parsed["object"]["nested_key"].get_string() == "nested_value");
    }

    if (parsed.has_path("/str")) { // 존재하는 경로 
        // 존재함
        assert(true);
        std::cout << "Value at path '/str': " << parsed["str"].get_string() << "\n";
    }
    else {
        // 존재하지 않음
        assert(false); // This should not happen
        std::cout << "Path '/str' does not exist, which is unexpected.\n";
    }

    if (parsed.has_path("/str2")) { // 존재하지 않는 경로
        // 존재함
        assert(false); // This should not happen
        std::cout << "Value at path '/str2': " << parsed["str2"].get_string() << "\n";
    }
    else {
        // 존재하지 않음
        assert(true);
        std::cout << "Path '/str2' does not exist, as expected.\n";
    }

    if (parsed.has_path("/array/1")) { // 존재하는 array 요소 경로
        // 존재함
        assert(true);
        std::cout << "Value at path '/array/1': " << (parsed["array"][1].is_bool() ? (parsed["array"][1].get_bool() ? "true" : "false") : "not a bool") << "\n";
    }
    else {
        // 존재하지 않음
        assert(false); // This should not happen
        std::cout << "Path '/array/1' does not exist, which is unexpected.\n";
    }

    if (parsed.has_path("/object/nested_key")) { // 존재하는 object key 경로
        // 존재함
        assert(true);
        std::cout << "Value at path '/object/nested_key': " << parsed["object"]["nested_key"].get_string() << "\n";
    }
    else {
        // 존재하지 않음
        assert(false); // This should not happen
        std::cout << "Path '/object/nested_key' does not exist, which is unexpected.\n";
    }

    std::cout << "[PASS] parser::parse\n";
}

void test_serializer() {
    namespace mjson = mino::core::json;
    using value = mjson::value;
    using array_t = mjson::array_t;
    using object_t = mjson::object_t;
    using serializer = mjson::serializer;
    using parser = mjson::parser;

    object_t obj;
    obj["id"] = value(1.0);
    obj["valid"] = value(true);

    value val(obj);
    std::string serialized = serializer::serialize(val);

    // Serialized output verification via re-parsing
    value re_parsed = parser::parse(serialized);
    assert(re_parsed.is_object());
    assert(re_parsed["id"].get_number() == 1.0);
    assert(re_parsed["valid"].get_bool() == true);

    std::cout << "[PASS] serializer::serialize\n";
}

void test_file_reader() {
    namespace mjson = mino::core::json;
    using value = mjson::value;
    using array_t = mjson::array_t;
    using object_t = mjson::object_t;
    using file_reader = mjson::file_reader;

    std::string test_filename = "test_data.json";

    // Create temporary file for testing
    {
        std::ofstream out(test_filename);
        out << R"({
            "file_test": true,
            "code": 200
        })";
    }

    value val = file_reader::read_file(test_filename);
    assert(val.is_object());
    assert(val["file_test"].is_bool() && val["file_test"].get_bool() == true);
    assert(val["code"].is_number() && val["code"].get_number() == 200.0);

    // Test nonexistent file handling
    value invalid_val = file_reader::read_file("non_existent_file.json");
    assert(invalid_val.is_null());
    assert(invalid_val.is_object() == false);

    // Cleanup temporary file
    std::filesystem::remove(test_filename);

    std::cout << "[PASS] file_reader::read_file\n";
}

int main() {
    std::cout << "Starting JSON library tests...\n";

    test_value_constructors_and_types();
    test_value_getters_and_operators();
    test_parser();
    test_serializer();
    test_file_reader();

    std::cout << "All tests passed successfully!\n";
    return 0;
}
