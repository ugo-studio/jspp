#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "values/string.hpp"
#include "error.hpp"
#include "any_value.hpp"
#include "values/prototypes/string.hpp"

std::string jspp::JsString::to_std_string() const
{
    return value;
}

jspp::JsIterator<jspp::AnyValue> jspp::JsString::get_iterator()
{
    size_t strLength = value.length();
    for (size_t idx = 0; idx < strLength; idx++)
    {
        co_yield AnyValue::make_string(std::string(1, value[idx]));
    }
    co_return AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsString::get_property(const std::string &key, const AnyValue &thisVal)
{
    // Check for prototype methods
    auto proto_fn = StringPrototypes::get(key, this);
    if (proto_fn.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_fn.value(), thisVal, key);
    }
    // Handle character access by string index (e.g., "abc"["1"])
    if (JsArray::is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return get_property(idx);
    }
    // not found
    return AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsString::get_property(uint32_t idx)
{
    if (idx < value.length())
    {
        return AnyValue::make_string(std::string(1, value[idx]));
    }
    return AnyValue::make_undefined();
}