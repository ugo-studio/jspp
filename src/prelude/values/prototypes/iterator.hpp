#pragma once

#include "types.hpp"
#include "values/iterator.hpp"
#include "any_value.hpp"
#include "error.hpp"
#include "operators.hpp"

namespace jspp
{
    namespace IteratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsIterator<AnyValue> *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toString->key)
            {
                return AnyValue::make_function([self](const std::vector<AnyValue> &) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([self](const std::vector<AnyValue> &) -> AnyValue
                                               {
                                                auto res = self->next();
                                                return AnyValue::make_object({{"value",res.value.value_or(AnyValue::make_undefined())},{"done",AnyValue::make_boolean(res.done)},}); },
                                               key);
            }
            // --- toArray() method ---
            if (key == "toArray")
            {
                return AnyValue::make_function([self](const std::vector<AnyValue> &) -> AnyValue
                                               { return AnyValue::make_array(self->to_vector()); },
                                               key);
            }

            return std::nullopt;
        }
    }
}