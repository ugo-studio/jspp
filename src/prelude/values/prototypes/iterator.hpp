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
                                                   // An iterator's iterator is itself.
                                                   // We need to return an AnyValue that holds a shared_ptr to this JsIterator.
                                                   // Since we only have a raw pointer `self`, we can't directly make a new shared_ptr.
                                                   // We'll return an AnyValue wrapping the raw pointer for now.
                                                   // This relies on the calling context to manage lifetime, which is true for `for-of`.
                                                   // A better solution might involve passing a shared_ptr to `self` into the prototype getters.
                                                   // For now, let's assume the object containing the iterator is alive.
                                                   return AnyValue::from_iterator_ref(self); },
                                                key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                                                AnyValue val = args.empty() ? AnyValue::make_undefined() : args[0];
                                                auto res = self->next(val);
                                                return AnyValue::make_object({{"value",res.value.value_or(AnyValue::make_undefined())},{"done",AnyValue::make_boolean(res.done)},}); },
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