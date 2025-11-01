#pragma once

#include "types.hpp"
#include "exception.hpp"
#include "convert.hpp"
#include "object.hpp"

namespace jspp
{
    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline JsValue deref(const std::shared_ptr<JsValue> &var, const std::string &name)
        {
            if (!var)
                // This case should ideally not be hit in normal operation
                throw Exception::make_error_with_name("null variable pointer for " + name, "Internal compiler error");
            if ((*var).type() == typeid(Uninitialized))
                throw Exception::make_error_with_name("Cannot access '" + name + "' before initialization", "ReferenceError");
            return *var;
        }

        inline std::vector<std::string> get_object_keys(const JsValue &obj)
        {
            std::vector<std::string> keys;
            std::set<std::string> seen_keys; // To handle duplicates and shadowing

            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (ptr)
                {
                    // Add own properties first
                    for (const auto &pair : ptr->properties)
                    {
                        if (seen_keys.find(pair.first) == seen_keys.end())
                        {
                            keys.push_back(pair.first);
                            seen_keys.insert(pair.first);
                        }
                    }

                    // Add enumerable prototype properties
                    for (const auto &pair : ptr->prototype)
                    {
                        bool is_enumerable = false;
                        if (std::holds_alternative<DataDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<DataDescriptor>(pair.second).enumerable;
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<AccessorDescriptor>(pair.second).enumerable;
                        }
                        // If it's enumerable and not already seen (shadowed by own property)
                        if (is_enumerable && seen_keys.find(pair.first) == seen_keys.end())
                        {
                            keys.push_back(pair.first);
                            seen_keys.insert(pair.first);
                        }
                    }
                }
            }
            return keys;
        }

        inline JsValue get_property(const JsValue &obj, const JsValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");

                const auto key_str = Convert::to_string(key);
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    return it->second;
                }

                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    // Handle prototype property
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
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

                std::string key_str = Convert::to_string(key);
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos != key_str.length())
                    {
                        throw std::invalid_argument("not a valid integer index");
                    }

                    if (index < ptr->items.size())
                    {
                        return ptr->items[index];
                    }
                    return undefined;
                }
                catch (...)
                {
                    // Not a numeric index, check prototype
                    const auto proto_it = ptr->prototype.find(key_str);
                    if (proto_it != ptr->prototype.end())
                    {
                        const auto &prop = proto_it->second;
                        if (std::holds_alternative<DataDescriptor>(prop))
                        {
                            return std::get<DataDescriptor>(prop).value;
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(prop))
                        {
                            const auto &accessor = std::get<AccessorDescriptor>(prop);
                            if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                            {
                                return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                            }
                        }
                    }
                    return undefined;
                }
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                // Check prototype for properties like 'length'
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                        }
                    }
                }
                // Also handle numeric indexing for characters
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos == key_str.length() && index < ptr->value.length())
                    {
                        return jspp::Object::make_string(std::string(1, ptr->value[index]));
                    }
                }
                catch (...)
                { /* Not a numeric index, ignore */
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
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
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
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
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
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            throw Exception::make_error_with_name("Cannot read properties of non-object type", "TypeError");
        }

        inline JsValue set_property(const JsValue &obj, const JsValue &key, const JsValue &val)
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
                    // Handle prototype property
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, JsValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set))
                        {
                            std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set)({val});
                        }
                    }
                }
                else
                {
                    ptr->properties[key_str] = val;
                }

                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");

                std::string key_str = Convert::to_string(key);
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos != key_str.length())
                    {
                        throw std::invalid_argument("not a valid integer index");
                    }

                    if (index >= ptr->items.size())
                    {
                        ptr->items.resize(index + 1, undefined);
                    }
                    ptr->items[index] = val;
                    return val;
                }
                catch (...)
                {
                    // Not a numeric index, but a string
                    // handle string properties on arrays
                    const auto proto_it = ptr->prototype.find(key_str);

                    if (proto_it != ptr->prototype.end())
                    {
                        const auto &prop = proto_it->second;
                        if (std::holds_alternative<DataDescriptor>(prop))
                        {
                            auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, JsValue> &>(prop));
                            if (data_desc.writable)
                            {
                                data_desc.value = val;
                            }
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(prop))
                        {
                            const auto &accessor = std::get<AccessorDescriptor>(prop);
                            if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set))
                            {
                                std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set)({val});
                            }
                        }
                    }
                    else
                    {
                        ptr->prototype[key_str] = val;
                    }
                    return val;
                }
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                ptr->prototype[key_str] = val;
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                ptr->prototype[key_str] = val;
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                ptr->prototype[key_str] = val;
                return val;
            }
            throw jspp::Exception::make_error_with_name("Cannot set properties of non-object type", "TypeError");
        }

    }
}
