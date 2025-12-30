#pragma once

#include "types.hpp"
#include "any_value.hpp"

jspp::AnyValue jspp::AnyValue::get_own_property(const std::string &key) const
{
    return get_property_with_receiver(key, *this);
}
bool jspp::AnyValue::has_property(const std::string &key) const
{
    switch (get_type())
    {
    case JsType::Object:
        return std::get<std::shared_ptr<JsObject>>(storage)->has_property(key);
    case JsType::Array:
        return std::get<std::shared_ptr<JsArray>>(storage)->has_property(key);
    case JsType::Function:
        return std::get<std::shared_ptr<JsFunction>>(storage)->has_property(key);
    case JsType::Promise:
        // Promises don't have their own props usually, but could.
        return false; 
    case JsType::Iterator:
        return false;
    case JsType::Symbol:
        return false;
    case JsType::String:
        if (key == "length") return true;
        if (JsArray::is_array_index(key)) {
            uint32_t idx = static_cast<uint32_t>(std::stoull(key));
            return idx < std::get<std::shared_ptr<JsString>>(storage)->value.length();
        }
        return false;
    case JsType::Number:
        return false;
    default:
        return false;
    }
}
jspp::AnyValue jspp::AnyValue::get_own_property(uint32_t idx) const noexcept
{
    switch (storage.index())
    {
    case 7: // Array
        return std::get<std::shared_ptr<JsArray>>(storage)->get_property(idx);
    case 5: // String
        return std::get<std::shared_ptr<JsString>>(storage)->get_property(idx);
    case 4: // Number
        return get_own_property(std::to_string(idx));
    default:
        return get_own_property(std::to_string(idx));
    }
}
jspp::AnyValue jspp::AnyValue::get_own_property(const AnyValue &key) const noexcept
{
    if (key.is_number() && is_array())
        return std::get<std::shared_ptr<JsArray>>(storage)->get_property(key.as_double());
    if (key.is_number() && is_string())
        return std::get<std::shared_ptr<JsString>>(storage)->get_property(key.as_double());

    // If the key is a Symbol, use its internal key string
    if (key.is_symbol())
        return get_own_property(key.as_symbol()->key);

    return get_own_property(key.to_std_string());
}

jspp::AnyValue jspp::AnyValue::get_property_with_receiver(const std::string &key, const AnyValue &receiver) const
{
    switch (get_type())
    {
    case JsType::Object:
        return std::get<std::shared_ptr<JsObject>>(storage)->get_property(key, receiver);
    case JsType::Array:
        return std::get<std::shared_ptr<JsArray>>(storage)->get_property(key, receiver);
    case JsType::Function:
        return std::get<std::shared_ptr<JsFunction>>(storage)->get_property(key, receiver);
    case JsType::Promise:
        return std::get<std::shared_ptr<JsPromise>>(storage)->get_property(key, receiver);
    case JsType::Iterator:
        return std::get<std::shared_ptr<JsIterator<AnyValue>>>(storage)->get_property(key, receiver);
    case JsType::Symbol:
        return std::get<std::shared_ptr<JsSymbol>>(storage)->get_property(key, receiver);
    case JsType::String:
        return std::get<std::shared_ptr<JsString>>(storage)->get_property(key, receiver);
    case JsType::Number:
    {
        auto proto_it = NumberPrototypes::get(key, std::get<double>(storage));
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
    switch (get_type())
    {
    case JsType::Object:
        return std::get<std::shared_ptr<JsObject>>(storage)->set_property(key, value, *this);
    case JsType::Array:
        return std::get<std::shared_ptr<JsArray>>(storage)->set_property(key, value, *this);
    case JsType::Function:
        return std::get<std::shared_ptr<JsFunction>>(storage)->set_property(key, value, *this);
    case JsType::Promise:
        return std::get<std::shared_ptr<JsPromise>>(storage)->set_property(key, value, *this);
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
    if (is_array())
    {
        return std::get<std::shared_ptr<JsArray>>(storage)->set_property(idx, value);
    }
    return set_own_property(std::to_string(idx), value);
}
jspp::AnyValue jspp::AnyValue::set_own_property(const AnyValue &key, const AnyValue &value) const
{
    if (key.is_number() && is_array())
    {
        return std::get<std::shared_ptr<JsArray>>(storage)->set_property(key.as_double(), value);
    }

    // If the key is a Symbol, use its internal key string
    if (key.is_symbol())
        return set_own_property(key.as_symbol()->key, value);

    return set_own_property(key.to_std_string(), value);
}
