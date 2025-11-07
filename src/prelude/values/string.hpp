#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsString
    {
        std::string value;

        std::string to_std_string()const
        {
            return value;
        }

        //     AnyValue &operator[](const AnyValue &key)
        //     {
        //         return AnyValue{NonValues::undefined};
        //     }
    };
}
