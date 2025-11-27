#pragma once

#include <variant>
#include "types.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    using JsFunctionCallable = std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>,                                // 0: Normal
                                            std::function<jspp::JsIterator<jspp::AnyValue>(const std::vector<jspp::AnyValue> &)>>; // 1: Generator

    struct JsFunction
    {
        JsFunctionCallable callable;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;

        bool is_generator() const
        {
            return callable.index() == 1;
        }

        std::string to_std_string() const;
        AnyValue call(const std::vector<AnyValue> &args);
        AnyValue get_property(const std::string &key);
        AnyValue set_property(const std::string &key, const AnyValue &value);
    };
}
