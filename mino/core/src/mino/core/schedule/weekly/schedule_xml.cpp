#include <string>
#include <memory>

#include "mino/core/xml/xml.hpp"

#include "mino/core/schedule/weekly/schedule_xml.hpp"

namespace mino::core::schedule::weekly {

    std::string to_xml(const weekly_ranges& ranges) {
        static const char* day_name[] = { "Mon","Tue","Wed","Thu","Fri","Sat","Sun" };

        // build xml_node tree
        mino::core::xml::xml_node root;
        root.name = "WeeklySchedule";

        for (const auto& r : ranges) {
            auto child = std::make_unique<mino::core::xml::xml_node>();
            child->name = "Range";

            mino::core::xml::xml_attribute a1;
            a1.name = "start_day";
            a1.value = std::string(day_name[static_cast<int>(r.start_day)]);
            child->attributes.push_back(std::move(a1));

            mino::core::xml::xml_attribute a2;
            a2.name = "start_h";
            a2.value = std::to_string(r.start_time.hour);
            child->attributes.push_back(std::move(a2));

            mino::core::xml::xml_attribute a3;
            a3.name = "start_m";
            a3.value = std::to_string(r.start_time.minute);
            child->attributes.push_back(std::move(a3));

            mino::core::xml::xml_attribute a4;
            a4.name = "end_day";
            a4.value = std::string(day_name[static_cast<int>(r.end_day)]);
            child->attributes.push_back(std::move(a4));

            mino::core::xml::xml_attribute a5;
            a5.name = "end_h";
            a5.value = std::to_string(r.end_time.hour);
            child->attributes.push_back(std::move(a5));

            mino::core::xml::xml_attribute a6;
            a6.name = "end_m";
            a6.value = std::to_string(r.end_time.minute);
            child->attributes.push_back(std::move(a6));

            root.children.push_back(std::move(child));
        }

        return mino::core::xml::serialize_xml(root, true);
    }

}
