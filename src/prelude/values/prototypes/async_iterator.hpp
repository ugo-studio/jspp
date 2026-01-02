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
            // For async iterators, the async iterator is itself (similar to sync iterators)
            if (key == WellKnownSymbols::asyncIterator->key)
            {
                // We return 'this'. Since we can't easily create a shared_ptr from raw pointer,
                // we rely on the context to hold the reference or implement better shared_from_this strategy.
                // For now, assume it works like Iterator.
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                               { 
                                                    // This is slightly dangerous as we create a new shared_ptr from raw.
                                                    // TODO: fix lifetime management
                                                    return thisVal; },
                                               key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                                                AnyValue val = args.empty() ? AnyValue::make_undefined() : args[0];
                                                auto res = self->next(val);
                                                return AnyValue::make_promise(res); },
                                               key);
            }

            return std::nullopt;
        }
    }
}
