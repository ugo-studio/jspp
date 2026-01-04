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
        inline std::optional<AnyValue> get(const std::string &key, JsAsyncIterator<AnyValue> *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }
            // --- [Symbol.asyncIterator]() method ---
            if (key == WellKnownSymbols::asyncIterator->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                               { return thisVal; },
                                               key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                                                AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                auto res = self->next(val);
                                                return AnyValue::make_promise(res); },
                                               key);
            }

            return std::nullopt;
        }
    }
}