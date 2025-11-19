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
    if (it == props.end())
    {
        // check prototype
        if constexpr (std::is_same_v<T, AnyValue>)
        {
            auto proto_it = GeneratorPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value());
            }
        }
        // prototype not found
        return jspp::AnyValue::make_undefined();
    }

    return jspp::AnyValue::resolve_property_for_read(it->second);
}

template <typename T>
jspp::AnyValue jspp::JsGenerator<T>::set_property(const std::string &key, const AnyValue &value)
{
    // set prototype property if accessor descriptor
    if constexpr (std::is_same_v<T, AnyValue>)
    {
        auto proto_it = GeneratorPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor())
            {
                return AnyValue::resolve_property_for_write(proto_it.value(), value);
            }
        }
    }

    // set own property
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
