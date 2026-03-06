#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace JsNumber
    {
        std::string to_std_string(double num);
        std::string to_std_string(const AnyValue &value);
        std::string to_radix_string(double value, int radix);

    }

    namespace NumberPrototypes
    {
        AnyValue &get_toExponential_fn();
        AnyValue &get_toFixed_fn();
        AnyValue &get_toPrecision_fn();
        AnyValue &get_toString_fn();
        AnyValue &get_valueOf_fn();
        AnyValue &get_toLocaleString_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
