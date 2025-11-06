#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsString
    {
        std::string value;

        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };
}
