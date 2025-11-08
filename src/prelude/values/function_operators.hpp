#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "values/any_value.hpp"

std::string jspp::JsFunction::to_raw_string() const
{
    return "function " + name + "() { [native code] }";
}

jspp::AnyValue &jspp::JsFunction::operator[](const std::string &key)
{
    props[key] = AnyValue::make_undefined();
    return (*this)[key];
}

jspp::AnyValue &jspp::JsFunction::operator[](const AnyValue &key)
{
    return (*this)[key.convert_to_raw_string()];
}