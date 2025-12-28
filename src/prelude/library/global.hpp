#pragma once

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

#include "library/promise.hpp"
#include "library/timer.hpp"

inline auto global = jspp::AnyValue::make_object({
    {"console", console},
    {"performance", performance},
    {"Error", Error},
    {"Promise", Promise},
    {"setTimeout", setTimeout},
    {"clearTimeout", clearTimeout},
    {"setInterval", setInterval},
    {"clearInterval", clearInterval},
});
