#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class JsValue;

    struct JsArray
    {
        std::vector<std::optional<JsValue>> dense;                   // dense storage for small/contiguous indices
        std::unordered_map<uint32_t, std::optional<JsValue>> sparse; // sparse indices (very large indices)
        std::unordered_map<std::string, JsValue> props;              // non-index string properties
        uint64_t length = 0;

        JsArray() = default;
        explicit JsArray(const std::vector<std::optional<JsValue>> &items) : dense(items), length(items.size()) {}

        std::string to_std_string() const;

        JsValue get_property(const std::string &key);
        JsValue get_property(uint32_t idx);
        JsValue set_property(const std::string &key, const JsValue &value);
        JsValue set_property(uint32_t idx, const JsValue &value);

        static bool is_array_index(const std::string &s)
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
    };
}
