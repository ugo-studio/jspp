#pragma once

#include "types.hpp"

namespace jspp
{
    struct JsFunction
    {
        std::function<AnyValue(const std::vector<AnyValue> &)> call;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;

        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };
}
