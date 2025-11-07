#pragma once

#include "types.hpp"

namespace jspp
{
    struct DataDescriptor
    {
        AnyValue value = NonValues::undefined;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };

    struct AccessorDescriptor
    {
        std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, JsUndefined> get = NonValues::undefined; // getter
        std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, JsUndefined> set = NonValues::undefined; // setter
        bool enumerable = false;
        bool configurable = true;
    };
}
