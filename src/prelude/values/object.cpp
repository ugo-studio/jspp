#include "jspp.hpp"
#include "values/object.hpp"
#include "values/prototypes/object.hpp"

namespace jspp
{

    // --- JsObject Implementation ---

    JsObject::JsObject() : shape(Shape::empty_shape()), proto(Constants::Null) {}

    JsObject::JsObject(std::initializer_list<std::pair<std::string, AnyValue>> p, AnyValue pr) : proto(pr)
    {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto &pair : p)
        {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }

    JsObject::JsObject(const std::map<std::string, AnyValue> &p, AnyValue pr) : proto(pr)
    {
        shape = Shape::empty_shape();
        storage.reserve(p.size());
        for (const auto &pair : p)
        {
            shape = shape->transition(pair.first);
            storage.push_back(pair.second);
        }
    }

    std::string JsObject::to_std_string() const
    {
        return "[Object Object]";
    }

    bool JsObject::has_property(const std::string &key) const
    {
        if (deleted_keys.count(key))
            return false;

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

    bool JsObject::has_symbol_property(const AnyValue &key) const
    {
        if (symbol_props.count(key))
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

    AnyValue JsObject::get_property(const std::string &key, const AnyValue &thisVal)
    {
        if (deleted_keys.count(key))
            return Constants::UNDEFINED;

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

    AnyValue JsObject::get_symbol_property(const AnyValue &key, const AnyValue &thisVal)
    {
        auto it = symbol_props.find(key);
        if (it == symbol_props.end())
        {
            if (!proto.is_null() && !proto.is_undefined())
            {
                auto res = proto.get_symbol_property_with_receiver(key, thisVal);
                if (!res.is_undefined())
                    return res;
            }

            auto proto_it = ObjectPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
            }
            return Constants::UNDEFINED;
        }
        return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
    }

    AnyValue JsObject::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
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

        if (deleted_keys.count(key))
            deleted_keys.erase(key);

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

    AnyValue JsObject::set_symbol_property(const AnyValue &key, const AnyValue &value, const AnyValue &thisVal)
    {
        auto it = symbol_props.find(key);
        if (it != symbol_props.end())
        {
            return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
        }
        else
        {
            symbol_props[key] = value;
            return value;
        }
    }

    // --- ObjectPrototypes Implementation ---

    namespace ObjectPrototypes
    {

        AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.to_std_string()); },
                                                         "toString");
            return fn;
        }

        std::optional<AnyValue> get(const std::string &key)
        {
            if (key == "toString")
            {
                return get_toString_fn();
            }
            return std::nullopt;
        }

        std::optional<AnyValue> get(const AnyValue &key)
        {
            if (key.is_string())
                return get(key.as_string()->value);

            if (key == AnyValue::from_symbol(WellKnownSymbols::toStringTag))
            {
                return get_toString_fn();
            }
            return std::nullopt;
        }

    } // namespace ObjectPrototypes

} // namespace jspp
