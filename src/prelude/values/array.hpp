#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsArray
    {
        std::vector<AnyValue> dense;                     // dense storage for small/contiguous indices
        std::unordered_map<uint32_t, AnyValue> sparse;   // sparse indices (very large indices)
        std::unordered_map<std::string, AnyValue> props; // non-index string properties
        std::shared_ptr<AnyValue> proto;
        uint64_t length = 0;

        JsArray() : proto(nullptr) {}
        explicit JsArray(const std::vector<AnyValue> &items) : dense(items), proto(nullptr), length(items.size()) {}
        explicit JsArray(std::vector<AnyValue> &&items) : dense(std::move(items)), proto(nullptr), length(dense.size()) {}

        std::string to_std_string() const;
        JsIterator<AnyValue> get_iterator();

        bool has_property(const std::string &key) const;
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue get_property(uint32_t idx);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
        AnyValue set_property(uint32_t idx, const AnyValue &value);

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
