#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "error.hpp"
#include "any_value.hpp"
#include "values/prototypes/array.hpp"

std::string jspp::JsArray::to_std_string() const
{
    if (length == 0)
    {
        return "";
    }

    std::string result = "";
    for (uint64_t i = 0; i < length; ++i)
    {
        std::optional<AnyValue> itemVal;
        if (i < dense.size())
        {
            itemVal = dense[i];
        }
        else
        {
            auto it = sparse.find(static_cast<uint32_t>(i));
            if (it != sparse.end())
            {
                itemVal = it->second;
            }
        }

        if (itemVal.has_value())
        {
            const auto &item = itemVal.value();
            if (!item.is_undefined() && !item.is_null())
            {
                result += item.to_std_string();
            }
        }

        if (i < length - 1)
        {
            result += ",";
        }
    }
    return result;
}

jspp::JsIterator<jspp::AnyValue> jspp::JsArray::get_iterator()
{
    for (uint64_t idx = 0; idx < length; ++idx)
    {
        co_yield get_property(static_cast<uint32_t>(idx));
    }

    co_return AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsArray::get_property(const std::string &key, const AnyValue &thisVal)
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
            auto proto_it = ArrayPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
            // not found
            return AnyValue::make_undefined();
        }
        return AnyValue::resolve_property_for_read(it->second, thisVal, key);
    }
}

jspp::AnyValue jspp::JsArray::get_property(uint32_t idx)
{
    if (idx < dense.size())
    {
        return dense[idx].value_or(AnyValue::make_undefined());
    }
    const auto &it = sparse.find(idx);
    if (it != sparse.end())
    {
        return it->second.value_or(AnyValue::make_undefined());
    }
    return AnyValue::make_undefined();
}

jspp::AnyValue jspp::JsArray::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
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
        auto proto_val_opt = ArrayPrototypes::get(key, this);
        if (proto_val_opt.has_value())
        {
            auto proto_value = proto_val_opt.value();
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
        dense[idx] = value;
        return value;
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
            it->second = value;
            return value;
        }
        else
        {
            sparse[idx] = value;
            return value;
        }
    }
}