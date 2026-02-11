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
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_function()->to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_call_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue thisArg = Constants::UNDEFINED;
                                                             std::span<const AnyValue> fnArgs;

                                                             if (!args.empty())
                                                             {
                                                                 thisArg = args[0];
                                                                 fnArgs = args.subspan(1);
                                                             }

                                                             return thisVal.call(thisArg, fnArgs); },
                                                         "call");
            return fn;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }

            // --- call() method ---
            if (key == "call")
            {
                return get_call_fn();
            }

            return std::nullopt;
        }
    }
}
