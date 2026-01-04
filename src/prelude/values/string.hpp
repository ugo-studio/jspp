#pragma once

#include "types.hpp"
#include <string>
#include <unordered_map>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsString : HeapObject
    {
        std::string value;

        JsString() = default;
        explicit JsString(const std::string &s) : value(s) {}

        JsType get_heap_type() const override { return JsType::String; }

        std::string to_std_string() const;
        JsIterator<AnyValue> get_iterator();

        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue get_property(uint32_t idx);
    };
}