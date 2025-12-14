#pragma once

#include <string>
#include <unordered_set>
#include "any_value.hpp"

namespace jspp
{
    namespace LogAnyValue
    {
        // Forward declarations
        inline std::string to_log_string(const AnyValue &val);
        inline std::string to_log_string(const AnyValue &val, std::unordered_set<const void *> &visited, int depth);
    }
}