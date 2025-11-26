#pragma once

#include "types.hpp"
#include "values/generator.hpp"
#include "any_value.hpp"

template <typename T>
std::string jspp::JsGenerator<T>::to_std_string() const
{
    return "[object Generator]";
}

template <typename T>
jspp::JsGenerator<T>::NextResult jspp::JsGenerator<T>::next()
{
    // If the generator is already finished or invalid, return {undefined, true}
    if (!handle || handle.done())
        return {std::nullopt, true};

    // Resume execution until next co_yield or co_return
    handle.resume();

    if (handle.promise().exception_)
    {
        std::rethrow_exception(handle.promise().exception_);
    }

    // If handle.done() is TRUE, we hit co_return (value: X, done: true)
    // If handle.done() is FALSE, we hit co_yield (value: X, done: false)
    bool is_done = handle.done();

    return {std::move(handle.promise().current_value), is_done};
}

template <typename T>
std::vector<std::optional<T>> jspp::JsGenerator<T>::to_vector()
{
    std::vector<std::optional<AnyValue>> result;
    while (true)
    {
        auto next = this->next();
        if (next.done)
        {
            break;
        }
        result.push_back(next.value);
    }
    return result;
}

template <typename T>
jspp::AnyValue jspp::JsGenerator<T>::get_property(const std::string &key)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // check prototype
        if constexpr (std::is_same_v<T, AnyValue>)
        {
            auto proto_it = GeneratorPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value());
            }
        }
        // prototype not found
        return jspp::AnyValue::make_undefined();
    }

    return jspp::AnyValue::resolve_property_for_read(it->second);
}

template <typename T>
jspp::AnyValue jspp::JsGenerator<T>::set_property(const std::string &key, const AnyValue &value)
{
    // set prototype property if accessor descriptor
    if constexpr (std::is_same_v<T, AnyValue>)
    {
        auto proto_it = GeneratorPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor())
            {
                return AnyValue::resolve_property_for_write(proto_it.value(), value);
            }
        }
    }

    // set own property
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_write(it->second, value);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
