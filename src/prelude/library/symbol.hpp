#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"

#include "values/object.hpp"
#include "any_value.hpp"

inline auto Symbol = jspp::AnyValue::make_object({
    {"iterator", jspp::AnyValue::make_string(jspp::WellKnownSymbols::iterator)},
});
