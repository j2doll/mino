#include "test_common.hpp"

void test_mutex_string() {
    namespace mcs = mino::core::string;
    TEST_SECTION("mutex_string");

    // Constructors & Assignment
    mcs::mutex_string ms1;
    TEST_CHECK(ms1.empty() && ms1.size() == 0);

    mcs::mutex_string ms2("initial_str");
    TEST_CHECK(ms2 == "initial_str");
    TEST_CHECK(ms2.length() == 11);

    mcs::mutex_string ms3 = ms2;
    TEST_CHECK(ms3.str() == ms2.str());

    mcs::mutex_string ms4 = std::move(ms3);
    TEST_CHECK(ms4 == "initial_str" && ms3.empty());

    ms1 = "assigned";
    TEST_CHECK(ms1 == "assigned");

    ms1 = std::string("assigned_std");
    TEST_CHECK(ms1 == "assigned_std");

    // Comparisons
    TEST_CHECK(ms1.str() != ms2.str());
    TEST_CHECK(ms1 != "other");
    TEST_CHECK("assigned_std" == ms1);
    TEST_CHECK(std::string("assigned_std") == ms1);
    TEST_CHECK("other" != ms1);
    TEST_CHECK(std::string("other") != ms1);

    // Explicit std::string cast
    std::string snap = static_cast<std::string>(ms1);
    TEST_CHECK(snap == "assigned_std");
    TEST_CHECK(ms1.str() == "assigned_std");

    // Capacity & state
    ms1.reserve(64);
    TEST_CHECK(ms1.capacity() >= 64);
    TEST_CHECK(ms1.max_size() > 0);
    ms1.shrink_to_fit();

    // Element access and setters
    ms1 = "hello";
    TEST_CHECK(ms1.at(0) == 'h');
    TEST_CHECK(ms1[1] == 'e');
    TEST_CHECK(ms1.front() == 'h');
    TEST_CHECK(ms1.back() == 'o');

    ms1.set(0, 'H');
    ms1.front('J');
    ms1.back('!');
    TEST_CHECK(ms1 == "Jell!");

    // Modifiers
    ms1.clear();
    TEST_CHECK(ms1.empty());

    ms1.push_back('A');
    ms1.push_back('B');
    ms1.pop_back();
    TEST_CHECK(ms1 == "A");

    ms1.assign("Base");
    TEST_CHECK(ms1 == "Base");

    ms1.assign(std::string("NewBase"));
    TEST_CHECK(ms1 == "NewBase");

    ms1.assign(3, 'X');
    TEST_CHECK(ms1 == "XXX");

    ms1.append("123");
    ms1.append(std::string("456"));
    ms1.append(2, '7');
    TEST_CHECK(ms1 == "XXX12345677");

    ms1 += "_";
    ms1 += std::string("end");
    ms1 += '!';
    TEST_CHECK(ms1.str().find("end!") != std::string::npos);

    ms1 = "ABCDEF";
    ms1.insert(3, "_INS_");
    TEST_CHECK(ms1 == "ABC_INS_DEF");

    ms1.insert(0, std::string("["));
    ms1.insert(ms1.size(), 1, ']');
    TEST_CHECK(ms1 == "[ABC_INS_DEF]");

    ms1 = "0123456789";
    ms1.erase(3, 4);
    TEST_CHECK(ms1 == "012789");

    ms1.replace(1, 2, "XX");
    TEST_CHECK(ms1 == "0XX789");

    ms1.replace(0, 1, std::string("ZZ"));
    TEST_CHECK(ms1 == "ZZXX789");

    ms1.replace(0, 2, 3, 'Q');
    TEST_CHECK(ms1 == "QQQXX789");

    ms1.resize(5);
    TEST_CHECK(ms1.size() == 5);

    ms1.resize(8, '#');
    TEST_CHECK(ms1 == "QQQXX###");

    // String operations
    TEST_CHECK(ms1.substr(0, 3) == "QQQ");

    char c_buf[10] = { 0 };
    ms1.copy(c_buf, 3, 0);
    TEST_CHECK(std::string(c_buf) == "QQQ");

    TEST_CHECK(ms1.compare("QQQXX###") == 0);
    TEST_CHECK(ms1.compare(0, 3, "QQQ") == 0);

    ms1 = "banana split";
    TEST_CHECK(ms1.find("na") == 2);
    TEST_CHECK(ms1.find(std::string("na"), 3) == 4);
    TEST_CHECK(ms1.find('s') == 7);

    TEST_CHECK(ms1.rfind("na") == 4);
    TEST_CHECK(ms1.rfind(std::string("na")) == 4);
    TEST_CHECK(ms1.rfind('a') == 5);

    TEST_CHECK(ms1.find_first_of("aeiou") == 1);
    TEST_CHECK(ms1.find_first_of(std::string("xyzs")) == 7);
    TEST_CHECK(ms1.find_first_of('p') == 8);

    TEST_CHECK(ms1.find_last_of("aeiou") == 10);
    TEST_CHECK(ms1.find_last_of(std::string("aeiou")) == 10);
    TEST_CHECK(ms1.find_last_of('b') == 0);

    TEST_CHECK(ms1.find_first_not_of("abn ") == 7);
    TEST_CHECK(ms1.find_first_not_of(std::string("abn ")) == 7);
    TEST_CHECK(ms1.find_first_not_of('b') == 1);

    TEST_CHECK(ms1.find_last_not_of("it") == 9);
    TEST_CHECK(ms1.find_last_not_of(std::string("it")) == 9);
    TEST_CHECK(ms1.find_last_not_of('t') == 10);

    // Swap
    mcs::mutex_string sw1("AAA"), sw2("BBB");

    sw1.swap(sw2);
    TEST_CHECK(sw1.str() == "BBB" && sw2.str() == "AAA");

    mcs::swap(sw1, sw2);
    TEST_CHECK(sw1.str() == "AAA" && sw2.str() == "BBB");

    std::string str_std = "CCC";
    sw1.swap(str_std);
    TEST_CHECK(sw1.str() == "CCC" && str_std == "AAA");

    // with / with_lock / synchronize / guard
    ms1 = "thread_safe_data";
    ms1.with([](std::string& s) {
        s += "_modified";
        });
    TEST_CHECK(ms1 == "thread_safe_data_modified");

    const mcs::mutex_string& const_ms = ms1;

    bool checked = const_ms.with_lock([](const std::string& s) {
        return s.size() > 0;
        });
    TEST_CHECK(checked);

    {
        std::string expected = ms1.str();
        auto locked = ms1.synchronize();

        TEST_CHECK(locked.owns_lock());
        TEST_CHECK(locked->size() == expected.size());
        TEST_CHECK(*locked == expected);
        TEST_CHECK(locked->find("thread") != std::string::npos);
    }

    {
        const auto guard = const_ms.guard();

        TEST_CHECK(guard.owns_lock());
        TEST_CHECK(guard->find("thread") != std::string::npos);
    }
}
