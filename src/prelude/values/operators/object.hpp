#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

jspp::AnyValue &jspp::JsObject::operator[](const std::string &key)
{

    auto it = props.find(key);
    if (it == props.end())
    {
        // std::unordered_map::operator[] default-constructs AnyValue (which is Undefined)
        return props[key];
    }
    return it->second;
}

jspp::AnyValue &jspp::JsObject::operator[](const AnyValue &key)
{
    return (*this)[key.to_std_string()];
}
