#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsBoolean
    {
        bool value;

        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };
}
