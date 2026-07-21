#pragma once

#include <string>
#include <type_traits>
#include <limits>
#include <cmath>

#include <nlohmann/json.hpp>

namespace mino::external::json {

    using nj = nlohmann::json;
    using njj = nlohmann::json_pointer<std::string>; // 변경: 전방선언된 템플릿을 직접 사용

    // 내부 헬퍼 선언 (정의는 .cpp)
    const nj* get_node(const nj& j, const njj& ptr) noexcept;
    nj* get_node_mutable(nj& j, const njj& ptr) noexcept;

    // 숫자 변환 템플릿 선언(정의는 .cpp, 필요 타입 명시적 인스턴스화)
    template <class T>
    bool try_number_cast(const nj& node, T& out) noexcept;

    // value_or 오버로드 선언 (정의는 .cpp)
    std::string value_or(const nj& j, const njj& ptr, const std::string& defval) noexcept;
    bool value_or(const nj& j, const njj& ptr, bool defval) noexcept;

    template <class T>
    T value_or(const nj& j, const njj& ptr, T defval) noexcept;

    // 경로 문자열 버전 선언 (정의는 .cpp)
    std::string value_or_path(const nj& j, const std::string& path, const std::string& defval) noexcept;
    bool value_or_path(const nj& j, const std::string& path, bool defval) noexcept;

    template <class T>
    T value_or_path(const nj& j, const std::string& path, T defval) noexcept;

    //--------------------------------------------------------------
    // 외부 공개 API (예외 없음) - 기존대로 선언만
    //--------------------------------------------------------------
    bool  exists(const nj& j, const std::string& path) noexcept;

    std::string  get_string(const nj& j,
        const std::string& path,
        const std::string& defval) noexcept;

    int  get_int(const nj& j,
        const std::string& path,
        int defval) noexcept;

    bool  get_bool(const nj& j,
        const std::string& path,
        bool defval) noexcept;

    double  get_double(const nj& j,
        const std::string& path,
        double defval) noexcept;

}  
