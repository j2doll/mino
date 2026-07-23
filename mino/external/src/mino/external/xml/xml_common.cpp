#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "mino/core/string/encoding_function.hpp"

#include "mino/external/xml/xml_common.hpp"
#include "mino/external/xml/xml_parser.hpp"

#include "mino/external/log/spd/auto_color_sink.hpp" 

namespace mino::external::xml
{
    // -----------------------------
    // 인코딩 탐지/변환
    // -----------------------------
    std::string detect_xml_encoding(
        const std::string& raw_xml
    )
    {
        // 간단히 XML 선언에서 encoding="..." 을 추출한다. 없으면 UTF-8 반환.
        auto pos = raw_xml.find("<?xml");
        if (pos == std::string::npos)
            return "UTF-8";

        auto decl_end = raw_xml.find("?>", pos);
        if (decl_end == std::string::npos)
            decl_end = raw_xml.size();

        auto enc_pos = raw_xml.find("encoding", pos);
        if (enc_pos == std::string::npos || enc_pos > decl_end)
            return "UTF-8";

        auto eq = raw_xml.find('=', enc_pos);
        if (eq == std::string::npos || eq > decl_end)
            return "UTF-8";

        // skip whitespace
        std::size_t i = eq + 1;
        while (i < raw_xml.size() && std::isspace(static_cast<unsigned char>(raw_xml[i])))
            ++i;
        if (i >= raw_xml.size())
            return "UTF-8";

        char quote = raw_xml[i];
        if (quote != '"' && quote != '\'')
            return "UTF-8";

        ++i;
        auto start = i;
        while (i < raw_xml.size() && raw_xml[i] != quote)
            ++i;
        if (i >= raw_xml.size())
            return "UTF-8";

        return raw_xml.substr(start, i - start);
    }

