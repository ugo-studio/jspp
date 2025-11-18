#pragma once

#include "types.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsObject
    {
        std::map<std::string, AnyValue> props;

        std::string to_std_string() const;
        AnyValue get_property(const std::string &key);
        AnyValue set_property(const std::string &key, const AnyValue &value);
    };
}
