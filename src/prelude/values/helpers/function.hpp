#pragma once

#include <variant>
#include <optional>

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"
#include "values/prototypes/function.hpp"

std::string jspp::JsFunction::to_std_string() const
{
    std::string type_part = this->is_async ? "async function" : this->is_generator ? "function*"
                                                                                   : "function";
    std::string name_part = this->name.value_or("");
    return type_part + " " + name_part + "() { [native code] }";
}

jspp::AnyValue jspp::JsFunction::call(const AnyValue &thisVal, const std::vector<AnyValue> &args)
{
    if (std::function<AnyValue(const AnyValue &, const std::vector<AnyValue> &)> *func = std::get_if<0>(&callable))
    {
        return (*func)(thisVal, args);
    }
    else if (std::function<jspp::JsIterator<jspp::AnyValue>(const AnyValue &, const std::vector<jspp::AnyValue> &)> *func = std::get_if<1>(&callable))
    {
        return AnyValue::from_iterator((*func)(thisVal, args));
    }
    else if (std::function<jspp::JsPromise(const AnyValue &, const std::vector<jspp::AnyValue> &)> *func = std::get_if<2>(&callable))
    {
        return AnyValue::make_promise((*func)(thisVal, args));
    }
    else
    {
        return AnyValue::make_undefined();
    }
}

bool jspp::JsFunction::has_property(const std::string &key) const
{
    if (props.find(key) != props.end())
        return true;
    if (proto && !(*proto).is_null() && !(*proto).is_undefined())
    {
        if ((*proto).has_property(key))
            return true;
    }
    if (FunctionPrototypes::get(key, const_cast<JsFunction *>(this)).has_value())
        return true;
    return false;
}

jspp::AnyValue jspp::JsFunction::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // check explicit proto chain (e.g. for classes extending other classes)
        if (proto && !(*proto).is_null() && !(*proto).is_undefined())
        {
            if ((*proto).has_property(key))
            {
                return (*proto).get_property_with_receiver(key, thisVal);
            }
        }

        // check prototype (implicit Function.prototype)
        auto proto_it = FunctionPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        // not found
        return AnyValue::make_undefined();
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

jspp::AnyValue jspp::JsFunction::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    // set prototype property if accessor descriptor
    auto proto_it = FunctionPrototypes::get(key, this);
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

    // set own property
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

// AnyValue::construct implementation
const jspp::AnyValue jspp::AnyValue::construct(const std::vector<AnyValue> &args, const std::optional<std::string> &name) const
{
    if (!is_function() || !as_function()->is_constructor)
    {
        throw Exception::make_exception(name.value_or(to_std_string()) + " is not a constructor", "TypeError");
    }

    // 1. Get prototype
    AnyValue proto = get_own_property("prototype");
    // If prototype is not an object, default to a plain object (which ideally inherits from Object.prototype)
    // Here we just make a plain object.
    if (!proto.is_object())
    {
        proto = AnyValue::make_object({});
    }

    // 2. Create instance
    AnyValue instance = AnyValue::make_object_with_proto({}, proto);

    // 3. Call function
    // We pass 'instance' as 'this'
    AnyValue result = as_function()->call(instance, args);

    // 4. Return result if object, else instance
    if (result.is_object() || result.is_function() || result.is_array() || result.is_promise())
    {
        return result;
    }
    return instance;
}
