#pragma once

#include "types.hpp"

namespace jspp
{
    class JsValue;

    struct JsFunction
    {
        std::function<JsValue(const std::vector<JsValue> &)> call;
        std::string name;
        std::unordered_map<std::string, JsValue> props;

        std::string to_std_string() const;
        JsValue get_property(const std::string &key);
        JsValue set_property(const std::string &key, const JsValue &value);
    };
}
