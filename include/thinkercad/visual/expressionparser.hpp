#pragma once
#include "blockprogram.hpp"
#include <string_view>
namespace thinkercad::visual {
// Parses only the documented expression subset, never statements or raw C++.
class ExpressionParser {
  public:
    static Expression parse(std::string_view);
};
} // namespace thinkercad::visual
