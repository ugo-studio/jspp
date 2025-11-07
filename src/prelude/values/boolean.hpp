#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsBoolean
    {
        bool value;

        std::string to_std_string() const
        {
            return value ? "true" : "false";
        }

        // AnyValue &operator[](const AnyValue &key)
        // {
        //     return AnyValue{NonValues::undefined};
        // }
    };
}
