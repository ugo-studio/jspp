#pragma once

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

inline auto global = jspp::AnyValue::make_object({
    {"console", console},
    {"performance", performance},
});
