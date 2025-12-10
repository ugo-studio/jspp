#pragma once

#include "types.hpp"
#include "values/iterator.hpp"
#include "any_value.hpp"

template <typename T>
std::string jspp::JsIterator<T>::to_std_string() const
{
    return "[object Generator]";
}

template <typename T>
jspp::JsIterator<T>::NextResult jspp::JsIterator<T>::next(const T &val)
{
    // If the generator is already finished or invalid, return {undefined, true}
    if (!handle || handle.done())
        return {std::nullopt, true};

    handle.promise().input_value = val;

    // Resume execution until next co_yield or co_return
    handle.resume();

    if (handle.promise().exception_)
    {
        std::rethrow_exception(handle.promise().exception_);
    }

    // If handle.done() is TRUE, we hit co_return (value: X, done: true)
    // If handle.done() is FALSE, we hit co_yield (value: X, done: false)
    bool is_done = handle.done();

    return {handle.promise().current_value, is_done};
}

template <typename T>
std::vector<std::optional<T>> jspp::JsIterator<T>::to_vector()
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
jspp::AnyValue jspp::JsIterator<T>::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        // check prototype
        if constexpr (std::is_same_v<T, AnyValue>)
        {
            auto proto_it = IteratorPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
        }
        // prototype not found
        return jspp::AnyValue::make_undefined();
    }

    return jspp::AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

template <typename T>
jspp::AnyValue jspp::JsIterator<T>::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    // set prototype property if accessor descriptor
    if constexpr (std::is_same_v<T, AnyValue>)
    {
        auto proto_it = IteratorPrototypes::get(key, this);
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
    }

    // set own property
    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    }
    else
    {
        props[key] = value;
        return value;
    }
}
