#pragma once

#include <variant>

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"
#include "values/prototypes/function.hpp"

std::string jspp::JsFunction::to_std_string() const
{
    return "function " + name + "() { [native code] }";
}

jspp::AnyValue jspp::JsFunction::call(const std::vector<AnyValue> &args)
{

    if (std::function<AnyValue(const std::vector<AnyValue> &)> *func = std::get_if<0>(&callable))
    {
        return (*func)(args);
    }
    else if (std::function<jspp::JsIterator<jspp::AnyValue>(const std::vector<jspp::AnyValue> &)> *func = std::get_if<1>(&callable))
    {
        return AnyValue::from_iterator((*func)(args));
    }
    else
    {
        return AnyValue::make_undefined();
    }
}

jspp::AnyValue jspp::JsFunction::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // check prototype
        auto proto_it = FunctionPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value());
        }
        // not found
        return AnyValue::make_undefined();
    }
    return AnyValue::resolve_property_for_read(it->second);
}

jspp::AnyValue jspp::JsFunction::set_property(const std::string &key, const AnyValue &value)
{
    // set prototype property if accessor descriptor
    auto proto_it = FunctionPrototypes::get(key, this);
    if (proto_it.has_value())
    {
        auto proto_value = proto_it.value();
        if (proto_value.is_accessor_descriptor())
        {
            return AnyValue::resolve_property_for_write(proto_value, value);
        }
    }

    // set own property
    auto it = props.find(key);
    if (it != props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, value);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
