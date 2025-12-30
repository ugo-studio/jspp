#pragma once

#include "types.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsObject
    {
        std::map<std::string, AnyValue> props;
        std::shared_ptr<AnyValue> proto;

        JsObject() : proto(nullptr) {}
        explicit JsObject(const std::map<std::string, AnyValue> &p, std::shared_ptr<AnyValue> pr = nullptr) : props(p), proto(pr) {}

        std::string to_std_string() const;
        bool has_property(const std::string &key) const;
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}
