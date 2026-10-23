#pragma once

#include <string>
#include <vector>
#include "mino/core/schedule/monthly/monthly_rule.hpp"

namespace mino::core::schedule::monthly {

    std::string dump_rules(const std::vector<monthly_rule>& rules);

} // namespace mino::core::schedule::monthly
