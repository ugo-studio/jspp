#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"

#include "values/object.hpp"
#include "any_value.hpp"

inline auto Symbol = jspp::AnyValue::make_object({
    {"iterator", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::iterator)},
});

// #pragma once

// #include "types.hpp"
// #include "well_known_symbols.hpp"
// #include "values/object.hpp"
// #include "any_value.hpp"

// // We define the Symbol constructor function
// inline auto symbolConstructor = [](const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
//     std::string desc = "";
//     if (!args.empty() && !args[0].is_undefined()) {
//         desc = args[0].to_std_string();
//     }
//     return jspp::AnyValue::make_symbol(desc);
// };

// inline auto Symbol = jspp::AnyValue::make_function(symbolConstructor, "Symbol");

// // Implementation of Symbol.for(key)
// inline auto forFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
//     std::string key = "";
//     if (!args.empty()) {
//         key = args[0].to_std_string();
//     }
//     auto symPtr = jspp::JsSymbol::for_global(key);
//     return jspp::AnyValue::from_symbol(symPtr);
// }, "for");

// // Implementation of Symbol.keyFor(sym)
// inline auto keyForFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
//     if (args.empty() || !args[0].is_symbol()) {
//         throw jspp::RuntimeError::make_error("Symbol.keyFor requires a symbol argument", "TypeError");
//     }

//     jspp::JsSymbol* sym = args[0].as_symbol();
//     auto result = jspp::JsSymbol::key_for(sym);

//     if (result.has_value()) {
//         return jspp::AnyValue::make_string(result.value());
//     } else {
//         return jspp::AnyValue::make_undefined();
//     }
// }, "keyFor");

// // Attach static properties/methods to the Symbol object
// struct SymbolInit {
//     SymbolInit() {
//         Symbol.set_own_property("iterator", jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::iterator));
//         Symbol.set_own_property("for", forFn);
//         Symbol.set_own_property("keyFor", keyForFn);
//     }
// } symbolInit;