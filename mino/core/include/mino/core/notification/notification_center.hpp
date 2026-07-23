#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <algorithm>

namespace mino::core::notification {

    // 알림 데이터 기반 클래스
    class message {
    public:
        virtual ~message() = default;
    };

    // 전체 프로세스에서 단 1개만 유지되는 알림 중재 센터
    class center {
    public:
        // 싱글톤 인스턴스 반환 (C++11 규격에 의해 멀티스레드 환경에서도 안전하게 1개만 생성됨)
        static center& default_center() {
            static center instance;
            return instance;
        }

        // 콜백 함수 타입 정의
        using callback_t = std::function<void(const std::shared_ptr<message>&)>;

        // 옵저버 등록을 위한 고유 ID 타입
        using observer_id_t = uint64_t;

        // 알림 구독 (옵저버 등록)
        observer_id_t add_observer(const std::string& name, callback_t callback) {
            std::lock_guard<std::mutex> lock(m_mutex);

            observer_id_t id = m_next_id++;
            m_observers[name].push_back({ id, std::move(callback) });
            return id;
        }

        // 알림 구독 취소 (옵저버 제거)
        void remove_observer(const std::string& name, observer_id_t id) {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_observers.find(name);
            if (it != m_observers.end()) {
                auto& list = it->second;
                list.erase(std::remove_if(list.begin(), list.end(),
                    [id](const observer_entry& entry) { return entry.id == id; }),
                    list.end());

                if (list.empty()) {
                    m_observers.erase(it);
                }
            }
        }

        // 알림 발송 (Broadcast)
        void post_notification(const std::string& name, const std::shared_ptr<message>& p_message) {
            std::vector<callback_t> callbacks_to_call;

            // 락 범위를 최소화하여 콜백 목록만 복사 후 즉시 해제
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_observers.find(name);
                if (it != m_observers.end()) {
                    for (const auto& entry : it->second) {
                        callbacks_to_call.push_back(entry.callback);
                    }
                }
            }

            // 락이 해제된 상태에서 콜백을 실행하므로 콜백 내부에서 add/remove가 일어나도 데드락이 발생하지 않음
            for (const auto& callback : callbacks_to_call) {
                if (callback) {
                    callback(p_message);
                }
            }
        }

    private:
        // 외부에서 인스턴스를 직접 생성하거나 복사하지 못하도록 생성자 차단 (싱글톤 보장)
        center() = default;
        ~center() = default;
        center(const center&) = delete;
        center& operator=(const center&) = delete;

        struct observer_entry {
            observer_id_t id;
            callback_t callback;
        };

        std::unordered_map<std::string, std::vector<observer_entry>> m_observers;
        observer_id_t m_next_id = 1;
        std::mutex m_mutex;
    };

}  