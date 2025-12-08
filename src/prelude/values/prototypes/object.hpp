#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"
#include "error.hpp"
#include "operators.hpp"

namespace jspp
{
    namespace ObjectPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsObject *self)
        {
            // --- toString() method ---
            if (key == "toString" )
            {
                return AnyValue::make_function([self](const std::vector<AnyValue> &_) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            return std::nullopt;
        }
    }
}