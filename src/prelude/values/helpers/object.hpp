#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "js_value.hpp"

std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

jspp::JsValue jspp::JsObject::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::JsValue::resolve_property_for_read(it->second);
    }
    return jspp::JsValue::make_undefined();
}

jspp::JsValue jspp::JsObject::set_property(const std::string &key, const JsValue &value)
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
