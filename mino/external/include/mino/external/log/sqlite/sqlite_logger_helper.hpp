#pragma once

#include "mino/external/log/sqlite/sqlite_logger.hpp"

#include "mino/core/datetime/unit/date_time.hpp"
#include "mino/core/string/u8/u8string.hpp"
#include "mino/core/string/mutex_string.hpp"

#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <utility>
#include <cctype>
#include <ostream>


namespace mino::external::log::sqlite {

    // --- 사용자 정의 타입(time_span, date_time, u8str, mutex_string) 출력용 operator<< ---

    inline std::ostream& operator<<(std::ostream& os, const mino::core::datetime::unit::time_span& ts) {
        os << ts.total_msecs() << "ms";
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const mino::core::datetime::unit::date_time& dt) {
        os << dt.to_string();
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const mino::core::string::u8::u8str& u8s) {
        os << u8s.to_std_string();
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const mino::core::string::mutex_string& ms) {
        os << ms.str();
        return os;
    }


    // --- 표준 컨테이너 및 페어(Pair) 출력용 operator<< ---

    template <typename T1, typename T2>
    inline std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2>& p) {
        os << "(" << p.first << ", " << p.second << ")";
        return os;
    }

    template <typename T, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::vector<T, Alloc>& c) {
        os << "[";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << *it;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "]";
        return os;
    }

    template <typename T, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::list<T, Alloc>& c) {
        os << "[";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << *it;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "]";
        return os;
    }

    template <typename T, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::deque<T, Alloc>& c) {
        os << "[";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << *it;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "]";
        return os;
    }

    template <typename K, typename Comp, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::set<K, Comp, Alloc>& c) {
        os << "[";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << *it;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "]";
        return os;
    }

    template <typename K, typename Hash, typename Pred, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::unordered_set<K, Hash, Pred, Alloc>& c) {
        os << "[";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << *it;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "]";
        return os;
    }

    template <typename K, typename V, typename Comp, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::map<K, V, Comp, Alloc>& c) {
        os << "{";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << it->first << ": " << it->second;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "}";
        return os;
    }

    template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
    inline std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V, Hash, Pred, Alloc>& c) {
        os << "{";
        for (auto it = c.begin(); it != c.end(); ++it) {
            os << it->first << ": " << it->second;
            if (std::next(it) != c.end()) os << ", ";
        }
        os << "}";
        return os;
    }

    template <typename T, typename Container>
    inline std::ostream& operator<<(std::ostream& os, std::queue<T, Container> q) {
        os << "[";
        bool first = true;
        while (!q.empty()) {
            if (!first) os << ", ";
            os << q.front();
            q.pop();
            first = false;
        }
        os << "]";
        return os;
    }

    template <typename T, typename Container>
    inline std::ostream& operator<<(std::ostream& os, std::stack<T, Container> s) {
        os << "[";
        bool first = true;
        while (!s.empty()) {
            if (!first) os << ", ";
            os << s.top();
            s.pop();
            first = false;
        }
        os << "]";
        return os;
    }

    template <typename T, typename Container, typename Compare>
    inline std::ostream& operator<<(std::ostream& os, std::priority_queue<T, Container, Compare> pq) {
        os << "[";
        bool first = true;
        while (!pq.empty()) {
            if (!first) os << ", ";
            os << pq.top();
            pq.pop();
            first = false;
        }
        os << "]";
        return os;
    }


    // --- 문자열 변환 및 포맷터 함수 ---

    template <typename T>
    inline std::string to_string_impl(T&& val) {
        std::ostringstream oss;
        oss << std::forward<T>(val);
        return oss.str();
    }

    template <typename... Args>
    inline std::string format_string(const std::string& fmt, Args&&... args) {
        std::vector<std::string> arg_list = { to_string_impl(std::forward<Args>(args))... };
        std::string result;
        size_t sequential_index = 0;
        size_t i = 0;

        while (i < fmt.size()) {
            if (fmt[i] == '{') {
                size_t j = i + 1;
                while (j < fmt.size() && fmt[j] != '}') {
                    j++;
                }
                if (j < fmt.size() && fmt[j] == '}') {
                    std::string content = fmt.substr(i + 1, j - i - 1);
                    if (content.empty()) {
                        if (sequential_index < arg_list.size()) {
                            result += arg_list[sequential_index++];
                        }
                        else {
                            result += "{}";
                        }
                    }
                    else {
                        bool is_number = true;
                        for (char c : content) {
                            if (!std::isdigit(static_cast<unsigned char>(c))) {
                                is_number = false;
                                break;
                            }
                        }
                        if (is_number) {
                            size_t idx = std::stoul(content);
                            if (idx < arg_list.size()) {
                                result += arg_list[idx];
                            }
                            else {
                                result += "{" + content + "}";
                            }
                        }
                        else {
                            result += "{" + content + "}";
                        }
                    }
                    i = j + 1;
                    continue;
                }
            }
            result += fmt[i++];
        }
        return result;
    }


    // --- 네임스페이스 내 레벨별 sqlog_* 헬퍼 인라인 함수들 (std::optional<std::string> 반환) ---

    template <typename... Args>
    inline std::optional<std::string> sqlog_trace(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_trace(table_name, msg);
    }

    template <typename... Args>
    inline std::optional<std::string> sqlog_debug(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_debug(table_name, msg);
    }

    template <typename... Args>
    inline std::optional<std::string> sqlog_info(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_info(table_name, msg);
    }

    template <typename... Args>
    inline std::optional<std::string> sqlog_warning(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_warning(table_name, msg);
    }

    template <typename... Args>
    inline std::optional<std::string> sqlog_error(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_error(table_name, msg);
    }

    template <typename... Args>
    inline std::optional<std::string> sqlog_fatal(const std::string& table_name, const std::string& fmt_str, Args&&... args) {
        std::string msg = format_string(fmt_str, std::forward<Args>(args)...);
        return sqlite_logger::get_instance().write_log_fatal(table_name, msg);
    }

} // namespace mino::external::log::sqlite
