#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <vector>

// 싱글톤 래퍼 템플릿 
namespace mino::core::singleton {

    template <typename T>
    class singleton_wrapper {
    public:
        // 싱글톤 인스턴스를 반환하는 static 메서드
        static T& get_instance() {
            static T instance;
            return instance;
        }

        // 래퍼 클래스 자체는 인스턴스화할 수 없도록 구조적 차단
        singleton_wrapper() = delete;
        ~singleton_wrapper() = delete;
        singleton_wrapper(const singleton_wrapper&) = delete;
        singleton_wrapper& operator=(const singleton_wrapper&) = delete;
    };

} 