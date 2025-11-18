#pragma once

#include "types.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsFunction
    {
        std::function<AnyValue(const std::vector<AnyValue> &)> call;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;
        AnyValue get_property(const std::string &key);
        AnyValue set_property(const std::string &key, const AnyValue &value);
    };
}
