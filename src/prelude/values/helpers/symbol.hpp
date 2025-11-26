#pragma once

#include "types.hpp"
#include "values/symbol.hpp"
#include "any_value.hpp"
#include "values/prototypes/symbol.hpp"

std::string jspp::JsSymbol::to_std_string() const
{
    return "Symbol(" + description + ")";
}

jspp::AnyValue jspp::JsSymbol::get_property(const std::string &key)
{
    // check prototype
    auto proto_it = SymbolPrototypes::get(key, this);
    if (proto_it.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_it.value());
    }
    // not found
    return AnyValue::make_undefined();
}