#pragma once

#include "types.hpp"

namespace jspp
{
    class AnyValue;

    struct JsFunction
    {
        std::function<AnyValue(const std::vector<AnyValue> &)> call;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;

        std::string to_raw_string() const;
        AnyValue &operator[](const std::string &key);
        AnyValue &operator[](const AnyValue &key);
    };
}
