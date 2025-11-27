#pragma once

#include "types.hpp"
#include <string>
#include <unordered_map>

namespace jspp
{
    class AnyValue;

    struct JsString
    {
        std::string value;

        JsString() = default;
        explicit JsString(const std::string &s) : value(s) {}

        std::string to_std_string() const;
        JsIterator<AnyValue> get_iterator();

        AnyValue get_property(const std::string &key);
        AnyValue get_property(uint32_t idx);
    };
}