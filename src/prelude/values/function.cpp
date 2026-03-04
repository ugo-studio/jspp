#include "jspp.hpp"
#include "values/function.hpp"
#include "values/prototypes/function.hpp"

namespace jspp {

// --- JsFunction Implementation ---

JsFunction::JsFunction(const JsFunctionCallable &c,
                        std::optional<std::string> n,
                        std::unordered_map<std::string, AnyValue> p,
                        std::map<AnyValue, AnyValue> sp,
                        bool is_cls,
                        bool is_ctor)
    : callable(c),
        name(std::move(n)),
        props(std::move(p)),
        symbol_props(std::move(sp)),
        proto(Constants::Null),
        is_generator(callable.index() == 1),
        is_async(callable.index() == 2),
        is_class(is_cls),
        is_constructor(is_ctor && !is_generator && !is_async)
{
}

JsFunction::JsFunction(const JsFunctionCallable &c,
                        bool is_gen,
                        std::optional<std::string> n,
                        std::unordered_map<std::string, AnyValue> p,
                        std::map<AnyValue, AnyValue> sp,
                        bool is_cls,
                        bool is_ctor)
    : callable(c),
        name(std::move(n)),
        props(std::move(p)),
        symbol_props(std::move(sp)),
        proto(Constants::Null),
        is_generator(is_gen),
        is_async(callable.index() == 2),
        is_class(is_cls),
        is_constructor(is_ctor && !is_gen && !is_async)
{
}

JsFunction::JsFunction(const JsFunctionCallable &c,
                        bool is_gen,
                        bool is_async_func,
                        std::optional<std::string> n,
                        std::unordered_map<std::string, AnyValue> p,
                        std::map<AnyValue, AnyValue> sp,
                        bool is_cls,
                        bool is_ctor)
    : callable(c),
        name(std::move(n)),
        props(std::move(p)),
        symbol_props(std::move(sp)),
        proto(Constants::Null),
        is_generator(is_gen),
        is_async(is_async_func),
        is_class(is_cls),
        is_constructor(is_ctor && !is_gen && !is_async_func)
{
}

std::string JsFunction::to_std_string() const
{
    std::string type_part = this->is_async ? "async function" : this->is_generator ? "function*"
                                                                                    : "function";
    std::string name_part = this->name.value_or("");
    return type_part + " " + name_part + "() { [native code] }";
}

AnyValue JsFunction::call(AnyValue thisVal, std::span<const AnyValue> args)
{
    if (std::function<AnyValue(AnyValue, std::span<const AnyValue>)> *func = std::get_if<0>(&callable))
    {
        return (*func)(thisVal, args);
    }
    else if (std::function<jspp::JsIterator<jspp::AnyValue>(AnyValue, std::vector<AnyValue>)> *func = std::get_if<1>(&callable))
    {
        return AnyValue::from_iterator((*func)(thisVal, std::vector<AnyValue>(args.begin(), args.end())));
    }
    else if (std::function<jspp::JsPromise(AnyValue, std::vector<AnyValue>)> *func = std::get_if<2>(&callable))
    {
        return AnyValue::from_promise((*func)(thisVal, std::vector<AnyValue>(args.begin(), args.end())));
    }
    else if (std::function<jspp::JsAsyncIterator<jspp::AnyValue>(AnyValue, std::vector<AnyValue>)> *func = std::get_if<3>(&callable))
    {
        return AnyValue::from_async_iterator((*func)(thisVal, std::vector<AnyValue>(args.begin(), args.end())));
    }
    else
    {
        return Constants::UNDEFINED;
    }
}

bool JsFunction::has_property(const std::string &key) const
{
    if (props.find(key) != props.end())
        return true;
    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }
    if (FunctionPrototypes::get(key).has_value())
        return true;
    return false;
}

bool JsFunction::has_symbol_property(const AnyValue &key) const
{
    if (symbol_props.count(key))
        return true;
    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }
    if (FunctionPrototypes::get(key).has_value())
        return true;
    return false;
}

AnyValue JsFunction::get_property(const std::string &key, AnyValue thisVal)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        if (!proto.is_null() && !proto.is_undefined())
        {
            if (proto.has_property(key))
            {
                return proto.get_property_with_receiver(key, thisVal);
            }
        }

        auto proto_it = FunctionPrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

AnyValue JsFunction::get_symbol_property(const AnyValue &key, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it == symbol_props.end())
    {
        if (!proto.is_null() && !proto.is_undefined())
        {
            auto res = proto.get_symbol_property_with_receiver(key, thisVal);
            if (!res.is_undefined())
                return res;
        }

        auto proto_it = FunctionPrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
}

AnyValue JsFunction::set_property(const std::string &key, AnyValue value, AnyValue thisVal)
{
    auto proto_it = FunctionPrototypes::get(key);
    if (proto_it.has_value())
    {
        auto proto_value = proto_it.value();
        if (proto_value.is_accessor_descriptor())
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
        if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
    }

    auto it = props.find(key);
    if (it != props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    }
    else
    {
        props[key] = value;
        return value;
    }
}

AnyValue JsFunction::set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it != symbol_props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
    }
    else
    {
        symbol_props[key] = value;
        return value;
    }
}

// --- FunctionPrototypes Implementation ---

namespace FunctionPrototypes {

AnyValue &get_toString_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                 { return AnyValue::make_string(thisVal.as_function()->to_std_string()); },
                                                 "toString");
    return fn;
}

AnyValue &get_call_fn()
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

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "toString") return get_toString_fn();
    if (key == "call") return get_call_fn();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    if (key.is_string())
        return get(key.as_string()->value);

    if (key == AnyValue::from_symbol(WellKnownSymbols::toStringTag)) return get_toString_fn();
    if (key == "call") return get_call_fn();
    return std::nullopt;
}

} // namespace FunctionPrototypes

} // namespace jspp
