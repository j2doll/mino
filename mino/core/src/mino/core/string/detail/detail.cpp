
#include "mino/core/string/detail/detail.hpp"

namespace mino::core::string::detail {

    void collect_format_args(std::vector<std::string>&) noexcept {}

    std::string format_impl(std::string_view fmt_str, const std::vector<std::string>& args) noexcept {
        try {
            std::string result;
            result.reserve(fmt_str.size() + args.size() * 16);

            std::size_t arg_index = 0;
            const std::size_t n = fmt_str.size();

            for (std::size_t i = 0; i < n; ++i) {
                char ch = fmt_str[i];

                if (ch == '{') {
                    if (i + 1 < n && fmt_str[i + 1] == '{') {
                        result.push_back('{');
                        ++i;
                        continue;
                    }

                    if (i + 1 < n && fmt_str[i + 1] == '}') {
                        if (arg_index >= args.size()) {
                            result.append("{missing_arg}");
                        }
                        else {
                            result.append(args[arg_index++]);
                        }
                        ++i;
                        continue;
                    }

                    result.append("{invalid_syntax}");
                }
                else if (ch == '}') {
                    if (i + 1 < n && fmt_str[i + 1] == '}') {
                        result.push_back('}');
                        ++i;
                        continue;
                    }
                    result.append("{unmatched_bracket}");
                }
                else {
                    result.push_back(ch);
                }
            }

            if (arg_index < args.size()) {
                result.append(" {extra_args_omitted}");
            }

            return result;
        }
        catch (...) {
            return std::string(fmt_str);
        }
    }

    NamedArg::NamedArg(std::string_view k, const char* v)
        : key(k), data(v),
        printer([](std::ostream& os, const void* ptr) {
        write_arg(os, static_cast<const char*>(ptr));
            }) {
    }

    void NamedArg::write(std::ostream& os) const {
        if (printer && data) {
            printer(os, data);
        }
    }

    void format_named_to(std::ostream& os, std::string_view fmt, named_args args) {
        format_named_impl(os, fmt, [&](std::string_view key) {
            for (const auto& arg : args) {
                if (arg.key == key || to_console_encoding(std::string(arg.key)) == key) {
                    arg.write(os);
                    return true;
                }
            }
            return false;
            });
    }

} // namespace mino::core::string::detail
