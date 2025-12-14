#pragma once

#include <chrono>
#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

inline auto performance = jspp::AnyValue::make_object({
    {"now",
     jspp::AnyValue::make_function(
         // [C++14 Feature] Generalized Lambda Capture
         // We initialize 'startTime' RIGHT HERE inside the [].
         // It acts like a private variable stored inside this specific function.
         [startTime = std::chrono::steady_clock::now()](const jspp::AnyValue &thisVal, const std::vector<jspp::AnyValue> &)
         {
             // We calculate the diff against the captured startTime
             std::chrono::duration<double, std::milli> duration =
                 std::chrono::steady_clock::now() - startTime;

             return jspp::AnyValue::make_number(duration.count());
         },
         "now")},
});