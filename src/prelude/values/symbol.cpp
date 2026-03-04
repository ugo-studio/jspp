#include "jspp.hpp"
#include "values/symbol.hpp"
#include "values/prototypes/symbol.hpp"

namespace jspp {

// --- JsSymbol Implementation ---

std::string JsSymbol::to_std_string() const
{
    return "Symbol(" + description + ")";
}

AnyValue JsSymbol::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto proto_it = SymbolPrototypes::get(key);
    if (proto_it.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
    }
    return Constants::UNDEFINED;
}

// --- SymbolPrototypes Implementation ---

namespace SymbolPrototypes {

AnyValue &get_toString_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                 { return AnyValue::make_string(thisVal.as_symbol()->to_std_string()); },
                                                 "toString");
    return fn;
}

AnyValue &get_valueOf_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                 { return thisVal; },
                                                 "valueOf");
    return fn;
}

AnyValue &get_toPrimitive_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                 { return thisVal; },
                                                 "[Symbol.toPrimitive]");
    return fn;
}

AnyValue &get_description_desc()
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

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "toString") return get_toString_fn();
    if (key == "valueOf") return get_valueOf_fn();
    if (key == "description") return get_description_desc();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    if (key.is_string())
        return get(key.as_string()->value);

    if (key == AnyValue::from_symbol(WellKnownSymbols::toStringTag)) return get_toString_fn();
    if (key == "valueOf") return get_valueOf_fn();
    if (key == AnyValue::from_symbol(WellKnownSymbols::toPrimitive)) return get_toPrimitive_fn();

    return std::nullopt;
}

} // namespace SymbolPrototypes

} // namespace jspp
