#pragma once

#include "types.hpp"
#include "exception.hpp"
#include "convert.hpp"
#include "object.hpp"

namespace jspp {
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

        inline bool is_truthy(const JsValue &val)
        {
            if (!val.has_value())
                return false; // undefined
            if (val.type() == typeid(Undefined))
                return false;
            if (val.type() == typeid(Null))
                return false;
            if (val.type() == typeid(bool))
                return std::any_cast<bool>(val);
            if (val.type() == typeid(int))
                return std::any_cast<int>(val) != 0;
            if (val.type() == typeid(double))
                return std::any_cast<double>(val) != 0.0;
            if (val.type() == typeid(std::string))
                return !std::any_cast<std::string>(val).empty();
            if (val.type() == typeid(const char *))
                return std::any_cast<const char *>(val)[0] != '\0';
            if (val.type() == typeid(Uninitialized))
                return false;
            if (val.type() == typeid(std::shared_ptr<JsObject>))
            {
                return std::any_cast<std::shared_ptr<JsObject>>(val) != nullptr;
            }
            if (val.type() == typeid(std::shared_ptr<JsArray>))
            {
                return std::any_cast<std::shared_ptr<JsArray>>(val) != nullptr;
            }
            if (val.type() == typeid(std::shared_ptr<JsString>))
            {
                auto s = std::any_cast<std::shared_ptr<JsString>>(val);
                return s && !s->value.empty();
            }
            if (val.type() == typeid(std::shared_ptr<JsFunction>))
            {
                return std::any_cast<std::shared_ptr<JsFunction>>(val) != nullptr;
            }
            if (val.type() == typeid(std::function<JsValue(const std::vector<JsValue> &)>))
            {
                return true;
            }
            return true;
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

                    if (index < ptr->properties.size())
                    {
                        return ptr->properties[index];
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

                    if (index >= ptr->properties.size())
                    {
                        ptr->properties.resize(index + 1, undefined);
                    }
                    ptr->properties[index] = val;
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
            throw jspp::Exception::make_error_with_name("Cannot set properties of non-object type", "TypeError");
        }

        inline bool strict_equals(const JsValue &lhs, const JsValue &rhs)
        {
            // Strict equality (===)
            if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
                return std::any_cast<int>(lhs) == std::any_cast<double>(rhs);
            if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
                return std::any_cast<double>(lhs) == std::any_cast<int>(rhs);

            if (lhs.type() != rhs.type())
                return false;

            if (!lhs.has_value() || lhs.type() == typeid(Undefined))
                return true; // both undefined
            if (lhs.type() == typeid(Null))
                return true; // both null

            if (lhs.type() == typeid(int))
                return std::any_cast<int>(lhs) == std::any_cast<int>(rhs);
            if (lhs.type() == typeid(double))
                return std::any_cast<double>(lhs) == std::any_cast<double>(rhs);
            if (lhs.type() == typeid(bool))
                return std::any_cast<bool>(lhs) == std::any_cast<bool>(rhs);
            if (lhs.type() == typeid(std::string))
                return std::any_cast<std::string>(lhs) == std::any_cast<std::string>(rhs);
            if (lhs.type() == typeid(const char *))
                return strcmp(std::any_cast<const char *>(lhs), std::any_cast<const char *>(rhs)) == 0;

            if (lhs.type() == typeid(std::shared_ptr<jspp::JsObject>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsObject>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsObject>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsArray>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsArray>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsArray>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsString>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsString>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsString>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsFunction>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsFunction>>(rhs);
            }

            // Cannot compare functions for equality in C++.
            return false;
        }

        inline bool equals(const JsValue &lhs, const JsValue &rhs)
        {
            // Abstract/Loose Equality (==)
            if (lhs.type() == rhs.type())
            {
                return strict_equals(lhs, rhs); // Use strict equality if types are same
            }

            // null == undefined
            if ((lhs.type() == typeid(Null) && (!rhs.has_value() || rhs.type() == typeid(Undefined))) ||
                ((!lhs.has_value() || lhs.type() == typeid(Undefined)) && rhs.type() == typeid(Null)))
            {
                return true;
            }

            // number == string
            if ((lhs.type() == typeid(int) || lhs.type() == typeid(double)) && (rhs.type() == typeid(std::shared_ptr<JsString>) || rhs.type() == typeid(std::string) || rhs.type() == typeid(const char *)))
            {
                double l = lhs.type() == typeid(int) ? std::any_cast<int>(lhs) : std::any_cast<double>(lhs);
                std::string s_rhs = Convert::to_string(rhs);

                s_rhs.erase(s_rhs.begin(), std::find_if(s_rhs.begin(), s_rhs.end(), [](int ch)
                                                        { return !std::isspace(ch); }));
                s_rhs.erase(std::find_if(s_rhs.rbegin(), s_rhs.rend(), [](int ch)
                                         { return !std::isspace(ch); })
                                .base(),
                            s_rhs.end());

                if (s_rhs.empty())
                {
                    return l == 0;
                }

                try
                {
                    double r = std::stod(s_rhs);
                    return l == r;
                }
                catch (...)
                {
                    return false;
                }
            }
            if ((rhs.type() == typeid(int) || rhs.type() == typeid(double)) && (lhs.type() == typeid(std::string) || lhs.type() == typeid(const char *)))
            {
                return equals(rhs, lhs); // reuse logic
            }

            // boolean == any
            if (lhs.type() == typeid(bool))
            {
                return equals(jspp::JsValue(std::any_cast<bool>(lhs) ? 1 : 0), rhs);
            }
            if (rhs.type() == typeid(bool))
            {
                return equals(lhs, jspp::JsValue(std::any_cast<bool>(rhs) ? 1 : 0));
            }

            // object == primitive
            if ((lhs.type() == typeid(std::shared_ptr<JsObject>) || lhs.type() == typeid(std::shared_ptr<JsArray>) || lhs.type() == typeid(std::shared_ptr<JsString>) || lhs.type() == typeid(std::shared_ptr<JsFunction>)) &&
                (rhs.type() != typeid(std::shared_ptr<JsObject>) && rhs.type() != typeid(std::shared_ptr<JsArray>) && rhs.type() != typeid(std::shared_ptr<JsString>) && rhs.type() != typeid(std::shared_ptr<JsFunction>)))
            {
                return equals(jspp::JsValue(Convert::to_string(lhs)), rhs);
            }
            if ((rhs.type() == typeid(std::shared_ptr<JsObject>) || rhs.type() == typeid(std::shared_ptr<JsArray>) || rhs.type() == typeid(std::shared_ptr<JsString>) || rhs.type() == typeid(std::shared_ptr<JsFunction>)) &&
                (lhs.type() != typeid(std::shared_ptr<JsObject>) && lhs.type() != typeid(std::shared_ptr<JsArray>) && lhs.type() != typeid(std::shared_ptr<JsString>) && lhs.type() != typeid(std::shared_ptr<JsFunction>)))
            {
                return equals(lhs, jspp::JsValue(Convert::to_string(rhs)));
            }

            return false;
        }
    }
}
