#pragma once

#include "types.hpp"
#include "values/iterator.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace IteratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsIterator<AnyValue> *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }
            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                {
                                                   return thisVal; },
                                                key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                                                AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                auto res = self->next(val);
                                                return AnyValue::make_object({{"value",res.value.value_or(Constants::UNDEFINED)},{"done",AnyValue::make_boolean(res.done)},}); },
                                               key);
            }
            // --- toArray() method ---
            if (key == "toArray")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                               { return AnyValue::make_array(self->to_vector()); },
                                               key);
            }

            return std::nullopt;
        }
    }
}
