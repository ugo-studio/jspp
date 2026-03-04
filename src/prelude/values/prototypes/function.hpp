#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace FunctionPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_call_fn();
        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
