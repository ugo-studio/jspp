#pragma once

#include "types.hpp"
#include "any_value.hpp"

// Define Function constructor
// In a full implementation, this would support 'new Function(args, body)'
inline auto Function = jspp::AnyValue::make_class([](const jspp::AnyValue& thisVal, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
    return jspp::UNDEFINED;
}, "Function");
