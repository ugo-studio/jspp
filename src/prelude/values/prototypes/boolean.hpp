#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace JsBoolean
    {
        std::string to_std_string(bool value);
        std::string to_std_string(const AnyValue &value);
    }

    namespace BooleanPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_valueOf_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
