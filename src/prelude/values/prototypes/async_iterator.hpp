#pragma once

#include "types.hpp"
#include "values/async_iterator.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace AsyncIteratorPrototypes
    {
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_async_iterator()->to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_asyncIterator_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return thisVal; },
                                                         "Symbol.asyncIterator");
            return fn;
        }

        inline AnyValue &get_next_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                             auto res = thisVal.as_async_iterator()->next(val);
                                                             return AnyValue::make_promise(res); },
                                                         "next");
            return fn;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }
            // --- [Symbol.asyncIterator]() method ---
            if (key == WellKnownSymbols::asyncIterator->key)
            {
                return get_asyncIterator_fn();
            }
            // --- next() method ---
            if (key == "next")
            {
                return get_next_fn();
            }

            return std::nullopt;
        }
    }
}
