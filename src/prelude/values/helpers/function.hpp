#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "js_value.hpp"

std::string jspp::JsFunction::to_std_string() const
{
    return "function " + name + "() { [native code] }";
}

jspp::JsValue jspp::JsFunction::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::JsValue::resolve_property_for_read(it->second);
    }
    return jspp::JsValue::make_undefined();
}

jspp::JsValue jspp::JsFunction::set_property(const std::string &key, const JsValue &value)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::JsValue::resolve_property_for_write(it->second, value);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
