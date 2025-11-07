#pragma once

#include "types.hpp"

namespace jspp
{
    class AnyValue;

    struct JsObject
    {
        std::unordered_map<std::string, AnyValue> props;

        std::string to_raw_string() const
        {
            return "[Object Object]";
        }

        AnyValue &operator[](const std::string &key)
        {
            auto it = props.find(key);
            if (it == props.end())
            {
                return AnyValue::make_undefined();
            }
            return it->second;
        }
        AnyValue &operator[](const AnyValue &key)
        {
            auto it = props.find(key.convert_to_raw_string());
            if (it == props.end())
            {
                return AnyValue::make_undefined();
            }
            return it->second;
        }
    };
}
