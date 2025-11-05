#pragma once

#include "types.hpp"
#include "object.hpp"
#include "well_known_symbols.hpp"

inline auto Symbol = jspp::Object::make_object({{"iterator", jspp::Object::make_string(jspp::WellKnownSymbols::iterator)},
                                                {"toString", jspp::Object::make_string(jspp::WellKnownSymbols::toString)}});
