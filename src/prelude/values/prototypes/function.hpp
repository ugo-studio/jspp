#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace FunctionPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsFunction *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            // --- call() method ---
            if (key == "call")
            {
                return AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    AnyValue thisArg = Constants::UNDEFINED;
                    std::span<const AnyValue> fnArgs;

                    if (!args.empty())
                    {
                        thisArg = args[0];
                        fnArgs = args.subspan(1);
                    }

                    return thisVal.call(thisArg, fnArgs);
                }, key);
            }

            return std::nullopt;
        }
    }
}