#pragma once

#include "types.hpp"

namespace jspp
{
    class AnyValue;

    struct JsArray
    {
        std::vector<AnyValue> dense;                     // dense storage for small/contiguous indices
        std::unordered_map<uint32_t, AnyValue> sparse;   // sparse indices (very large indices)
        std::unordered_map<std::string, AnyValue> props; // non-index string properties
        uint64_t length = 0;

        static bool isArrayIndex(const std::string &s)
        {
            if (s.empty())
                return false;
            // must be all digits
            for (char c : s)
                if (!std::isdigit(static_cast<unsigned char>(c)))
                    return false;
            // reject the special 2^32-1 string
            if (s == "4294967295")
                return false;
            // parse without overflow up to 2^32-1
            uint64_t v = 0;
            for (char c : s)
            {
                v = v * 10 + (c - '0');
                if (v >= (1ULL << 32))
                    return false;
            }
            // ToString(ToUint32(v)) === s?
            return std::to_string(static_cast<uint32_t>(v)) == s;
        }

        std::string to_std_string() const;
        AnyValue &operator[](const std::string &key);
        AnyValue &operator[](uint32_t idx);
        AnyValue &operator[](const AnyValue &key);
    };
}
