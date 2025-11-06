#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsNumber
    {
        double value;

        AnyValue &operator[](const AnyValue &_)
        {
            return AnyValue{NonValues::undefined};
        }
    };
}
