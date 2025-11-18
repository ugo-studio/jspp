#pragma once

#include "types.hpp"

namespace jspp
{
    class JsValue;

    struct JsObject
    {
        std::map<std::string, JsValue> props;

        std::string to_std_string() const;
        JsValue get_property(const std::string &key);
        JsValue set_property(const std::string &key, const JsValue &value);
    };
}
