#pragma once

#include "types.hpp"
#include "convert.hpp"
#include "well_known_symbols.hpp"
#include "default_prototypes.hpp"

namespace jspp
{
    namespace Prototype
    {
        using PrototypeMap = std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>>;

        inline bool setter_exists(const AnyValue &obj,
                                  const AnyValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_object_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_array_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_function_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_number_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_string_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return true;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_boolean_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
            throw Exception::make_error_with_name("Cannot read properties of non-object type", "TypeError");
        }

        inline AnyValue get_prototype(
            const AnyValue &obj,
            const AnyValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_object_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_array_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_function_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_number_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_string_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        const auto &data_desc = std::get<DataDescriptor>(prop);
                        if (data_desc.value.type() == typeid(std::function<AnyValue(const std::vector<AnyValue> &)>))
                        {
                            auto fn = std::any_cast<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.value);
                            return fn({});
                        }
                        return data_desc.value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(prop))
                    {
                        return std::get<AnyValue>(prop);
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_boolean_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            throw Exception::make_error_with_name("Cannot read properties of non-object type", "TypeError");
        }

        inline AnyValue set_prototype(
            const AnyValue &obj,
            const AnyValue &key,
            const AnyValue &val)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_object_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_array_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_function_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_number_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_string_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, AnyValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                            return val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                auto def_proto_it = PrototypeDefaults::get_boolean_prototye(obj, key_str);
                if (def_proto_it.has_value())
                {
                    const auto &prop = def_proto_it.value();
                    if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set))
                        {
                            std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(accessor.set)({val});
                            return val;
                        }
                    }
                }
                // Set the prototype of this specific obj
                ptr->prototype[key_str] = DataDescriptor{val};
                return val;
            }
            throw Exception::make_error_with_name("Cannot set properties of non-object type", "TypeError");
        }

        // inline void set_data_property(
        //     PrototypeMap &prototype,
        //     const std::string &name,
        //     const AnyValue &value,
        //     bool writable = true,
        //     bool enumerable = false,
        //     bool configurable = true)
        // {
        //     prototype[name] = DataDescriptor{value, writable, enumerable, configurable};
        // }

        // inline void set_accessor_property(
        //     PrototypeMap &prototype,
        //     const std::string &name,
        //     const std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, Undefined> &getter,
        //     const std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, Undefined> &setter,
        //     bool enumerable = false,
        //     bool configurable = true)
        // {
        //     prototype[name] = AccessorDescriptor{getter, setter, enumerable, configurable};
        // }
    }
}
