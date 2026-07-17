#pragma once

#include <string>
#include <utility> // for std::move

// Macro to define a private member variable, its getter, and its setter
#define DEFINE_SETTER_GETTER(type, name)                                       \
private:                                                                       \
    type _##name{}; /* 값 초기화로 기본형도 0으로 초기화 */                      \
public:                                                                        \
    const type& name() const { return _##name; }                               \
    type& name() { return _##name; }                                           \
    auto& name(const type& value) { _##name = value; return *this; }           \
    auto& name(type&& value) { _##name = std::move(value); return *this; }
//
// Usage example:
// 
//  struct Config {
//     DEFINE_SETTER_GETTER(std::string, name)
//     DEFINE_SETTER_GETTER(int, width)
//     DEFINE_SETTER_GETTER(int, height)
//  };
// 
// Config cfg;
// 
// cfg.name("Main Window")
//      .width(1280)
//      .height(720); // Setter 
// 
// std::cout << "Name: " << cfg.name() << "\n"; // Getter 
// std::cout << "Width: " << cfg.width() << "\n";
// std::cout << "Fullscreen: " << std::boolalpha << cfg.fullscreen() << "\n";
// 
