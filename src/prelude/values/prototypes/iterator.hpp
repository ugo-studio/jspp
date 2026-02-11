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
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_iterator()->to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_iterator_fn()
        {
            static AnyValue fn = AnyValue::make_generator([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                          { return thisVal; },
                                                          "Symbol.iterator");
            return fn;
        }

        inline AnyValue &get_next_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                             auto res = thisVal.as_iterator()->next(val);
                                                             return AnyValue::make_object({{"value", res.value.value_or(Constants::UNDEFINED)}, {"done", AnyValue::make_boolean(res.done)}}); },
                                                         "next");
            return fn;
        }

        inline AnyValue &get_toArray_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_array(thisVal.as_iterator()->to_vector()); },
                                                         "toArray");
            return fn;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }
            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return get_iterator_fn();
            }
            // --- next() method ---
            if (key == "next")
            {
                return get_next_fn();
            }
            // --- toArray() method ---
            if (key == "toArray")
            {
                return get_toArray_fn();
            }

            return std::nullopt;
        }
    }
}
