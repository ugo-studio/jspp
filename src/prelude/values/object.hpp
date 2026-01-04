#pragma once

#include "types.hpp"
#include "shape.hpp"
#include <vector>
#include <unordered_set>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsObject : HeapObject
    {
        std::shared_ptr<Shape> shape;
        std::vector<AnyValue> storage;
        AnyValue proto;
        std::unordered_set<std::string> deleted_keys;

        JsObject();
        JsObject(std::initializer_list<std::pair<std::string, AnyValue>> p, AnyValue pr);
        JsObject(const std::map<std::string, AnyValue> &p, AnyValue pr);

        JsType get_heap_type() const override { return JsType::Object; }

        std::string to_std_string() const;
        bool has_property(const std::string &key) const;
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}
