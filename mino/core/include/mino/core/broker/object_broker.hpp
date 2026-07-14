#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <optional>
#include <vector>
#include <unordered_set>
#include <algorithm>
 
namespace mino::core::broker {

    // 객체 중개자
    class  object_broker {
    public:

        // 내부 키 구조체
        struct key {
            std::type_index type;
            std::string name;

            bool operator==(const key& other) const {
                return type == other.type && name == other.name;
            }
        };

        // unordered_map을 위한 커스텀 해시 계산기
        struct key_hash {
            std::size_t operator()(const key& k) const {
                return k.type.hash_code() ^ (std::hash<std::string>{}(k.name) << 1);
            }
        };

    public:
        /**
         * @brief 기본 인스턴스 등록
         * @tparam T 등록할 객체의 타입
         * @param instance 공유할 객체의 shared_ptr
         */
        template <typename T>
        static void register_instance(std::shared_ptr<T> instance) {
            register_instance<T>("__default__", instance);
        }

        /**
         * @brief 이름을 지정하여 인스턴스 등록
         * @tparam T 등록할 객체의 타입
         * @param name 객체 식별을 위한 고유 별칭
         * @param instance 공유할 객체의 shared_ptr
         */
        template <typename T>
        static void register_instance(const std::string& name, std::shared_ptr<T> instance) {
            std::unique_lock lock(get_instance().mutex_);
            get_instance().storage_[{typeid(T), name}] = instance;
        }

        /**
         * @brief 등록된 인스턴스 조회
         * @tparam T 가져올 객체의 타입
         * @param name 조회할 객체의 별칭 (기본값 사용 시 생략 가능)
         * @return std::shared_ptr<T> 조회된 객체 (없을 경우 nullptr 반환)
         */
        template <typename T>
        static std::shared_ptr<T> get(const std::string& name = "__default__") {
            std::shared_lock lock(get_instance().mutex_);
            auto& s = get_instance().storage_;

            auto it = s.find({ typeid(T), name });
            if (it != s.end()) {
                try {
                    return std::any_cast<std::shared_ptr<T>>(it->second);
                }
                catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        /**
         * @brief 특정 인스턴스가 등록되어 있는지 확인
         */
        template <typename T>
        static bool contains(const std::string& name = "__default__") {
            std::shared_lock lock(get_instance().mutex_);
            auto& s = get_instance().storage_;
            return s.find({ typeid(T), name }) != s.end();
        }

        /**
         * @brief 실패 시 nullptr 대신 optional 반환 (C++17)
         */
        template <typename T>
        static std::optional<std::shared_ptr<T>> get_optional(const std::string& name = "__default__") {
            std::shared_lock lock(get_instance().mutex_);
            auto& s = get_instance().storage_;
            auto it = s.find({ typeid(T), name });
            if (it != s.end()) {
                try {
                    return std::make_optional(std::any_cast<std::shared_ptr<T>>(it->second));
                }
                catch (const std::bad_any_cast&) {
                    return std::nullopt;
                }
            }
            return std::nullopt;
        }

        /**
         * @brief 특정 인스턴스 제거
         */
        template <typename T>
        static void unregister_instance(const std::string& name = "__default__") {
            std::unique_lock lock(get_instance().mutex_);
            get_instance().storage_.erase({ typeid(T), name });
        }

        // 모든 중개 목록 초기화
        static void clear();

        /**
         * @brief 특정 타입에 등록된 모든 인스턴스(포인터만) 반환
         * 기존 호환성을 유지하기 위한 보존형 API
         */
        template <typename T>
        static std::vector<std::shared_ptr<T>> get_all() {
            std::vector<std::shared_ptr<T>> result;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                if (entry.first.type == typeid(T)) {
                    try {
                        auto sp = std::any_cast<std::shared_ptr<T>>(entry.second);
                        if (sp) result.push_back(sp);
                    }
                    catch (const std::bad_any_cast&) {
                        // 무시
                    }
                }
            }
            return result;
        }

        /**
         * @brief 특정 타입에 등록된 모든 인스턴스와 이름을 반환
         * 반환값: vector of (name, shared_ptr<T>)
         */
        template <typename T>
        static std::vector<std::pair<std::string, std::shared_ptr<T>>> get_all_with_names() {
            std::vector<std::pair<std::string, std::shared_ptr<T>>> result;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                if (entry.first.type == typeid(T)) {
                    try {
                        auto sp = std::any_cast<std::shared_ptr<T>>(entry.second);
                        if (sp) result.emplace_back(entry.first.name, sp);
                    }
                    catch (const std::bad_any_cast&) {
                        // 무시
                    }
                }
            }
            return result;
        }

        /**
         * @brief 특정 타입에 등록된 이름 목록만 반환
         */
        template <typename T>
        static std::vector<std::string> list_names_for_type() {
            std::vector<std::string> names;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                if (entry.first.type == typeid(T)) {
                    names.push_back(entry.first.name);
                }
            }
            return names;
        }

        /**
         * @brief 전체 등록 엔트리에서 이름만 모두 반환 (중복 포함)
         */
        static std::vector<std::string> list_all_names() {
            std::vector<std::string> names;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                names.push_back(entry.first.name);
            }
            return names;
        }

        /**
         * @brief 전체 등록 엔트리에서 이름을 중복 제거하여 반환
         */
        static std::vector<std::string> list_unique_names() {
            std::unordered_set<std::string> set;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                set.insert(entry.first.name);
            }
            return std::vector<std::string>(set.begin(), set.end());
        }

        /**
         * @brief 전체 등록 엔트리를 (type_index, name) 쌍으로 반환
         * 타입 정보를 통해 어떤 타입에 등록되었는지 확인할 수 있습니다.
         */
        static std::vector<std::pair<std::type_index, std::string>> list_all_entries() {
            std::vector<std::pair<std::type_index, std::string>> entries;
            std::shared_lock lock(get_instance().mutex_);
            for (const auto& entry : get_instance().storage_) {
                entries.emplace_back(entry.first.type, entry.first.name);
            }
            return entries;
        }

        /**
         * @brief 스코프 기반 자동 등록/해제 헬퍼
         * 사용 예:
         *   {
         *       object_broker::registration_guard<Foo> g("temp", ptr);
         *       // 스코프 종료 시 자동으로 unregister
         *   }
         */
        template <typename T>
        struct registration_guard {
            registration_guard(const std::string& name, std::shared_ptr<T> instance)
                : name_(name) {
                object_broker::register_instance<T>(name_, instance);
            }
            explicit registration_guard(std::shared_ptr<T> instance)
                : registration_guard("__default__", instance) {}
            ~registration_guard() {
                object_broker::unregister_instance<T>(name_);
            }
        private:
            std::string name_;
        };

    protected:
        object_broker() = default;
        ~object_broker() = default;

        // 싱글톤 속성 유지를 위해 복사 및 이동 제한
        object_broker(const object_broker&) = delete;
        object_broker& operator=(const object_broker&) = delete;

        // 내부 싱글톤 인스턴스 반환
        static object_broker& get_instance();

        std::unordered_map<key, std::any, key_hash> storage_;
        mutable std::shared_mutex mutex_;
    };

}
