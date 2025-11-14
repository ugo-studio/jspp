#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "any_value.hpp"
#include "error.hpp"

std::string jspp::JsArray::to_std_string() const
{
    std::string result = "";
    for (size_t i = 0; i < dense.size(); ++i)
    {
        if (dense[i].has_value())
        {
            const auto &item = dense[i].value();
            if (!item.is_undefined() && !item.is_null())
            {
                result += item.to_std_string();
            }
        }
        if (i < dense.size() - 1)
        {
            result += ",";
        }
    }
    return result;
}

jspp::AnyValue jspp::JsArray::get_property(const std::string &key)
{
    if (
        !key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) // Quick check: if the first character is not a digit, it can't be a standard index.
        && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return get_property(idx);
    }
    else
    {
        auto it = props.find(key);
        if (it == props.end())
        {
            // check prototype
            auto proto_it = get_prototype(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value());
            }
            // not found
            return AnyValue::make_undefined();
        }
        return AnyValue::resolve_property_for_read(it->second);
    }
}

jspp::AnyValue jspp::JsArray::get_property(uint32_t idx)
{
    if (idx < dense.size())
    {
        return AnyValue::resolve_property_for_read(dense[idx].value_or(AnyValue::make_undefined()));
    }
    const auto &it = sparse.find(idx);
    if (it != sparse.end())
    {
        return AnyValue::resolve_property_for_read(it->second.value_or(AnyValue::make_undefined()));
    }
    return AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsArray::set_property(const std::string &key, const AnyValue &value)
{
    if (
        !key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) // Quick check: if the first character is not a digit, it can't be a standard index.
        && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return set_property(idx, value);
    }
    else
    {
        // set prototype property if accessor descriptor
        auto proto_it = get_prototype(key);
        if (proto_it.has_value())
        {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor())
            {
                return AnyValue::resolve_property_for_read(proto_it.value());
            }
        }

        // set own property
        auto it = props.find(key);
        if (it != props.end())
        {
            return AnyValue::resolve_property_for_write(it->second, value);
        }
        else
        {
            props[key] = value;
            return value;
        }
    }
}

jspp::AnyValue jspp::JsArray::set_property(uint32_t idx, const AnyValue &value)
{
    uint64_t newLen = static_cast<uint64_t>(idx) + 1;
    if (newLen > length)
        length = newLen;

    const uint32_t DENSE_GROW_THRESHOLD = 1024;
    if (idx < dense.size())
    {
        if (!dense[idx].has_value())
        {
            dense[idx] = AnyValue::make_undefined();
        }
        return AnyValue::resolve_property_for_write(dense[idx].value(), value);
    }
    else if (idx <= dense.size() + DENSE_GROW_THRESHOLD)
    {
        dense.resize(idx + 1);
        dense[idx] = value;
        return value;
    }
    else
    {
        auto it = sparse.find(idx);
        if (it != sparse.end())
        {
            if (!it->second.has_value())
            {
                it->second = AnyValue::make_undefined();
            }
            return AnyValue::resolve_property_for_write(it->second.value(), value);
        }
        else
        {
            sparse[idx] = value;
            return value;
        }
    }
}