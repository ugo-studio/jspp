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
        if (!dense[i].is_undefined() && !dense[i].is_null())
        {
            result += dense[i].to_std_string();
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
            if (key == "length")
            {
                static AnyValue proto = AnyValue::make_accessor_descriptor([this](const std::vector<AnyValue> &args) -> AnyValue
                                                                           { return AnyValue::make_number(this->length); },
                                                                           std::nullopt,
                                                                           false,
                                                                           false);
                return AnyValue::resolve_property_for_read(proto);
            }
            return AnyValue::make_undefined();
        }
        return AnyValue::resolve_property_for_read(it->second);
    }
}

jspp::AnyValue jspp::JsArray::get_property(uint32_t idx)
{
    if (idx < dense.size())
    {
        return AnyValue::resolve_property_for_read(dense[idx]);
    }
    auto it = sparse.find(idx);
    if (it != sparse.end())
    {
        return AnyValue::resolve_property_for_read(it->second);
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
        // if (key == "length")
        // {
        //     if (value.is_number())
        //     {
        //         double newLenD = value.as_double();
        //         if (newLenD < 0 || std::isnan(newLenD) || std::isinf(newLenD))
        //         {
        //             throw RuntimeError::make_error("Invalid array length", "RangeError");
        //         }
        //         uint64_t newLen = static_cast<uint64_t>(newLenD);
        //         if (newLenD != static_cast<double>(newLen))
        //         {
        //             throw RuntimeError::make_error("Invalid array length", "RangeError");
        //         }
        //         // truncate dense and sparse storage if needed
        //         if (newLen < this->length)
        //         {
        //             // truncate dense
        //             if (newLen < this->dense.size())
        //             {
        //                 this->dense.resize(static_cast<size_t>(newLen));
        //             }
        //             // truncate sparse
        //             for (auto it = this->sparse.begin(); it != this->sparse.end();)
        //             {
        //                 if (it->first >= newLen)
        //                 {
        //                     it = this->sparse.erase(it);
        //                 }
        //                 else
        //                 {
        //                     ++it;
        //                 }
        //             }
        //         }
        //         this->length = newLen;
        //     }
        //     return value;
        // }

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
        return AnyValue::resolve_property_for_write(dense[idx], value);
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
            return AnyValue::resolve_property_for_write(it->second, value);
        }
        else
        {
            sparse[idx] = value;
            return value;
        }
    }
}