#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsObject
    {
        std::unordered_map<std::string, AnyValue> props;

        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };
}
