#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace SymbolPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_valueOf_fn();
        AnyValue &get_toPrimitive_fn();
        AnyValue &get_description_desc();
        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
