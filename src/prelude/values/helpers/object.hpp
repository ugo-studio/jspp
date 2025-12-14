#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

jspp::AnyValue jspp::JsObject::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // check prototype chain
        if (proto && !(*proto).is_null() && !(*proto).is_undefined())
        {
            return (*proto).get_property_with_receiver(key, thisVal);
        }

        // check built-in prototype methods (Object.prototype)
        // ideally these should be on the root prototype object, but for now we keep this fallback
        auto proto_it = ObjectPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        // not found
        return AnyValue::make_undefined();
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

jspp::AnyValue jspp::JsObject::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    // set prototype property if accessor descriptor
    auto proto_it = ObjectPrototypes::get(key, this);
    if (proto_it.has_value())
    {
        auto proto_value = proto_it.value();
        if (proto_value.is_accessor_descriptor())
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
        if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
    }

    // set own property
    auto it = props.find(key);
    if (it != props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
