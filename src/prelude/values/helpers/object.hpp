#pragma once

#include "types.hpp"
#include "values/object.hpp"
#include "any_value.hpp"

namespace jspp {
    JsObject::JsObject() : shape(Shape::empty_shape()), proto(nullptr) {}

    JsObject::JsObject(std::initializer_list<std::pair<std::string, AnyValue>> p, std::shared_ptr<AnyValue> pr) : proto(pr) {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto& pair : p) {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }

    JsObject::JsObject(const std::map<std::string, AnyValue> &p, std::shared_ptr<AnyValue> pr) : proto(pr) {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto& pair : p) {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }
}

std::string jspp::JsObject::to_std_string() const
{
    return "[Object Object]";
}

bool jspp::JsObject::has_property(const std::string &key) const
{
    if (shape->get_offset(key).has_value())
        return true;
    if (proto && !(*proto).is_null() && !(*proto).is_undefined())
    {
        if ((*proto).has_property(key))
            return true;
    }
    if (ObjectPrototypes::get(key, const_cast<JsObject *>(this)).has_value())
        return true;
    return false;
}

jspp::AnyValue jspp::JsObject::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto offset = shape->get_offset(key);
    if (!offset.has_value())
    {
        // check prototype chain
        if (proto && !(*proto).is_null() && !(*proto).is_undefined())
        {
            if ((*proto).has_property(key))
            {
                return (*proto).get_property_with_receiver(key, thisVal);
            }
        }

        // check built-in prototype methods (Object.prototype)
        auto proto_it = ObjectPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        // not found
        return AnyValue::make_undefined();
    }
    return AnyValue::resolve_property_for_read(storage[offset.value()], thisVal, key);
}

jspp::AnyValue jspp::JsObject::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    // set prototype property if accessor descriptor
    auto proto_it = ObjectPrototypes::get(key, this);
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

    // set own property
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
