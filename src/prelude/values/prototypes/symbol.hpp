#pragma once

#include "types.hpp"
#include "values/symbol.hpp"
#include "any_value.hpp"
#include "utils/well_known_symbols.hpp"

namespace jspp
{
    namespace SymbolPrototypes
    {
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_symbol()->to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_valueOf_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return thisVal; },
                                                         "valueOf");
            return fn;
        }

        inline AnyValue &get_toPrimitive_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return thisVal; },
                                                         "[Symbol.toPrimitive]");
            return fn;
        }

        inline AnyValue &get_description_desc()
        {
            static auto getter = [](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
            {
                auto self = thisVal.as_symbol();
                if (self->description.empty())
                {
                    return Constants::UNDEFINED;
                }
                return AnyValue::make_string(self->description);
            };
            static AnyValue desc = AnyValue::make_accessor_descriptor(getter, std::nullopt, false, true);
            return desc;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }

            // --- valueOf() method ---
            if (key == "valueOf")
            {
                return get_valueOf_fn();
            }

            // --- [Symbol.toPrimitive] ---
            if (key == WellKnownSymbols::toPrimitive->key)
            {
                return get_toPrimitive_fn();
            }

            // --- description property ---
            if (key == "description")
            {
                return get_description_desc();
            }

            return std::nullopt;
        }
    }
}
