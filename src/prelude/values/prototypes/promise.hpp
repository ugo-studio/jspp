#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace PromisePrototypes
    {
        AnyValue &get_then_fn();
        AnyValue &get_catch_fn();
        AnyValue &get_finally_fn();
        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
