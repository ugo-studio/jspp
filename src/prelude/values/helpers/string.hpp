#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "values/string.hpp"
#include "exception.hpp"
#include "any_value.hpp"
#include "values/prototypes/string.hpp"

inline std::string jspp::JsString::to_std_string() const
{
    return value;
}

inline jspp::JsIterator<jspp::AnyValue> jspp::JsString::get_iterator()
{
    size_t strLength = value.length();
    for (size_t idx = 0; idx < strLength; idx++)
    {
        co_yield AnyValue::make_string(std::string(1, value[idx]));
    }
    co_return AnyValue::make_undefined();
}

inline jspp::AnyValue jspp::JsString::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto proto_fn = StringPrototypes::get(key);
    if (proto_fn.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_fn.value(), thisVal, key);
    }
    if (JsArray::is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return get_property(idx);
    }
    return Constants::UNDEFINED;
}

inline jspp::AnyValue jspp::JsString::get_property(uint32_t idx)
{
    if (idx < value.length())
    {
        return AnyValue::make_string(std::string(1, value[idx]));
    }
    return Constants::UNDEFINED;
}
