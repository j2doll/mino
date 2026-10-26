#include "mino/core/schedule/task/holiday_calendar.hpp"
#include "mino/core/json/value.hpp"
#include "mino/core/json/parser.hpp"
#include "mino/core/json/serializer.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace mino::core::schedule::task {

    namespace {
        // smart_enum의 enum_name, enum_cast를 활용한 변환 유틸리티
        std::string_view time_base_to_str(time_base base) {
            return mino::core::enums::enum_name(base);
        }

        time_base str_to_time_base(std::string_view str) {
            return mino::core::enums::enum_cast<time_base>(str).value_or(time_base::localtime);
        }

        std::string_view policy_to_str(holiday_policy p) {
            return mino::core::enums::enum_name(p);
        }

        holiday_policy str_to_policy(std::string_view str) {
            return mino::core::enums::enum_cast<holiday_policy>(str).value_or(holiday_policy::skip);
        }
    }

    holiday_calendar::holiday_calendar()
        : time_basis_(time_base::localtime), policy_(holiday_policy::skip) {
        for (int i = 0; i < 7; ++i) {
            weekend_holidays_[i] = false;
        }
        weekend_holidays_[static_cast<int>(weekday::saturday)] = true;
        weekend_holidays_[static_cast<int>(weekday::sunday)] = true;
    }

    holiday_calendar::holiday_calendar(time_base base, holiday_policy policy)
        : time_basis_(base), policy_(policy) {
        for (int i = 0; i < 7; ++i) {
            weekend_holidays_[i] = false;
        }
        weekend_holidays_[static_cast<int>(weekday::saturday)] = true;
        weekend_holidays_[static_cast<int>(weekday::sunday)] = true;
    }

    void holiday_calendar::set_time_base(time_base base) {
        time_basis_ = base;
    }

    time_base holiday_calendar::get_time_base() const {
        return time_basis_;
    }

    void holiday_calendar::set_policy(holiday_policy p) {
        policy_ = p;
    }

    holiday_policy holiday_calendar::get_policy() const {
        return policy_;
    }

    void holiday_calendar::set_weekend_holiday(weekday wd, bool is_holiday) {
        weekend_holidays_[static_cast<int>(wd)] = is_holiday;
    }

    bool holiday_calendar::is_weekend_holiday(weekday wd) const {
        return weekend_holidays_[static_cast<int>(wd)];
    }

    void holiday_calendar::add_recurring_holiday(int month, int day) {
        recurring_holidays_.insert({ month, day });
    }

    void holiday_calendar::remove_recurring_holiday(int month, int day) {
        recurring_holidays_.erase({ month, day });
    }

    void holiday_calendar::add_specific_holiday(int year, int month, int day) {
        specific_holidays_.insert({ year, month, day });
    }

    void holiday_calendar::remove_specific_holiday(int year, int month, int day) {
        specific_holidays_.erase({ year, month, day });
    }

    void holiday_calendar::add_specific_exclusion(int year, int month, int day) {
        specific_exclusions_.insert({ year, month, day });
    }

    void holiday_calendar::remove_specific_exclusion(int year, int month, int day) {
        specific_exclusions_.erase({ year, month, day });
    }

    void holiday_calendar::add_recurring_exclusion(int month, int day) {
        recurring_exclusions_.insert({ month, day });
    }

    void holiday_calendar::remove_recurring_exclusion(int month, int day) {
        recurring_exclusions_.erase({ month, day });
    }

    bool holiday_calendar::is_holiday(int year, int month, int day, weekday wd) const {
        // 우선순위 1: 1회성 제외일 -> 평일
        if (specific_exclusions_.find({ year, month, day }) != specific_exclusions_.end()) {
            return false;
        }
        // 우선순위 2: 매년 반복 제외일 -> 평일
        if (recurring_exclusions_.find({ month, day }) != recurring_exclusions_.end()) {
            return false;
        }
        // 우선순위 3: 1회성 공휴일 -> 공휴일
        if (specific_holidays_.find({ year, month, day }) != specific_holidays_.end()) {
            return true;
        }
        // 우선순위 4: 매년 반복 공휴일 -> 공휴일
        if (recurring_holidays_.find({ month, day }) != recurring_holidays_.end()) {
            return true;
        }
        // 우선순위 5: 주말 휴무 체크
        if (weekend_holidays_[static_cast<int>(wd)]) {
            return true;
        }
        return false;
    }

    bool holiday_calendar::is_holiday(const date_time_parts& parts) const {
        return is_holiday(parts.year, parts.month, parts.day, parts.weekday);
    }

    bool holiday_calendar::is_holiday(std::chrono::system_clock::time_point tp) const {
        auto parts = date_time_parts::from_time_point(tp, time_basis_);
        return is_holiday(parts);
    }

    std::string holiday_calendar::to_json_string(int indent) const {
        std::string ind1(indent, ' ');
        std::string ind2(indent * 2, ' ');

        std::ostringstream ss;
        ss << "{\n";
        ss << ind1 << "\"time_basis\": \"" << time_base_to_str(time_basis_) << "\",\n";
        ss << ind1 << "\"policy\": \"" << policy_to_str(policy_) << "\",\n";

        // 주말 설정
        ss << ind1 << "\"weekend_holidays\": [\n";
        bool first = true;
        for (int i = 0; i < 7; ++i) {
            if (weekend_holidays_[i]) {
                if (!first) ss << ",\n";
                first = false;
                ss << ind2 << "\"" << to_string(static_cast<weekday>(i)) << "\"";
            }
        }
        ss << "\n" << ind1 << "],\n";

        // recurring_holidays
        ss << ind1 << "\"recurring_holidays\": [\n";
        first = true;
        for (const auto& item : recurring_holidays_) {
            if (!first) ss << ",\n";
            first = false;
            ss << ind2 << "{ \"month\": " << item.first << ", \"day\": " << item.second << " }";
        }
        ss << "\n" << ind1 << "],\n";

        // specific_holidays
        ss << ind1 << "\"specific_holidays\": [\n";
        first = true;
        for (const auto& item : specific_holidays_) {
            if (!first) ss << ",\n";
            first = false;
            ss << ind2 << "{ \"year\": " << std::get<0>(item)
                << ", \"month\": " << std::get<1>(item)
                << ", \"day\": " << std::get<2>(item) << " }";
        }
        ss << "\n" << ind1 << "],\n";

        // recurring_exclusions
        ss << ind1 << "\"recurring_exclusions\": [\n";
        first = true;
        for (const auto& item : recurring_exclusions_) {
            if (!first) ss << ",\n";
            first = false;
            ss << ind2 << "{ \"month\": " << item.first << ", \"day\": " << item.second << " }";
        }
        ss << "\n" << ind1 << "],\n";

        // specific_exclusions
        ss << ind1 << "\"specific_exclusions\": [\n";
        first = true;
        for (const auto& item : specific_exclusions_) {
            if (!first) ss << ",\n";
            first = false;
            ss << ind2 << "{ \"year\": " << std::get<0>(item)
                << ", \"month\": " << std::get<1>(item)
                << ", \"day\": " << std::get<2>(item) << " }";
        }
        ss << "\n" << ind1 << "]\n";
        ss << "}\n";

        return ss.str();
    }

    bool holiday_calendar::to_json_file(const std::string& file_path, int indent) const {
        std::ofstream ofs(file_path, std::ios::binary);
        if (!ofs.is_open()) return false;
        std::string s = to_json_string(indent);
        ofs.write(s.data(), s.size());
        return true;
    }

    std::optional<holiday_calendar> holiday_calendar::from_json_string(std::string_view json_str) {
        if (json_str.empty()) return std::nullopt;

        auto parsed = mino::core::json::parser::parse(json_str);
        if (!parsed.is_object()) return std::nullopt;

        const auto& obj = parsed.get_object();
        holiday_calendar cal;

        // 초기화: 모든 요일을 false로 초기화 후 JSON 내용으로 설정
        for (int i = 0; i < 7; ++i) {
            cal.set_weekend_holiday(static_cast<weekday>(i), false);
        }

        if (auto it = obj.find("time_basis"); it != obj.end() && it->second.is_string()) {
            cal.set_time_base(str_to_time_base(it->second.get_string()));
        }
        if (auto it = obj.find("policy"); it != obj.end() && it->second.is_string()) {
            cal.set_policy(str_to_policy(it->second.get_string()));
        }
        if (auto it = obj.find("weekend_holidays"); it != obj.end() && it->second.is_array()) {
            for (const auto& item : it->second.get_array()) {
                if (item.is_string()) {
                    auto wd_opt = weekday_from_string(item.get_string());
                    if (wd_opt.has_value()) {
                        cal.set_weekend_holiday(*wd_opt, true);
                    }
                }
            }
        }
        if (auto it = obj.find("recurring_holidays"); it != obj.end() && it->second.is_array()) {
            for (const auto& item : it->second.get_array()) {
                if (item.is_object()) {
                    const auto& o = item.get_object();
                    auto m_it = o.find("month");
                    auto d_it = o.find("day");
                    if (m_it != o.end() && d_it != o.end()) {
                        cal.add_recurring_holiday(
                            static_cast<int>(m_it->second.get_number()),
                            static_cast<int>(d_it->second.get_number())
                        );
                    }
                }
            }
        }
        if (auto it = obj.find("specific_holidays"); it != obj.end() && it->second.is_array()) {
            for (const auto& item : it->second.get_array()) {
                if (item.is_object()) {
                    const auto& o = item.get_object();
                    auto y_it = o.find("year");
                    auto m_it = o.find("month");
                    auto d_it = o.find("day");
                    if (y_it != o.end() && m_it != o.end() && d_it != o.end()) {
                        cal.add_specific_holiday(
                            static_cast<int>(y_it->second.get_number()),
                            static_cast<int>(m_it->second.get_number()),
                            static_cast<int>(d_it->second.get_number())
                        );
                    }
                }
            }
        }
        if (auto it = obj.find("recurring_exclusions"); it != obj.end() && it->second.is_array()) {
            for (const auto& item : it->second.get_array()) {
                if (item.is_object()) {
                    const auto& o = item.get_object();
                    auto m_it = o.find("month");
                    auto d_it = o.find("day");
                    if (m_it != o.end() && d_it != o.end()) {
                        cal.add_recurring_exclusion(
                            static_cast<int>(m_it->second.get_number()),
                            static_cast<int>(d_it->second.get_number())
                        );
                    }
                }
            }
        }
        if (auto it = obj.find("specific_exclusions"); it != obj.end() && it->second.is_array()) {
            for (const auto& item : it->second.get_array()) {
                if (item.is_object()) {
                    const auto& o = item.get_object();
                    auto y_it = o.find("year");
                    auto m_it = o.find("month");
                    auto d_it = o.find("day");
                    if (y_it != o.end() && m_it != o.end() && d_it != o.end()) {
                        cal.add_specific_exclusion(
                            static_cast<int>(y_it->second.get_number()),
                            static_cast<int>(m_it->second.get_number()),
                            static_cast<int>(d_it->second.get_number())
                        );
                    }
                }
            }
        }

        return cal;
    }

    std::optional<holiday_calendar> holiday_calendar::from_json_file(const std::string& file_path) {
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs.is_open()) return std::nullopt;

        ifs.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        if (size == 0) return std::nullopt;

        std::string buf(size, '\0');
        ifs.read(&buf[0], size);
        return from_json_string(buf);
    }

}
