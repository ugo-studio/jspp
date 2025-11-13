#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "values/any_value.hpp"
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

// bracket by string: mimic JS arr["0"] vs arr["00"]
jspp::AnyValue &jspp::JsArray::operator[](const std::string &key)
{
    if (
        !key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) // Quick check: if the first character is not a digit, it can't be a standard index.
        && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return (*this)[idx];
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
                                                                           [this](const std::vector<AnyValue> &args) -> AnyValue
                                                                           {
                                                                               if (args.size() > 0 && args[0].is_number())
                                                                               {
                                                                                   double newLenD = args[0].as_double();
                                                                                   if (newLenD < 0 || std::isnan(newLenD) || std::isinf(newLenD))
                                                                                   {
                                                                                       throw RuntimeError::make_error("Invalid array length", "RangeError");
                                                                                   }
                                                                                   uint64_t newLen = static_cast<uint64_t>(newLenD);
                                                                                   if (newLenD != static_cast<double>(newLen))
                                                                                   {
                                                                                       throw RuntimeError::make_error("Invalid array length", "RangeError");
                                                                                   }
                                                                                   // truncate dense and sparse storage if needed
                                                                                   if (newLen < this->length)
                                                                                   {
                                                                                       // truncate dense
                                                                                       if (newLen < this->dense.size())
                                                                                       {
                                                                                           this->dense.resize(static_cast<size_t>(newLen));
                                                                                       }
                                                                                       // truncate sparse
                                                                                       for (auto it = this->sparse.begin(); it != this->sparse.end();)
                                                                                       {
                                                                                           if (it->first >= newLen)
                                                                                           {
                                                                                               it = this->sparse.erase(it);
                                                                                           }
                                                                                           else
                                                                                           {
                                                                                               ++it;
                                                                                           }
                                                                                       }
                                                                                   }
                                                                                   this->length = newLen;
                                                                               }
                                                                               return AnyValue::make_undefined();
                                                                           },
                                                                           false,
                                                                           false);
                return proto;
            }
            // std::unordered_map::operator[] default-constructs AnyValue (which is Undefined)
            return props[key];
        }
        return it->second;
    }
}

// bracket by numeric index
jspp::AnyValue &jspp::JsArray::operator[](uint32_t idx)
{
    // update length like JS does
    uint64_t newLen = static_cast<uint64_t>(idx) + 1;
    if (newLen > length)
        length = newLen;

    // cheap heuristic: keep reasonably close indices in vector
    const uint32_t DENSE_GROW_THRESHOLD = 1024; // tune to your use-case
    if (idx < dense.size())
    {
        return dense[idx];
    }
    else if (idx <= dense.size() + DENSE_GROW_THRESHOLD)
    {
        dense.resize(idx + 1); // fill with default-constructed T
        return dense[idx];
    }
    else
    {
        // very large/sparse index → use hash map
        return sparse[idx]; // creates default in map
    }
}

jspp::AnyValue &jspp::JsArray::operator[](const AnyValue &key)
{
    return (*this)[key.to_std_string()];
}