#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace AsyncIteratorPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_asyncIterator_fn();
        AnyValue &get_next_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
