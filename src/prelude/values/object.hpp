#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    struct JsObject
    {
        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;
        AnyValue &operator[](const std::string &key);
        AnyValue &operator[](const AnyValue &key);
    };

    struct DataDescriptor
    {
        std::shared_ptr<AnyValue> value;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };

    struct AccessorDescriptor
    {
        std::optional<std::function<AnyValue(const std::vector<AnyValue> &)>> get; // getter = function or undefined
        std::optional<std::function<AnyValue(const std::vector<AnyValue> &)>> set; // setter = function or undefined
        bool enumerable = false;
        bool configurable = true;
    };
}
