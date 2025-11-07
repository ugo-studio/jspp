#pragma once

#include "types.hpp"

namespace jspp
{
    namespace Convert
    {
        inline std::string to_string(const AnyValue &val);
    }

    struct JsArray
    {
        std::vector<AnyValue> dense;                     // dense storage for small/contiguous indices
        std::unordered_map<uint32_t, AnyValue> sparse;   // sparse indices (very large indices)
        std::unordered_map<std::string, AnyValue> props; // non-index string properties
        uint64_t length = 0;

        std::string to_std_string() const
        {
            std::string result = "";
            for (size_t i = 0; i < dense.size(); ++i)
            {
                if (!std::holds_alternative<JsUndefined>(dense[i]) && !std::holds_alternative<JsNull>(dense[i]))
                {
                    result += Convert::to_string(dense[i]);
                }
                if (i < dense.size() - 1)
                {
                    result += ",";
                }
            }
            return result;
        }

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

        // bracket by string: mimic JS arr["0"] vs arr["00"]
        AnyValue &operator[](const std::string &key)
        {
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
        AnyValue &operator[](uint32_t idx)
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
    };
}
