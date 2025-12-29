#pragma once

#include "types.hpp"
#include "any_value.hpp"

jspp::AnyValue jspp::AnyValue::get_own_property(const std::string &key) const
{
    return get_property_with_receiver(key, *this);
}
jspp::AnyValue jspp::AnyValue::get_own_property(uint32_t idx) const noexcept
{
    switch (storage.type)
    {
    case JsType::Array:
        return storage.array->get_property(idx);
    case JsType::String:
        return storage.str->get_property(idx);
    case JsType::Number:
        return get_own_property(std::to_string(idx));
    default:
        return get_own_property(std::to_string(idx));
    }
}
jspp::AnyValue jspp::AnyValue::get_own_property(const AnyValue &key) const noexcept
{
    if (key.storage.type == JsType::Number && storage.type == JsType::Array)
        return storage.array->get_property(key.storage.number);
    if (key.storage.type == JsType::Number && storage.type == JsType::String)
        return storage.str->get_property(key.storage.number);

    // If the key is a Symbol, use its internal key string
    if (key.storage.type == JsType::Symbol)
        return get_own_property(key.storage.symbol->key);

    return get_own_property(key.to_std_string());
}

jspp::AnyValue jspp::AnyValue::get_property_with_receiver(const std::string &key, const AnyValue &receiver) const
{
    switch (storage.type)
    {
    case JsType::Object:
        return storage.object->get_property(key, receiver);
    case JsType::Array:
        return storage.array->get_property(key, receiver);
    case JsType::Function:
        return storage.function->get_property(key, receiver);
    case JsType::Promise:
        return storage.promise->get_property(key, receiver);
    case JsType::Iterator:
        return storage.iterator->get_property(key, receiver);
    case JsType::Symbol:
        return storage.symbol->get_property(key, receiver);
    case JsType::String:
        return storage.str->get_property(key, receiver);
    case JsType::Number:
    {
        auto proto_it = NumberPrototypes::get(key, storage.number);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key);
        }
        return AnyValue::make_undefined();
    }
    case JsType::Undefined:
        throw Exception::make_exception("Cannot read properties of undefined (reading '" + key + "')", "TypeError");
    case JsType::Null:
        throw Exception::make_exception("Cannot read properties of null (reading '" + key + "')", "TypeError");
    default:
        return AnyValue::make_undefined();
    }
}

jspp::AnyValue jspp::AnyValue::set_own_property(const std::string &key, const AnyValue &value) const
{
    switch (storage.type)
    {
    case JsType::Object:
        return storage.object->set_property(key, value, *this);
    case JsType::Array:
        return storage.array->set_property(key, value, *this);
    case JsType::Function:
        return storage.function->set_property(key, value, *this);
    case JsType::Promise:
        return storage.promise->set_property(key, value, *this);
    case JsType::Undefined:
        throw Exception::make_exception("Cannot set properties of undefined (setting '" + key + "')", "TypeError");
    case JsType::Null:
        throw Exception::make_exception("Cannot set properties of null (setting '" + key + "')", "TypeError");
    default:
        return value;
    }
}
jspp::AnyValue jspp::AnyValue::set_own_property(uint32_t idx, const AnyValue &value) const
{
    if (storage.type == JsType::Array)
    {
        return storage.array->set_property(idx, value);
    }
    return set_own_property(std::to_string(idx), value);
}
jspp::AnyValue jspp::AnyValue::set_own_property(const AnyValue &key, const AnyValue &value) const
{
    if (key.storage.type == JsType::Number && storage.type == JsType::Array)
    {
        return storage.array->set_property(key.storage.number, value);
    }

    // If the key is a Symbol, use its internal key string
    if (key.storage.type == JsType::Symbol)
        return set_own_property(key.storage.symbol->key, value);

    return set_own_property(key.to_std_string(), value);
}
