#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>
#include <sstream>
#include <memory>
#include <map>

struct Undefined
{
};
inline Undefined undefined;
struct Null
{
};
inline Null null;

// JSPP standard library
namespace jspp
{
    // Dynamic JsValue
    using JsValue = std::any;

    // Objects
    struct JsObject
    {
        std::map<std::string, JsValue> properties;
    };

    // Arrays
    struct JsArray
    {
        std::vector<JsValue> elements;
        std::map<std::string, JsValue> properties;
    };

    // Errors
    struct JsError
    {
        static JsValue make_error(const JsValue &message, const JsValue &name)
        {
            auto error = std::make_shared<JsObject>(JsObject{{{"message", message}, {"name", name}}});
            error->properties["toString"] = std::function<JsValue()>([=]() mutable -> JsValue
                                                                     {
                        std::string name_str = "Error";
                        if (error->properties.count("name") > 0) {
                            if (error->properties["name"].type() == typeid(std::string)) {
                                name_str = std::any_cast<std::string>(error->properties["name"]);
                            } else if (error->properties["name"].type() == typeid(const char *)) {
                                name_str = std::any_cast<const char *>(error->properties["name"]);
                            }
                        }
                        std::string message_str = "";
                        if (error->properties.count("message") > 0) {
                            if (error->properties["message"].type() == typeid(std::string)) {
                                message_str = std::any_cast<std::string>(error->properties["message"]);
                            } else if (error->properties["message"].type() == typeid(const char *)) {
                                message_str = std::any_cast<const char *>(error->properties["message"]);
                            }
                        }
                        std::string stack_str = ""; // Default to an empty string if not present
                        if (error->properties.count("stack") > 0) {
                             if (error->properties["stack"].type() == typeid(std::string)) {
                                stack_str = std::any_cast<std::string>(error->properties["stack"]);
                            } else if (error->properties["stack"].type() == typeid(const char *)) {
                                stack_str = std::any_cast<const char *>(error->properties["stack"]);
                            }
                        }
                        return name_str + ": " + message_str + "\n    at " + stack_str; });
            return error;
        }
        static JsValue make_default_error(const JsValue &message)
        {
            return JsError::make_error(message, "Error");
        }
        static JsValue parse_error_from_value(const JsValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<std::exception>))
            {
                return JsError::make_error(std::string(std::any_cast<std::exception>(val).what()), "Error");
            }
            return val;
        }
        static JsValue throw_unresolved_reference(const std::string &varName)
        {
            throw JsError::make_error(varName + " is not defined", "ReferenceError");
        }
        static JsValue throw_immutable_assignment()
        {
            throw JsError::make_error("Assignment to constant variable.", "TypeError");
        }
        static JsValue throw_invalid_return_statement()
        {
            throw JsError::make_error("Return statements are only valid inside functions.", "SyntaxError");
        }
    };

    // Define state for TDZ(Temporal Dead Zone)
    namespace Tdz
    {
        struct Uninitialized
        {
        };
        inline constexpr Uninitialized uninitialized;
        // Helper function to check for TDZ and deref variables
        inline JsValue deref(const std::shared_ptr<JsValue> &var, const std::string &name)
        {
            if (!var)
                // This case should ideally not be hit in normal operation
                throw JsError::make_error("null variable pointer for " + name, "Internal compiler error");
            if ((*var).type() == typeid(Uninitialized))
                throw JsError::make_error("Cannot access '" + name + "' before initialization", "ReferenceError");
            return *var;
        }
    };

    namespace Access
    {

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
            if (val.type() == typeid(Tdz::Uninitialized))
                return false;
            if (val.type() == typeid(std::shared_ptr<JsObject>))
            {
                return std::any_cast<std::shared_ptr<JsObject>>(val) != nullptr;
            }
            if (val.type() == typeid(std::shared_ptr<JsArray>))
            {
                return std::any_cast<std::shared_ptr<JsArray>>(val) != nullptr;
            }
            if (val.type() == typeid(std::function<JsValue(const std::vector<JsValue> &)>))
            {
                return true;
            }
            return true;
        }

        inline std::string js_value_to_string(const JsValue &val)
        {
            if (!val.has_value())
                return "undefined";
            if (val.type() == typeid(Tdz::Uninitialized))
                return "<uninitialized>"; // This should ideally not be returned if the TDZ logic is correct
            if (val.type() == typeid(std::string))
                return std::any_cast<std::string>(val);
            if (val.type() == typeid(const char *))
                return std::any_cast<const char *>(val);
            if (val.type() == typeid(int))
                return std::to_string(std::any_cast<int>(val));
            if (val.type() == typeid(double))
                return std::to_string(std::any_cast<double>(val));
            if (val.type() == typeid(bool))
                return std::any_cast<bool>(val) ? "true" : "false";
            if (val.type() == typeid(Null))
                return "null";
            if (val.type() == typeid(Undefined))
                return "undefined";
            if (val.type() == typeid(std::shared_ptr<jspp::JsObject>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsObject>>(val);
                if (ptr->properties.count("toString") > 0 && ptr->properties["toString"].type() == typeid(std::function<JsValue()>))
                {
                    auto val_str = std::any_cast<std::function<jspp::JsValue()>>(ptr->properties["toString"])();
                    if (val_str.type() == typeid(std::string))
                    {
                        return std::any_cast<std::string>(val_str);
                    }
                }
                return "[Object Object]";
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsArray>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsArray>>(val);
                if (ptr->properties.count("toString") > 0 && ptr->properties["toString"].type() == typeid(std::function<JsValue()>))
                {
                    auto val_str = std::any_cast<std::function<jspp::JsValue()>>(ptr->properties["toString"])();
                    if (val_str.type() == typeid(std::string))
                    {
                        return std::any_cast<std::string>(val_str);
                    }
                }
                return "[Array Array]";
            }
            return "function(){}";
        }

        inline std::shared_ptr<jspp::JsObject> make_object(const std::map<std::string, JsValue> &properties)
        {
            auto object = std::make_shared<jspp::JsObject>(jspp::JsObject{properties});
            if (object->properties.count("toString") == 0)
            {
                object->properties["toString"] = std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                                                                { return "[object Object]"; });
            }
            return object;
        }

        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<JsValue> &elements)
        {
            auto array = std::make_shared<jspp::JsArray>(jspp::JsArray{elements, {}});
            array->properties["toString"] = std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                                                           {
                std::string str = "[";
                for (size_t i = 0; i < array->elements.size(); ++i)
                {
                    str += js_value_to_string(array->elements[i]);
                    if (i < array->elements.size() - 1)
                        str += ", ";
                }
                str += "]";
                return str; });
            return array;
        }

        inline JsValue get_property(const JsValue &obj, const JsValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw JsError::make_error("Cannot read properties of null", "TypeError");
                const auto it = ptr->properties.find(js_value_to_string(key));
                if (it != ptr->properties.end())
                {
                    return it->second;
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw JsError::make_error("Cannot read properties of null", "TypeError");

                std::string key_str = js_value_to_string(key);
                if (key_str == "length")
                {
                    return (int)ptr->elements.size();
                }

                if (ptr->properties.count(key_str) > 0)
                {
                    return ptr->properties[key_str];
                }

                size_t index = -1;
                try
                {
                    index = std::stoul(key_str);
                }
                catch (...)
                {
                    // Not a numeric index, fall through
                }

                if (index != -1 && index < ptr->elements.size())
                {
                    return ptr->elements[index];
                }
                return undefined;
            }
            throw JsError::make_error("Cannot read properties of non-object type", "TypeError");
        }

        inline JsValue set_property(const JsValue &obj, const JsValue &key, const JsValue &val)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw JsError::make_error("Cannot set properties of null", "TypeError");
                ptr->properties[js_value_to_string(key)] = val;
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw JsError::make_error("Cannot set properties of null", "TypeError");

                if (js_value_to_string(key) == "length")
                {
                    size_t new_length = 0;
                    if (val.type() == typeid(int))
                    {
                        int v = std::any_cast<int>(val);
                        if (v < 0)
                        {
                            throw JsError::make_error("Invalid array length", "RangeError");
                        }
                        new_length = static_cast<size_t>(v);
                    }
                    else if (val.type() == typeid(double))
                    {
                        double v = std::any_cast<double>(val);
                        if (v < 0 || v != static_cast<uint32_t>(v))
                        {
                            throw JsError::make_error("Invalid array length", "RangeError");
                        }
                        new_length = static_cast<size_t>(v);
                    }
                    else
                    {
                        // Other types could be converted to number in a more complete implementation
                        return val;
                    }
                    ptr->elements.resize(new_length, undefined);
                    return val;
                }

                size_t index = -1;
                if (key.type() == typeid(int))
                {
                    index = std::any_cast<int>(key);
                }
                else if (key.type() == typeid(std::string))
                {
                    try
                    {
                        index = std::stoul(std::any_cast<std::string>(key));
                    }
                    catch (...)
                    {
                        // Not a numeric index, but a string
                        // handle string properties on arrays
                        ptr->properties[std::any_cast<std::string>(key)] = val;
                        return val;
                    }
                }

                if (index != -1)
                {
                    if (index >= ptr->elements.size())
                    {
                        ptr->elements.resize(index + 1, undefined);
                    }
                    ptr->elements[index] = val;
                    return val;
                }

                // TODO: handle non-alpha-numeric index
                return val;
            }
            throw jspp::JsError::make_error("Cannot set properties of non-object type", "TypeError");
        }
    }

}

