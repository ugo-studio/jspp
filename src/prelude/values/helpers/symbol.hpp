#pragma once

#include "types.hpp"
#include "values/symbol.hpp"
#include "any_value.hpp"
#include "values/prototypes/symbol.hpp"

inline std::string jspp::JsSymbol::to_std_string() const
{
    return "Symbol(" + description + ")";
}

inline jspp::AnyValue jspp::JsSymbol::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto proto_it = SymbolPrototypes::get(key);
    if (proto_it.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
    }
    return Constants::UNDEFINED;
}
