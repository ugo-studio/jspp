#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

namespace jspp {
    inline JsObject::JsObject() : shape(Shape::empty_shape()), proto(Constants::Null) {}

    inline JsObject::JsObject(std::initializer_list<std::pair<std::string, AnyValue>> p, AnyValue pr) : proto(pr) {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto& pair : p) {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }

    inline JsObject::JsObject(const std::map<std::string, AnyValue> &p, AnyValue pr) : proto(pr) {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto& pair : p) {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }
}

inline std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

inline bool jspp::JsObject::has_property(const std::string &key) const
{
    if (deleted_keys.count(key)) return false;

    if (shape->get_offset(key).has_value())
        return true;
    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }
    if (ObjectPrototypes::get(key).has_value())
        return true;
    return false;
}

inline jspp::AnyValue jspp::JsObject::get_property(const std::string &key, const AnyValue &thisVal)
{
    if (deleted_keys.count(key)) return Constants::UNDEFINED;

    auto offset = shape->get_offset(key);
    if (!offset.has_value())
    {
        if (!proto.is_null() && !proto.is_undefined())
        {
            if (proto.has_property(key))
            {
                return proto.get_property_with_receiver(key, thisVal);
            }
        }

        auto proto_it = ObjectPrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(storage[offset.value()], thisVal, key);
}

inline jspp::AnyValue jspp::JsObject::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    auto proto_it = ObjectPrototypes::get(key);
    if (proto_it.has_value())
    {
        auto proto_value = proto_it.value();
        if (proto_value.is_accessor_descriptor())
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
        if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
        {
            return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
    }

    if (deleted_keys.count(key)) deleted_keys.erase(key);

    auto offset = shape->get_offset(key);
    if (offset.has_value())
    {
        return AnyValue::resolve_property_for_write(storage[offset.value()], thisVal, value, key);
    }
    else
    {
        shape = shape->transition(key);
        storage.push_back(value);
        return value;
    }
}
