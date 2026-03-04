#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace IteratorPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_iterator_fn();
        AnyValue &get_next_fn();
        AnyValue &get_return_fn();
        AnyValue &get_throw_fn();
        AnyValue &get_toArray_fn();
        AnyValue &get_drop_fn();
        AnyValue &get_take_fn();
        AnyValue &get_some_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
