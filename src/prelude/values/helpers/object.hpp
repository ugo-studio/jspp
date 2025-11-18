#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

jspp::AnyValue jspp::JsObject::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_read(it->second);
    }
    return jspp::AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsObject::set_property(const std::string &key, const AnyValue &value)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_write(it->second, value);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
