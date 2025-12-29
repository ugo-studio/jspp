#pragma once

#include "types.hpp"
#include "values/symbol.hpp"
#include "any_value.hpp"
#include "utils/well_known_symbols.hpp"

namespace jspp
{
    namespace SymbolPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsSymbol *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            // --- valueOf() method ---
            if (key == "valueOf")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &) -> AnyValue
                                               { 
                                                   return thisVal; 
                                               },
                                               key);
            }

            // --- [Symbol.toPrimitive] ---
            if (key == WellKnownSymbols::toPrimitive->key) {
                 return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &) -> AnyValue
                                               { 
                                                   return thisVal;
                                               },
                                               "[Symbol.toPrimitive]");
            }

            // --- description property ---
            if (key == "description")
            {
                return AnyValue::make_accessor_descriptor(
                    [self](const AnyValue &thisVal, const std::vector<AnyValue> &) -> AnyValue
                    {
                        if (self->description.empty())
                        {
                            return AnyValue::make_undefined();
                        }
                        return AnyValue::make_string(self->description);
                    },
                    std::nullopt,
                    false,
                    true);
            }

            return std::nullopt;
        }
    }
}