    std::string convert_text_encoding_to_utf8(
        const std::string& input,
        const std::string& encoding
    )
    {
        // encoding 비교를 소문자 기준으로 수행
        std::string enc_lower;
        enc_lower.reserve(encoding.size());
        for (char ch : encoding)
            enc_lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));

        if (enc_lower.empty() || enc_lower == "utf-8" || enc_lower == "utf8")
        {
            return input;
        }

        std::string out;
        // 흔한 레거시 인코딩 처리: CP949 / EUC-KR / ISO-2022-KR
        if (enc_lower == "cp949" || enc_lower == "ms949" || enc_lower == "euc-kr" || enc_lower == "euc_kr")
        {
            if (mino::core::string::cp949_to_utf8(input, out))
                return out;
            throw std::runtime_error("Encoding conversion failed: " + encoding);
        }
        if (enc_lower == "iso-2022-kr" || enc_lower == "iso2022-kr" || enc_lower == "iso-2022-kr")
        {
            if (mino::core::string::iso2022kr_to_utf8(input, out))
                return out;
            throw std::runtime_error("Encoding conversion failed: " + encoding);
        }

        // 기타 인코딩은 우선 입력을 그대로 반환(보수적 처리).
        // 필요시 iconv/Windows API로 확장 가능.
        return input;
    }

    std::string convert_xml_to_utf8(
        const std::string& raw_xml
    )
    {
        std::string enc = detect_xml_encoding(raw_xml);
        try
        {
            return convert_text_encoding_to_utf8(raw_xml, enc);
        }
        catch (...)
        {
            // 변환 실패 시 원본을 반환하거나 예외를 올릴 수 있음.
            // 여기서는 호출자가 예외를 원하면 convert_text_encoding_to_utf8에서 던진다.
            throw;
        }
    }

    // -----------------------------
    // 공용 공백 제거 함수
    // -----------------------------
    std::string trim_spaces(const std::string& s)
    {
        std::size_t begin = 0;
        while (begin < s.size() &&
            std::isspace(static_cast<unsigned char>(s[begin])))
        {
            ++begin;
        }
        if (begin == s.size())
            return {};

        std::size_t end = s.size();
        while (end > begin &&
            std::isspace(static_cast<unsigned char>(s[end - 1])))
        {
            --end;
        }
        return s.substr(begin, end - begin);
    }

    // -----------------------------
    // xml_node 멤버
    // -----------------------------
    xml_node* xml_node::find_child(const std::string& child_name)
    {
        for (auto& c : children)
        {
            if (c->name == child_name)
                return c.get();
        }
        return nullptr;
    }

    const xml_attribute* xml_node::find_attribute(const std::string& attr_name) const
    {
        for (const auto& a : attributes)
        {
            if (a.name == attr_name)
                return &a;
        }
        return nullptr;
    }

    // -----------------------------
    // SAX 기본 구현 및 트리 순회
    // -----------------------------
    sax_handler::~sax_handler() = default;

    void sax_handler::on_start_element(const xml_node& node)
    {
        (void)node;
    }

    void sax_handler::on_end_element(const xml_node& node)
    {
        (void)node;
    }

    void sax_handler::on_text(const std::string& text)
    {
        (void)text;
    }

    void traverse_sax(const xml_node& node, sax_handler& handler)
    {
        handler.on_start_element(node);

        if (!node.text.empty())
        {
            handler.on_text(node.text);
        }

        for (const auto& child : node.children)
        {
            traverse_sax(*child, handler);
        }

        handler.on_end_element(node);
    }

    // -----------------------------
    // 트리 출력
    // -----------------------------
    void print_xml_tree(const xml_node& node, int indent)
    {
        auto console_sink = std::make_shared<mino::external::log::spd::auto_color_sink<std::mutex>>();
        std::vector<spdlog::sink_ptr> sinks{ console_sink };
        auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

        std::string indent_str(static_cast<std::size_t>(indent) * 2, ' ');

        logger->debug("{}<", indent_str);
        if (!node.prefix.empty())
            logger->debug("{}:", node.prefix);
        logger->debug("{}", node.name);

        for (const auto& attr : node.attributes)
        {
            logger->debug(" ");
            if (!attr.prefix.empty())
                logger->debug("{}:", attr.prefix);
            logger->debug("{}=\"{}\"", attr.name, attr.value);
        }

        if (node.children.empty() && node.text.empty())
        {
            logger->debug(" />");
            return;
        }

        logger->debug(">");
        if (!node.text.empty())
            logger->debug("{}", node.text);

        if (!node.children.empty())
            logger->debug("\n");

        for (const auto& child : node.children)
        {
            print_xml_tree(*child, indent + 1);
        }

        if (!node.children.empty())
            logger->debug("{}", indent_str);

        logger->debug("</");
        if (!node.prefix.empty())
            logger->debug("{}:", node.prefix);
        logger->debug("{}>\n", node.name);
    }

    // -----------------------------
    // XPath 유사 기능
    // -----------------------------
    xpath_step parse_xpath_step(const std::string& step_str)
    {
        xpath_step step;

        auto lb = step_str.find('[');
        if (lb == std::string::npos)
        {
            step.name = step_str;
            return step;
        }

        step.name = step_str.substr(0, lb);
        auto rb = step_str.find(']', lb);
        if (rb == std::string::npos)
            throw std::runtime_error("XPath step bracket is not closed.");

        std::string cond = step_str.substr(lb + 1, rb - lb - 1);
        cond.erase(std::remove_if(cond.begin(), cond.end(), ::isspace), cond.end());

        if (cond.size() < 5 || cond[0] != '@')
            throw std::runtime_error("Unsupported XPath condition: " + cond);

        auto eq_pos = cond.find('=');
        if (eq_pos == std::string::npos)
            throw std::runtime_error("XPath condition missing '=': " + cond);

        step.attr_name = cond.substr(1, eq_pos - 1);

        char quote = cond[eq_pos + 1];
        if (quote != '"' && quote != '\'')
            throw std::runtime_error("XPath condition value must be quoted: " + cond);

        auto q2 = cond.find(quote, eq_pos + 2);
        if (q2 == std::string::npos)
            throw std::runtime_error("XPath condition value is not closed: " + cond);

        step.attr_value = cond.substr(eq_pos + 2, q2 - (eq_pos + 2));

        return step;
    }

    std::vector<xpath_step> parse_xpath(const std::string& expr)
    {
        std::vector<xpath_step> steps;

        std::string tmp;
        for (char ch : expr)
        {
            if (ch == '/')
            {
                if (!tmp.empty())
                {
                    steps.push_back(parse_xpath_step(tmp));
                    tmp.clear();
                }
            }
            else
            {
                tmp.push_back(ch);
            }
        }

        if (!tmp.empty())
            steps.push_back(parse_xpath_step(tmp));

        return steps;
    }

    std::vector<xml_node*> match_step_in_children(xml_node* parent, const xpath_step& step)
    {
        std::vector<xml_node*> result;

        for (auto& child_uptr : parent->children)
        {
            xml_node* child = child_uptr.get();
            if (child->name != step.name)
                continue;

            if (!step.attr_name.empty())
            {
                const xml_attribute* attr = child->find_attribute(step.attr_name);
                if (!attr || attr->value != step.attr_value)
                    continue;
            }

            result.push_back(child);
        }

        return result;
    }

    std::vector<xml_node*> xpath_select(xml_node* root, const std::string& expr)
    {
        std::string trimmed = trim_spaces(expr);

        // 선행 '/' 제거
        while (!trimmed.empty() && trimmed.front() == '/')
            trimmed.erase(trimmed.begin());

        if (trimmed.empty())
            return { root };

        auto steps = parse_xpath(trimmed);

        std::vector<xml_node*> current;
        current.push_back(root);

        // 첫 step이 루트 노드 이름과 같으면 건너뜀
        std::size_t start_index = 0;
        if (!steps.empty() && steps[0].name == root->name)
        {
            start_index = 1;
        }

        for (std::size_t i = start_index; i < steps.size(); ++i)
        {
            const auto& step = steps[i];
            std::vector<xml_node*> next;

            for (auto* node : current)
            {
                auto matched = match_step_in_children(node, step);
                next.insert(next.end(), matched.begin(), matched.end());
            }

            current.swap(next);
            if (current.empty())
                break;
        }

        return current;
    }

    // -----------------------------
    // debug_sax_handler
    // -----------------------------
    void debug_sax_handler::on_start_element(const xml_node& node)
    {
        auto console_sink = std::make_shared<mino::external::log::spd::auto_color_sink<std::mutex>>();
        std::vector<spdlog::sink_ptr> sinks{ console_sink };
        auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

        logger->debug("[SAX] START: ");
        if (!node.prefix.empty())
            logger->debug("{}:", node.prefix);
        logger->debug("{}", node.name);
    }

    void debug_sax_handler::on_end_element(const xml_node& node)
    {
        auto console_sink = std::make_shared<mino::external::log::spd::auto_color_sink<std::mutex>>();
        std::vector<spdlog::sink_ptr> sinks{ console_sink };
        auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

        logger->debug("[SAX] END  : ");
        if (!node.prefix.empty())
            logger->debug("{}:", node.prefix);
        logger->debug("{}", node.name);
    }

    void debug_sax_handler::on_text(const std::string& text)
    {
        auto console_sink = std::make_shared<mino::external::log::spd::auto_color_sink<std::mutex>>();
        std::vector<spdlog::sink_ptr> sinks{ console_sink };
        auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

        std::string trimmed = trim_spaces(text);
        if (!trimmed.empty())
        {
            // std::cout << "[SAX] TEXT : " << trimmed << "\n";
            logger->debug("[SAX] TEXT : {}", trimmed);
        }
    }

    // -----------------------------
    // xml_file_loader
    // -----------------------------
    std::string xml_file_loader::read_file_binary(const std::string& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
            throw std::runtime_error("Unable to open file: " + path);

        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    }

    std::unique_ptr<xml_node> xml_file_loader::parse_file_with_auto_encoding(
        const std::string& path,
        text_policy policy)
    {
        std::string raw_xml = read_file_binary(path);
        return parse_with_auto_encoding(raw_xml, policy);
    }

    // -----------------------------
    // 속성/텍스트 경로 헬퍼 및 변환 유틸
    // -----------------------------
    std::string get_attr_value_by_path(xml_node* root,
        const std::string& full_path)
    {
        if (!root)
            return {};

        std::string path = trim_spaces(full_path);

        while (!path.empty() && path.front() == '/')
            path.erase(path.begin());

        if (path.empty())
            return {};

        std::string node_path;
        std::string attr_part;

        auto pos = path.rfind('/');
        if (pos == std::string::npos)
        {
            node_path = "";
            attr_part = path;
        }
        else
        {
            node_path = path.substr(0, pos);
            attr_part = path.substr(pos + 1);
        }

        attr_part = trim_spaces(attr_part);
        if (attr_part.empty() || attr_part[0] != '@')
            return {};

        std::string attr_name = attr_part.substr(1);
        if (attr_name.empty())
            return {};

        xml_node* target_node = nullptr;

        if (node_path.empty())
        {
            target_node = root;
        }
        else
        {
            auto nodes = xpath_select(root, node_path);
            if (nodes.empty())
                return {};
            target_node = nodes.front();
        }

        const xml_attribute* attr = target_node->find_attribute(attr_name);
        if (!attr)
            return {};

        return attr->value;
    }

    std::optional<int> get_attr_int_by_path(xml_node* root,
        const std::string& full_path)
    {
        std::string value = get_attr_value_by_path(root, full_path);
        if (value.empty())
            return std::nullopt;

        try
        {
            int v = std::stoi(value);
            return std::optional<int>(v);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<bool> get_attr_bool_by_path(xml_node* root,
        const std::string& full_path)
    {
        std::string value = get_attr_value_by_path(root, full_path);
        if (value.empty())
            return std::nullopt;

        std::string lower;
        lower.reserve(value.size());
        for (char ch : value)
        {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
            return std::optional<bool>(true);
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
            return std::optional<bool>(false);

        return std::nullopt;
    }

    static bool is_attr_path(const std::string& path)
    {
        std::string trimmed = trim_spaces(path);
        if (trimmed.empty())
            return false;

        std::string tmp = trimmed;
        while (!tmp.empty() && tmp.front() == '/')
            tmp.erase(tmp.begin());

        if (tmp.empty())
            return false;

        auto pos = tmp.rfind('/');
        std::string last = (pos == std::string::npos) ? tmp : tmp.substr(pos + 1);
        last = trim_spaces(last);

        return (!last.empty() && last[0] == '@');
    }

    std::string get_text_by_path(xml_node* root,
        const std::string& path)
    {
        if (!root)
            return {};

        if (is_attr_path(path))
        {
            return get_attr_value_by_path(root, path);
        }

        auto nodes = xpath_select(root, path);
        if (nodes.empty())
            return {};

        return nodes.front()->text;
    }

    std::optional<std::string> get_text_opt_by_path(xml_node* root,
        const std::string& path)
    {
        if (!root)
            return std::nullopt;

        std::string v = get_text_by_path(root, path);
        if (v.empty())
            return std::nullopt;

        return std::optional<std::string>(v);
    }

    std::optional<int> get_int_by_path(xml_node* root,
        const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        try
        {
            int v = std::stoi(*opt_text);
            return std::optional<int>(v);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<bool> get_bool_by_path(xml_node* root,
        const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        std::string lower;
        lower.reserve(opt_text->size());
        for (char ch : *opt_text)
        {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
            return std::optional<bool>(true);
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
            return std::optional<bool>(false);

        return std::nullopt;
    }

    std::optional<double> get_double_by_path(xml_node* root,
        const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        try
        {
            double v = std::stod(*opt_text);
            return std::optional<double>(v);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<std::chrono::system_clock::time_point>
        parse_iso8601_datetime(const std::string& text)
    {
        std::string s = trim_spaces(text);
        if (s.empty())
            return std::nullopt;

        if (s.size() < 10)
            return std::nullopt;

        if (s[4] != '-' || s[7] != '-')
            return std::nullopt;

        int year = 0;
        int month = 0;
        int day = 0;

        try
        {
            year = std::stoi(s.substr(0, 4));
            month = std::stoi(s.substr(5, 2));
            day = std::stoi(s.substr(8, 2));
        }
        catch (...)
        {
            return std::nullopt;
        }

        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        std::size_t pos = 10;

        if (pos >= s.size())
        {
            std::time_t tt = std::mktime(&tm);
            if (tt == static_cast<std::time_t>(-1))
                return std::nullopt;
            auto tp = std::chrono::system_clock::from_time_t(tt);
            return std::optional<std::chrono::system_clock::time_point>(tp);
        }

        if (s[pos] == 'T' || s[pos] == 't' || s[pos] == ' ')
        {
            ++pos;
        }
        else
        {
            return std::nullopt;
        }

        if (pos + 8 > s.size())
            return std::nullopt;

        if (s[pos + 2] != ':' || s[pos + 5] != ':')
            return std::nullopt;

        int hour = 0;
        int min = 0;
        int sec = 0;

        try
        {
            hour = std::stoi(s.substr(pos, 2));
            min = std::stoi(s.substr(pos + 3, 2));
            sec = std::stoi(s.substr(pos + 6, 2));
        }
        catch (...)
        {
            return std::nullopt;
        }

        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;

        pos += 8;

        long long fractional_ns = 0;
        if (pos < s.size() && s[pos] == '.')
        {
            ++pos;
            std::size_t start_frac = pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos])))
            {
                ++pos;
            }

            std::size_t frac_len = pos - start_frac;
            if (frac_len > 0)
            {
                std::string frac_str = s.substr(start_frac, frac_len);
                if (frac_len > 9)
                    frac_str = frac_str.substr(0, 9);
                frac_len = frac_str.size();

                try
                {
                    long long frac_val = std::stoll(frac_str);
                    for (std::size_t i = frac_len; i < 9; ++i)
                        frac_val *= 10;
                    fractional_ns = frac_val;
                }
                catch (...)
                {
                    fractional_ns = 0;
                }
            }
        }

        if (pos < s.size())
        {
            char tz_ch = s[pos];
            if (tz_ch == 'Z' || tz_ch == 'z')
            {
                ++pos;
            }
            else if (tz_ch == '+' || tz_ch == '-')
            {
                ++pos;
                if (pos + 2 <= s.size() &&
                    std::isdigit(static_cast<unsigned char>(s[pos])) &&
                    std::isdigit(static_cast<unsigned char>(s[pos + 1])))
                {
                    pos += 2;
                }
                if (pos < s.size() && s[pos] == ':')
                    ++pos;
                if (pos + 2 <= s.size() &&
                    std::isdigit(static_cast<unsigned char>(s[pos])) &&
                    std::isdigit(static_cast<unsigned char>(s[pos + 1])))
                {
                    pos += 2;
                }
                // 실제 시간대 보정은 생략 (로컬 시간 기준으로 처리)
            }
        }

        std::time_t tt = std::mktime(&tm);
        if (tt == static_cast<std::time_t>(-1))
            return std::nullopt;

        auto base_tp = std::chrono::system_clock::from_time_t(tt);

        if (fractional_ns != 0)
        {
            auto frac = std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::nanoseconds(fractional_ns));
            auto tp = base_tp + frac;
            return std::optional<std::chrono::system_clock::time_point>(tp);
        }

        return std::optional<std::chrono::system_clock::time_point>(base_tp);
    }

    std::optional<std::chrono::system_clock::time_point>
        get_datetime_by_path(xml_node* root,
            const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        return parse_iso8601_datetime(*opt_text);
    }

    std::optional<std::chrono::system_clock::time_point>
        parse_unix_epoch_seconds(const std::string& text)
    {
        std::string s = trim_spaces(text);
        if (s.empty())
            return std::nullopt;

        try
        {
            long long sec = std::stoll(s);

            auto dur = std::chrono::seconds(sec);
            auto sys_dur = std::chrono::duration_cast<std::chrono::system_clock::duration>(dur);
            std::chrono::system_clock::time_point tp(sys_dur);

            return std::optional<std::chrono::system_clock::time_point>(tp);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<std::chrono::system_clock::time_point>
        parse_unix_epoch_millis(const std::string& text)
    {
        std::string s = trim_spaces(text);
        if (s.empty())
            return std::nullopt;

        try
        {
            long long ms = std::stoll(s);

            auto dur_ms = std::chrono::milliseconds(ms);
            auto sys_dur = std::chrono::duration_cast<std::chrono::system_clock::duration>(dur_ms);
            std::chrono::system_clock::time_point tp(sys_dur);

            return std::optional<std::chrono::system_clock::time_point>(tp);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<std::chrono::system_clock::time_point>
        get_datetime_from_epoch_sec_by_path(xml_node* root,
            const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        return parse_unix_epoch_seconds(*opt_text);
    }

    std::optional<std::chrono::system_clock::time_point>
        get_datetime_from_epoch_millis_by_path(xml_node* root,
            const std::string& path)
    {
        auto opt_text = get_text_opt_by_path(root, path);
        if (!opt_text)
            return std::nullopt;

        return parse_unix_epoch_millis(*opt_text);
    }

    // -----------------------------
    // XML 검증 모듈 구현
    // -----------------------------
    required_path_validator::required_path_validator(std::vector<std::string> required_paths)
        : required_paths_(std::move(required_paths))
    {
    }

    bool required_path_validator::validate(const xml_node& root,
        std::string& error_message) const
    {
        xml_node* root_non_const = const_cast<xml_node*>(&root);

        for (const auto& path : required_paths_)
        {
            if (is_attr_path(path))
            {
                std::string full = trim_spaces(path);

                while (!full.empty() && full.front() == '/')
                    full.erase(full.begin());

                if (full.empty())
                {
                    error_message = "Required path does not exist: " + path;
                    return false;
                }

                std::string node_path;
                std::string attr_part;

                auto pos = full.rfind('/');
                if (pos == std::string::npos)
                {
                    node_path = "";
                    attr_part = full;
                }
                else
                {
                    node_path = full.substr(0, pos);
                    attr_part = full.substr(pos + 1);
                }

                attr_part = trim_spaces(attr_part);
                if (attr_part.empty() || attr_part[0] != '@')
                {
                    error_message = "Invalid required path format:" + path;
                    return false;
                }

                std::string attr_name = attr_part.substr(1);
                if (attr_name.empty())
                {
                    error_message = "Invalid required path format: " + path;
                    return false;
                }

                xml_node* target_node = nullptr;
                if (node_path.empty())
                {
                    target_node = root_non_const;
                }
                else
                {
                    auto nodes = xpath_select(root_non_const, node_path);
                    if (nodes.empty())
                    {
                        error_message = "Required path does not exist: " + path;
                        return false;
                    }
                    target_node = nodes.front();
                }

                const xml_attribute* attr = target_node->find_attribute(attr_name);
                if (!attr)
                {
                    error_message = "Required properties do not exist: " + path;
                    return false;
                }

                continue;
            }

            auto nodes = xpath_select(root_non_const, path);
            if (nodes.empty())
            {
                error_message = "Required path does not exist: " + path;
                return false;
            }
        }

        return true;
    }

}  
