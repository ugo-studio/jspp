#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "values/any_value.hpp"

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
    // Quick check: if the first character is not a digit, it can't be a standard index.
    if (key.empty() || !std::isdigit(static_cast<unsigned char>(key[0])))
    {
        return props[key];
    }

    if (isArrayIndex(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return (*this)[idx];
    }
    else
    {
        // non-index property — stored in props
        // creates default if missing (like JS arr["foo"] = ...)
        return props[key];
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