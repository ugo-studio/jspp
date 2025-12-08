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
    if (it == props.end())
    {
        // check prototype
        auto proto_it = ObjectPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), key);
        }
        // not found
        return AnyValue::make_undefined();
    }
    return AnyValue::resolve_property_for_read(it->second, key);
}

jspp::AnyValue jspp::JsObject::set_property(const std::string &key, const AnyValue &value)
{
    // set prototype property if accessor descriptor
    auto proto_it = ObjectPrototypes::get(key, this);
    if (proto_it.has_value())
    {
        auto proto_value = proto_it.value();
        if (proto_value.is_accessor_descriptor())
        {
            return AnyValue::resolve_property_for_write(proto_value, value, key);
        }
        if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
        {
            return AnyValue::resolve_property_for_write(proto_value, value, key);
        }
    }

    // set own property
    auto it = props.find(key);
    if (it != props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, value, key);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
