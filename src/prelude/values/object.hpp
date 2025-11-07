#pragma once

#include "types.hpp"

namespace jspp
{
    namespace Convert
    {
        inline std::string to_string(const AnyValue &val);
    }

    struct JsObject
    {
        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const
        {
            return "[Object Object]";
        }

        AnyValue &operator[](const std::string &key)
        {
            auto it = props.find(key);
            if (it == props.end())
            {
                static AnyValue undefinedVal = NonValues::undefined; // store somewhere safe
                return undefinedVal;
            }
            return it->second;
        }
        AnyValue &operator[](const AnyValue &key)
        {
            auto it = props.find(Convert::to_string(key));
            if (it == props.end())
            {
                static AnyValue undefinedVal = NonValues::undefined; // store somewhere safe
                return undefinedVal;
            }
            return it->second;
        }
    };
}
