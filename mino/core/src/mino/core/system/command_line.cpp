#include <sstream>
#include <algorithm>

#include "mino/core/system/command_line.hpp"

namespace mino::core::system {

        void command_line::add_option(const std::string& long_name,
                                      char short_name,
                                      bool requires_value,
                                      const std::string& description)
        {
            option_def def;
            def.long_name = long_name;
            def.short_name = short_name;
            def.requires_value = requires_value;
            def.description = description;
            m_defs.push_back(def);

            if (short_name != '\0') {
                m_short_to_long[short_name] = long_name;
            }
        }

        void command_line::set_version(const std::string& ver)
        {
            m_version = ver;
        }

        bool command_line::parse(int argc, char* argv[])
        {
            m_values.clear();
            m_positionals.clear();
            m_program_name.clear();

            if (argc <= 0) {
                return false;
            }

            m_program_name = argv[0];

            auto find_long_def = [&](const std::string& name) -> const option_def* {
                for (const auto& d : m_defs) {
                    if (d.long_name == name) return &d;
                }
                return nullptr;
            };

            auto find_short_def = [&](char s) -> const option_def* {
                auto it = m_short_to_long.find(s);
                if (it == m_short_to_long.end()) return nullptr;
                return find_long_def(it->second);
            };

            for (int i = 1; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg.size() >= 2 && arg[0] == '-' && arg[1] == '-') {
                    // --long or --long=val
                    std::string body = arg.substr(2);
                    auto eq = body.find('=');
                    std::string name = (eq == std::string::npos) ? body : body.substr(0, eq);
                    std::string val;
                    if (eq != std::string::npos) {
                        val = body.substr(eq + 1);
                    }

                    if (name == "help") {
                        // 자동 처리: 사용법 출력용 값 저장
                        m_values["help"] = "1";
                        continue;
                    }
                    if (name == "version") {
                        m_values["version"] = "1";
                        continue;
                    }

                    const option_def* def = find_long_def(name);
                    if (!def) {
                        // 알 수 없는 옵션은 위치 인자로 취급
                        m_positionals.push_back(arg);
                        continue;
                    }

                    if (def->requires_value) {
                        if (!val.empty()) {
                            m_values[def->long_name] = val;
                        } else {
                            // 다음 인자를 값으로 사용
                            if (i + 1 < argc) {
                                m_values[def->long_name] = argv[++i];
                            } else {
                                // 값이 필요하지만 없음 -> 파싱 실패
                                return false;
                            }
                        }
                    } else {
                        m_values[def->long_name] = "1";
                    }
                } else if (arg.size() >= 2 && arg[0] == '-') {
                    // -a or -abc or -a=value or -a value
                    // 처리: 첫 문자를 단축 옵션으로 해석. 결합된 옵션은 모두 플래그로 처리.
                    if (arg.size() == 2) {
                        char s = arg[1];
                        if (s == 'h') {
                            m_values["help"] = "1";
                            continue;
                        }
                        if (s == 'v') {
                            m_values["version"] = "1";
                            continue;
                        }

                        const option_def* def = find_short_def(s);
                        if (!def) {
                            m_positionals.push_back(arg);
                            continue;
                        }

                        if (def->requires_value) {
                            // 다음 인자 사용
                            if (i + 1 < argc) {
                                m_values[def->long_name] = argv[++i];
                            } else {
                                return false;
                            }
                        } else {
                            m_values[def->long_name] = "1";
                        }
                    } else {
                        // 길이가 3 이상: -abc 또는 -a=value
                        // check if format is -a=val
                        if (arg.size() >= 4 && arg[2] == '=') {
                            char s = arg[1];
                            std::string val = arg.substr(3);
                            const option_def* def = find_short_def(s);
                            if (!def) {
                                m_positionals.push_back(arg);
                                continue;
                            }
                            if (!def->requires_value) {
                                // short option이 값 불필요인데 = 가 있음 -> 그냥 플래그
                                m_values[def->long_name] = "1";
                            } else {
                                m_values[def->long_name] = val;
                            }
                        } else {
                            // 결합된 플래그 처리: -abc -> -a, -b, -c (모두 requires_value == false 여야 함)
                            bool treated = true;
                            for (size_t k = 1; k < arg.size(); ++k) {
                                char s = arg[k];
                                const option_def* def = find_short_def(s);
                                if (!def || def->requires_value) {
                                    treated = false;
                                    break;
                                }
                            }
                            if (treated) {
                                for (size_t k = 1; k < arg.size(); ++k) {
                                    char s = arg[k];
                                    const option_def* def = find_short_def(s);
                                    if (def) {
                                        m_values[def->long_name] = "1";
                                    }
                                }
                            } else {
                                // 처리 불가하면 위치 인자로
                                m_positionals.push_back(arg);
                            }
                        }
                    }
                } else {
                    // 위치 인자
                    m_positionals.push_back(arg);
                }
            }

            // 내장된 help/version 처리: --help 또는 -h를 사용하면 사용법 출력 유도
            if (m_values.find("help") != m_values.end()) {
                // 사용법은 호출자(프로그램)가 필요하면 호출하여 출력할 수 있도록 true 반환
                return true;
            }
            if (m_values.find("version") != m_values.end()) {
                return true;
            }

            return true;
        }

        bool command_line::has(const std::string& name) const
        {
            auto it = m_values.find(name);
            return it != m_values.end();
        }

        std::string command_line::get(const std::string& name, const std::string& default_val) const
        {
            auto it = m_values.find(name);
            if (it == m_values.end()) return default_val;
            return it->second;
        }

        std::vector<std::string> command_line::positional() const
        {
            return m_positionals;
        }

        std::string command_line::usage() const
        {
            std::ostringstream ss;
            ss << "Usage: " << (m_program_name.empty() ? "<prog>" : m_program_name) << " [options] [args]\n\n";
            ss << "Options:\n";
            // 표준 도움말: 정렬 간단 구현
            for (const auto& d : m_defs) {
                ss << "  ";
                if (d.short_name != '\0') {
                    ss << "-" << d.short_name << ", ";
                } else {
                    ss << "    ";
                }
                ss << "--" << d.long_name;
                if (d.requires_value) {
                    ss << " <value>";
                }
                if (!d.description.empty()) {
                    ss << "\t" << d.description;
                }
                ss << "\n";
            }
            // 내장 옵션
            ss << "  -h, --help\tShow this help message\n";
            if (!m_version.empty()) {
                ss << "  -v, --version\tShow version (" << m_version << ")\n";
            }
            return ss.str();
        }


}