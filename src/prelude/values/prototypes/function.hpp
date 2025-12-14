#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"
#include "error.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace FunctionPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsFunction *self)
        {
            // --- toString() method ---
            if (key == "toString" )
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &_) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            return std::nullopt;
        }
    }
}