// Define operators for JsValue
inline jspp::JsValue operator+(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) + std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) + std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) + std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) + std::any_cast<int>(rhs);
    return undefined;
}
inline jspp::JsValue operator*(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) * std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) * std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) * std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) * std::any_cast<int>(rhs);
    return undefined;
}
inline jspp::JsValue operator-(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) - std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) - std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) - std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) - std::any_cast<int>(rhs);
    return undefined;
}
inline bool operator<=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) <= std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) <= std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) <= std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) <= std::any_cast<int>(rhs);
    return false;
}
inline bool operator>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) > std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) > std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) > std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) > std::any_cast<int>(rhs);
    return false;
}
inline bool operator<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) < std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) < std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) < std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) < std::any_cast<int>(rhs);
    return false;
}
inline bool operator==(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) == std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) == std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) == std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) == std::any_cast<int>(rhs);
    if (lhs.type() == typeid(bool) && rhs.type() == typeid(bool))
        return std::any_cast<bool>(lhs) == std::any_cast<bool>(rhs);
    return false;
}

// Define operators for osstream
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline std::ostream &operator<<(std::ostream &os, const jspp::JsValue &v)
{
    if (v.type() == typeid(std::shared_ptr<jspp::JsValue>))
    {
        const auto &ptr = std::any_cast<std::shared_ptr<jspp::JsValue>>(v);
        if (ptr)
            return os << *ptr;
        else
        {
            os << "undefined";
            return os;
        }
    }
    os << jspp::Access::js_value_to_string(v);
    return os;
}

// Basic console API
struct Console
{
    template <typename... Args>
    Undefined log(Args... args)
    {
        ((std::cout << jspp::JsValue(args) << " "), ...);
        std::cout << std::endl;
        return undefined;
    }

    template <typename... Args>
    Undefined warn(Args... args)
    {
        std::cerr << "\033[33m"; // Yellow
        ((std::cerr << jspp::JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }

    template <typename... Args>
    Undefined error(Args... args)
    {
        std::cerr << "\033[31m"; // Red
        ((std::cerr << jspp::JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }
};
inline Console console;
