#pragma once

#include "types.hpp"
#include "values/generator.hpp"
#include "any_value.hpp"

template <typename T>
std::string jspp::JsGenerator<T>::to_std_string() const
{
    return "[object Generator]";
}

template <typename T>
jspp::AnyValue jspp::JsGenerator<T>::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_read(it->second);
    }
    return jspp::AnyValue::make_undefined();
}

template <typename T>
jspp::AnyValue jspp::JsGenerator<T>::set_property(const std::string &key, const AnyValue &value)
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
