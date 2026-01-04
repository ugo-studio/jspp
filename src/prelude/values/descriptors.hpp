#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    struct DataDescriptor : HeapObject
    {
        AnyValue value;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;

        DataDescriptor(AnyValue v, bool w, bool e, bool c) 
            : value(v), writable(w), enumerable(e), configurable(c) {}

        JsType get_heap_type() const override { return JsType::DataDescriptor; }
    };

    struct AccessorDescriptor : HeapObject
    {
        std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> get; // getter = function or undefined
        std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> set; // setter = function or undefined
        bool enumerable = false;
        bool configurable = true;

        AccessorDescriptor(std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> g,
                           std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> s,
                           bool e, bool c)
            : get(std::move(g)), set(std::move(s)), enumerable(e), configurable(c) {}

        JsType get_heap_type() const override { return JsType::AccessorDescriptor; }
    };
}