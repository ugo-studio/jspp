#pragma once

#include "types.hpp"
#include "exception.hpp"
#include "convert.hpp"
#include "object.hpp"

namespace jspp
{
    // Forward declaration of Prototype namespace and (setter_exists, set_prototype) function
    namespace Prototype
    {
        bool setter_exists(const AnyValue &obj, const AnyValue &key);
        AnyValue set_prototype(const AnyValue &obj, const AnyValue &key, const AnyValue &val);
    }

    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline AnyValue deref(const std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if (!var)
                // This case should ideally not be hit in normal operation
                throw Exception::make_error_with_name("null variable pointer for " + name, "Internal compiler error");
            if ((*var).type() == typeid(Uninitialized))
                throw Exception::make_error_with_name("Cannot access '" + name + "' before initialization", "ReferenceError");
            return *var;
        }

        // Helper function to call JsFunction
        inline AnyValue call_function(const AnyValue &var, const std::vector<AnyValue> &args, const std::string &name)
        {
            if (var.type() == typeid(std::shared_ptr<JsFunction>))
            {
                return std::any_cast<const std::shared_ptr<JsFunction> &>(var)->call(args);
            }
            throw Exception::make_error_with_name(name + " is not a function", "TypeError");
        }

        inline std::vector<std::string> get_object_keys(const AnyValue &obj)
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
                        bool is_enumerable = false;
                        if (std::holds_alternative<DataDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<DataDescriptor>(pair.second).enumerable;
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<AccessorDescriptor>(pair.second).enumerable;
                        }
                        else if (std::holds_alternative<AnyValue>(pair.second))
                        {
                            is_enumerable = true;
                        }
                        // If it's enumerable and not already seen (shadowed by own property)
                        if (is_enumerable && seen_keys.find(pair.first) == seen_keys.end())
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
                        else if (std::holds_alternative<AnyValue>(pair.second))
                        {
                            is_enumerable = true;
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

        inline AnyValue get_property(const AnyValue &obj, const AnyValue &key)
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
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }

                // Handle prototype property
                return Prototype::get_prototype(obj, key);
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
                }
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }
                // Handle prototype property
                return Prototype::get_prototype(obj, key);
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);

                // handle numeric indexing for characters
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos == key_str.length() && index < ptr->value.length())
                    {
                        return Object::make_string(std::string(1, ptr->value[index]));
                    }
                }
                catch (...)
                { /* Not a numeric index, ignore */
                }

                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }

                // Handle prototype property
                return Prototype::get_prototype(obj, key);
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }
                // Handle prototype property
                return Prototype::get_prototype(obj, key);
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }
                // Handle prototype property
                return Prototype::get_prototype(obj, key);
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    if (std::holds_alternative<DataDescriptor>(it->second))
                    {
                        return std::get<DataDescriptor>(it->second).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(it->second))
                    {
                        auto desc = std::get<AccessorDescriptor>(it->second);
                        if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get))
                        {
                            return std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(desc.get)({});
                        }
                        else if (std::holds_alternative<Undefined>(desc.get))
                        {
                            return undefined;
                        }
                    }
                    else if (std::holds_alternative<AnyValue>(it->second))
                    {
                        return std::get<AnyValue>(it->second);
                    }
                }
                // Handle prototype property
                return Prototype::get_prototype(obj, key);
            }
            throw Exception::make_error_with_name("Cannot read properties of non-object type", "TypeError");
        }

        inline AnyValue set_property(const AnyValue &obj, const AnyValue &key, const AnyValue &val)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
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
                        throw std::invalid_argument("not a valid integer index"); // will be catched
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
                    std::cout << "failed" << key_str << std::endl;
                }

                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
            }
            if (obj.type() == typeid(std::shared_ptr<JsNumber>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
            }
            if (obj.type() == typeid(std::shared_ptr<JsBoolean>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                if (ptr->properties.find(key_str) != ptr->properties.end() || !Prototype::setter_exists(obj, key))
                {
                    ptr->properties[key_str] = val;
                }
                return Prototype::set_prototype(obj, key, val);
            }
            throw Exception::make_error_with_name("Cannot set properties of non-object type", "TypeError");
        }

    }
}
