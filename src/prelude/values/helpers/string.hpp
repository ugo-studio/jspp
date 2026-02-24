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
    for (size_t i = 0; i < value.length();)
    {
        unsigned char c = static_cast<unsigned char>(value[i]);
        size_t len = 1;
        if ((c & 0x80) == 0)
            len = 1;
        else if ((c & 0xE0) == 0xC0)
            len = 2;
        else if ((c & 0xF0) == 0xE0)
            len = 3;
        else if ((c & 0xF8) == 0xF0)
            len = 4;

        if (i + len > value.length())
            len = value.length() - i;

        co_yield AnyValue::make_string(value.substr(i, len));
        i += len;
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
