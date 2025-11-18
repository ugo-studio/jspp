#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class JsValue;

    struct DataDescriptor
    {
        std::shared_ptr<JsValue> value;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };

    struct AccessorDescriptor
    {
        std::optional<std::function<JsValue(const std::vector<JsValue> &)>> get; // getter = function or undefined
        std::optional<std::function<JsValue(const std::vector<JsValue> &)>> set; // setter = function or undefined
        bool enumerable = false;
        bool configurable = true;
    };
}