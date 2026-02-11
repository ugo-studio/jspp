#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "exception.hpp"
#include "any_value.hpp"
#include "values/prototypes/array.hpp"

inline jspp::JsArray::JsArray() : proto(Constants::Null), length(0) {}
inline jspp::JsArray::JsArray(const std::vector<jspp::AnyValue> &items) : dense(items), proto(Constants::Null), length(items.size()) {}
inline jspp::JsArray::JsArray(std::vector<jspp::AnyValue> &&items) : dense(std::move(items)), proto(Constants::Null), length(dense.size()) {}

inline std::string jspp::JsArray::to_std_string() const
{
    if (length == 0)
    {
        return "";
    }

    std::string result = "";
    for (uint64_t i = 0; i < length; ++i)
    {
        AnyValue itemVal = Constants::UNINITIALIZED;
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

        if (!itemVal.is_uninitialized())
        {
            if (!itemVal.is_undefined() && !itemVal.is_null())
            {
                result += itemVal.to_std_string();
            }
        }

        if (i < length - 1)
        {
            result += ",";
        }
    }
    return result;
}

inline jspp::JsIterator<jspp::AnyValue> jspp::JsArray::get_iterator()
{
    for (uint64_t idx = 0; idx < length; ++idx)
    {
        co_yield get_property(static_cast<uint32_t>(idx));
    }

    co_return AnyValue::make_undefined();
}

inline bool jspp::JsArray::has_property(const std::string &key) const
{
    if (key == "length")
        return true;
    if (is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        if (idx < dense.size())
            return true;
        if (sparse.find(idx) != sparse.end())
            return true;
    }
    if (props.find(key) != props.end())
        return true;

    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }

    if (ArrayPrototypes::get(key).has_value())
        return true;
    return false;
}

inline jspp::AnyValue jspp::JsArray::get_property(const std::string &key, const AnyValue &thisVal)
{
    if (
        !key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) 
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
            if (key == "length")
            {
                auto proto_it = ArrayPrototypes::get(key);
                if (proto_it.has_value())
                {
                    return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
                }
            }

            if (!proto.is_null() && !proto.is_undefined())
            {
                if (proto.has_property(key))
                {
                    return proto.get_property_with_receiver(key, thisVal);
                }
            }

            auto proto_it = ArrayPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
            return Constants::UNDEFINED;
        }
        return AnyValue::resolve_property_for_read(it->second, thisVal, key);
    }
}

inline jspp::AnyValue jspp::JsArray::get_property(uint32_t idx)
{
    if (idx < dense.size())
    {
        const auto &val = dense[idx];
        return val.is_uninitialized() ? Constants::UNDEFINED : val;
    }
    const auto &it = sparse.find(idx);
    if (it != sparse.end())
    {
        const auto &val = it->second;
        return val.is_uninitialized() ? Constants::UNDEFINED : val;
    }
    return Constants::UNDEFINED;
}

inline jspp::AnyValue jspp::JsArray::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    if (
        !key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) 
        && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return set_property(idx, value);
    }
    else
    {
        auto proto_val_opt = ArrayPrototypes::get(key);
        
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

inline jspp::AnyValue jspp::JsArray::set_property(uint32_t idx, const AnyValue &value)
{
    uint64_t newLen = static_cast<uint64_t>(idx) + 1;
    if (newLen > length)
        length = newLen;

    const uint32_t DENSE_GROW_THRESHOLD = 1024;
    if (idx < dense.size())
    {
        dense[idx] = value;
        return value;
    }
    else if (idx <= dense.size() + DENSE_GROW_THRESHOLD)
    {
        dense.resize(idx + 1, Constants::UNINITIALIZED);
        dense[idx] = value;
        return value;
    }
    else
    {
        sparse[idx] = value;
        return value;
    }
}