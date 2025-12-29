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
        inline std::optional<AnyValue> get(const std::string &key, JsObject *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &_) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            return std::nullopt;
        }
    }
}