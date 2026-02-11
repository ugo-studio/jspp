#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace ObjectPrototypes
    {
        inline AnyValue& get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }

            return std::nullopt;
        }
    }
}