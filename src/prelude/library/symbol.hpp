#pragma once

#include "types.hpp"
#include "utils/well_known_symbols.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

// Define Symbol as a function
inline auto Symbol = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                   {
    std::string description = "";
    if (!args.empty() && !args[0].is_undefined()) {
        description = args[0].to_std_string();
    }
    return jspp::AnyValue::make_symbol(description); }, "Symbol", false);

// Initialize Symbol properties
struct SymbolInit
{
    SymbolInit()
    {
        // Static methods
        Symbol.define_data_property("for", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                         {
             std::string key = "";
             if (!args.empty()) key = args[0].to_std_string();
             return jspp::AnyValue::from_symbol(jspp::JsSymbol::for_global(key)); }, "for"));

        Symbol.define_data_property("keyFor", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                            {
             if (args.empty() || !args[0].is_symbol()) throw jspp::Exception::make_exception("Symbol.keyFor requires a symbol", "TypeError");
             auto sym = args[0].as_symbol();
             auto key = jspp::JsSymbol::key_for(sym);
             if (key.has_value()) return jspp::AnyValue::make_string(key.value());
             return jspp::AnyValue::make_undefined(); }, "keyFor"));

        // Well-known symbols
        Symbol.define_data_property("iterator", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::iterator), false, false, false);
        Symbol.define_data_property("asyncIterator", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::asyncIterator), false, false, false);
        Symbol.define_data_property("hasInstance", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::hasInstance), false, false, false);
        Symbol.define_data_property("isConcatSpreadable", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::isConcatSpreadable), false, false, false);
        Symbol.define_data_property("match", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::match), false, false, false);
        Symbol.define_data_property("matchAll", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::matchAll), false, false, false);
        Symbol.define_data_property("replace", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::replace), false, false, false);
        Symbol.define_data_property("search", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::search), false, false, false);
        Symbol.define_data_property("species", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::species), false, false, false);
        Symbol.define_data_property("split", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::split), false, false, false);
        Symbol.define_data_property("toPrimitive", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::toPrimitive), false, false, false);
        Symbol.define_data_property("toStringTag", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::toStringTag), false, false, false);
        Symbol.define_data_property("unscopables", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::unscopables), false, false, false);
    }
} symbolInit;
