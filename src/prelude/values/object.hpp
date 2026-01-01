#pragma once

#include "types.hpp"
#include "shape.hpp"
#include <vector>
#include <unordered_set>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsObject
    {
        std::shared_ptr<Shape> shape;
        std::vector<AnyValue> storage;
        std::shared_ptr<AnyValue> proto;
        std::unordered_set<std::string> deleted_keys;

        JsObject();
        JsObject(std::initializer_list<std::pair<std::string, AnyValue>> p, std::shared_ptr<AnyValue> pr = nullptr);
        JsObject(const std::map<std::string, AnyValue> &p, std::shared_ptr<AnyValue> pr = nullptr);

        std::string to_std_string() const;
        bool has_property(const std::string &key) const;
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}
