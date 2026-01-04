#pragma once

#include <variant>
#include <optional>

#include "types.hpp"
#include "values/function.hpp"
#include "any_value.hpp"
#include "values/prototypes/function.hpp"

namespace jspp
{
    inline std::string JsFunction::to_std_string() const
    {
        std::string type_part = this->is_async ? "async function" : this->is_generator ? "function*"
                                                                                       : "function";
        std::string name_part = this->name.value_or("");
        return type_part + " " + name_part + "() { [native code] }";
    }

    inline AnyValue JsFunction::call(const AnyValue &thisVal, std::span<const AnyValue> args)
    {
        if (std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)> *func = std::get_if<0>(&callable))
        {
            return (*func)(thisVal, args);
        }
        else if (std::function<jspp::JsIterator<jspp::AnyValue>(const AnyValue &, std::span<const AnyValue>)> *func = std::get_if<1>(&callable))
        {
            return AnyValue::from_iterator((*func)(thisVal, args));
        }
        else if (std::function<jspp::JsPromise(const AnyValue &, std::span<const AnyValue>)> *func = std::get_if<2>(&callable))
        {
            return AnyValue::make_promise((*func)(thisVal, args));
        }
        else if (std::function<jspp::JsAsyncIterator<jspp::AnyValue>(const AnyValue &, std::span<const AnyValue>)> *func = std::get_if<3>(&callable))
        {
            return AnyValue::from_async_iterator((*func)(thisVal, args));
        }
        else
        {
            return Constants::UNDEFINED;
        }
    }

    inline bool JsFunction::has_property(const std::string &key) const
    {
        if (props.find(key) != props.end())
            return true;
        if (!proto.is_null() && !proto.is_undefined())
        {
            if (proto.has_property(key))
                return true;
        }
        if (FunctionPrototypes::get(key, const_cast<JsFunction *>(this)).has_value())
            return true;
        return false;
    }

    inline AnyValue JsFunction::get_property(const std::string &key, const AnyValue &thisVal)
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

            auto proto_it = FunctionPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
            return Constants::UNDEFINED;
        }
        return AnyValue::resolve_property_for_read(it->second, thisVal, key);
    }

    inline AnyValue JsFunction::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
    {
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
}