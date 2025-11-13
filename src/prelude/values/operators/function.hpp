#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"

std::string jspp::JsFunction::to_std_string() const
{
    return "function " + name + "() { [native code] }";
}

jspp::AnyValue &jspp::JsFunction::operator[](const std::string &key)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // std::unordered_map::operator[] default-constructs AnyValue (which is Undefined)
        return props[key];
    }
    return it->second;
}

jspp::AnyValue &jspp::JsFunction::operator[](const AnyValue &key)
{
    return (*this)[key.to_std_string()];
}
