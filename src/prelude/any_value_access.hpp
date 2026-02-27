#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp
{
    inline AnyValue AnyValue::get_own_property(const std::string &key) const
    {
        return get_property_with_receiver(key, *this);
    }
    inline AnyValue AnyValue::get_own_property_descriptor(const AnyValue &key) const
    {
        if (key.is_symbol())
        {
            switch (get_type())
            {
            case JsType::Object:
            {
                auto obj = as_object();
                auto it = obj->symbol_props.find(key);
                if (it != obj->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            case JsType::Array:
            {
                auto arr = as_array();
                auto it = arr->symbol_props.find(key);
                if (it != arr->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            case JsType::Function:
            {
                auto func = as_function();
                auto it = func->symbol_props.find(key);
                if (it != func->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            default:
                return Constants::UNDEFINED;
            }
        }

        std::string key_str = key.to_std_string();
        switch (get_type())
        {
        case JsType::Object:
        {
            auto obj = as_object();
            if (obj->deleted_keys.count(key_str))
                return Constants::UNDEFINED;
            auto offset = obj->shape->get_offset(key_str);
            if (offset.has_value())
                return obj->storage[offset.value()];
            return Constants::UNDEFINED;
        }
        case JsType::Array:
        {
            auto arr = as_array();
            if (key_str == "length")
                return AnyValue::make_number(arr->length);
            if (JsArray::is_array_index(key_str))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key_str));
                if (idx < arr->dense.size() && !arr->dense[idx].is_uninitialized())
                    return arr->dense[idx];
                if (arr->sparse.count(idx))
                    return arr->sparse[idx];
            }
            if (arr->props.count(key_str))
                return arr->props.at(key_str);
            return Constants::UNDEFINED;
        }
        case JsType::Function:
        {
            auto func = as_function();
            if (func->props.count(key_str))
                return func->props.at(key_str);
            return Constants::UNDEFINED;
        }
        case JsType::String:
        {
            auto str = as_string();
            if (key_str == "length")
                return AnyValue::make_number(str->value.length());
            if (JsArray::is_array_index(key_str))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key_str));
                if (idx < str->value.length())
                    return AnyValue::make_string(std::string(1, str->value[idx]));
            }
            return Constants::UNDEFINED;
        }
        default:
            return Constants::UNDEFINED;
        }
    }
    inline bool AnyValue::has_property(const std::string &key) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->has_property(key);
        case JsType::Array:
            return as_array()->has_property(key);
        case JsType::Function:
            return as_function()->has_property(key);
        case JsType::Promise:
            return as_promise()->get_property(key, *this).is_undefined() == false;
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->get_property(key, *this).is_undefined() == false;
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->get_property(key, *this).is_undefined() == false;
        case JsType::Symbol:
            return SymbolPrototypes::get(key).has_value();
        case JsType::String:
            if (key == "length")
                return true;
            if (JsArray::is_array_index(key))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key));
                return idx < as_string()->value.length();
            }
            return StringPrototypes::get(key).has_value();
        case JsType::Number:
            return NumberPrototypes::get(key).has_value();
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
            return false;
        default:
            return false;
        }
    }
    inline bool AnyValue::has_property(const AnyValue &key) const
    {
        if (key.is_symbol())
        {
            switch (get_type())
            {
            case JsType::Object:
                return as_object()->has_symbol_property(key);
            case JsType::Array:
                return as_array()->has_symbol_property(key);
            case JsType::Function:
                return as_function()->has_symbol_property(key);
            case JsType::Promise:
                return as_promise()->has_symbol_property(key);
            case JsType::Iterator:
                return static_cast<JsIterator<AnyValue> *>(get_ptr())->has_symbol_property(key);
            case JsType::AsyncIterator:
                return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->has_symbol_property(key);
            default:
                return false;
            }
        }
        return has_property(key.to_std_string());
    }
    inline AnyValue AnyValue::get_own_property(uint32_t idx) const
    {
        if (is_array()) return as_array()->get_property(idx);
        if (is_string()) return as_string()->get_property(idx);
        return get_own_property(std::to_string(idx));
    }
    inline AnyValue AnyValue::get_own_property(const AnyValue &key) const
    {
        if (key.is_number() && is_array())
            return as_array()->get_property(key.as_double());
        if (key.is_number() && is_string())
            return as_string()->get_property(key.as_double());

        if (key.is_symbol())
            return get_own_symbol_property(key);

        return get_own_property(key.to_std_string());
    }
    inline AnyValue AnyValue::get_own_symbol_property(const AnyValue &key) const
    {
        return get_symbol_property_with_receiver(key, *this);
    }

    inline AnyValue AnyValue::get_property_with_receiver(const std::string &key, AnyValue receiver) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->get_property(key, receiver);
        case JsType::Array:
            return as_array()->get_property(key, receiver);
        case JsType::Function:
            return as_function()->get_property(key, receiver);
        case JsType::Promise:
            return as_promise()->get_property(key, receiver);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue>*>(get_ptr())->get_property(key, receiver);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue>*>(get_ptr())->get_property(key, receiver);
        case JsType::Symbol:
            return as_symbol()->get_property(key, receiver);
        case JsType::String:
            return as_string()->get_property(key, receiver);
        case JsType::Number:
        {
            auto proto_it = NumberPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key);
            }
            return Constants::UNDEFINED;
        }
        case JsType::Undefined:
            throw Exception::make_exception("Cannot read properties of undefined (reading '" + key + "')", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot read properties of null (reading '" + key + "')", "TypeError");
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return Constants::UNDEFINED;
        }
    }

    inline AnyValue AnyValue::get_symbol_property_with_receiver(const AnyValue &key, AnyValue receiver) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->get_symbol_property(key, receiver);
        case JsType::Array:
            return as_array()->get_symbol_property(key, receiver);
        case JsType::Function:
            return as_function()->get_symbol_property(key, receiver);
        case JsType::Promise:
            return as_promise()->get_symbol_property(key, receiver);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->get_symbol_property(key, receiver);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->get_symbol_property(key, receiver);
        case JsType::Symbol:
        {
            auto proto_it = SymbolPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            }
            return Constants::UNDEFINED;
        }
        case JsType::String:
        {
            auto proto_it = StringPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            }
            return Constants::UNDEFINED;
        }
        case JsType::Number:
        {
            auto proto_it = NumberPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            }
            return Constants::UNDEFINED;
        }
        case JsType::Undefined:
            throw Exception::make_exception("Cannot read properties of undefined (reading Symbol)", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot read properties of null (reading Symbol)", "TypeError");
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return Constants::UNDEFINED;
        }
    }

    inline AnyValue AnyValue::set_own_property(const std::string &key, AnyValue value) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->set_property(key, value, *this);
        case JsType::Array:
            return as_array()->set_property(key, value, *this);
        case JsType::Function:
            return as_function()->set_property(key, value, *this);
        case JsType::Promise:
            return as_promise()->set_property(key, value, *this);
        case JsType::Undefined:
            throw Exception::make_exception("Cannot set properties of undefined (setting '" + key + "')", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot set properties of null (setting '" + key + "')", "TypeError");
        default:
            return value;
        }
    }
    inline AnyValue AnyValue::set_own_property(uint32_t idx, AnyValue value) const
    {
        if (is_array())
        {
            return as_array()->set_property(idx, value);
        }
        return set_own_property(std::to_string(idx), value);
    }
    inline AnyValue AnyValue::set_own_property(const AnyValue &key, AnyValue value) const
    {
        if (key.is_number() && is_array())
        {
            return as_array()->set_property(key.as_double(), value);
        }

        if (key.is_symbol())
            return set_own_symbol_property(key, value);

        return set_own_property(key.to_std_string(), value);
    }

    inline AnyValue AnyValue::set_own_symbol_property(const AnyValue &key, AnyValue value) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->set_symbol_property(key, value, *this);
        case JsType::Array:
            return as_array()->set_symbol_property(key, value, *this);
        case JsType::Function:
            return as_function()->set_symbol_property(key, value, *this);
        case JsType::Promise:
            return as_promise()->set_symbol_property(key, value, *this);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->set_symbol_property(key, value, *this);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->set_symbol_property(key, value, *this);
        case JsType::Undefined:
            throw Exception::make_exception("Cannot set properties of undefined (setting Symbol)", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot set properties of null (setting Symbol)", "TypeError");
        default:
            return value;
        }
    }

    inline AnyValue AnyValue::call_own_property(const std::string &key, std::span<const AnyValue> args) const
    {
        return get_own_property(key).call((*this), args, key);
    }
    inline AnyValue AnyValue::call_own_property(uint32_t idx, std::span<const AnyValue> args) const
    {
        if (is_array()) return as_array()->get_property(idx).call((*this), args, "[" + std::to_string(idx) + "]");
        if (is_string()) return as_string()->get_property(idx).call((*this), args, "[" + std::to_string(idx) + "]");
        return call_own_property(std::to_string(idx), args);
    }
    inline AnyValue AnyValue::call_own_property(const AnyValue &key, std::span<const AnyValue> args) const
    {
        if (key.is_number() && is_array())
            return as_array()->get_property(key.as_double()).call((*this), args, "[" + key.to_std_string() + "]");
        if (key.is_number() && is_string())
            return as_string()->get_property(key.as_double()).call((*this), args, "[" + key.to_std_string() + "]");

        if (key.is_symbol())
            return get_own_symbol_property(key).call((*this), args, key.to_std_string());

        return call_own_property(key.to_std_string(), args);
    }
}
