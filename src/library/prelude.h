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
                throw std::runtime_error("Internal compiler error: null variable pointer for " + name);
            if ((*var).type() == typeid(Uninitialized))
                throw std::runtime_error("ReferenceError: Cannot access '" + name + "' before initialization");
            return *var;
        }
    };

    struct JsObject
    {
        std::map<std::string, JsValue> properties;
    };

    struct JsArray
    {
        std::vector<JsValue> elements;
    };

    namespace Access
    {
        inline std::string js_value_to_string(const JsValue &val)
        {
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
            return "undefined";
        }

        inline JsValue getProperty(const JsValue &obj, const JsValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw std::runtime_error("TypeError: Cannot read properties of null");
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
                    throw std::runtime_error("TypeError: Cannot read properties of null");

                std::string key_str = js_value_to_string(key);
                if (key_str == "length")
                {
                    return (int)ptr->elements.size();
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
            throw std::runtime_error("TypeError: Cannot read properties of non-object type");
        }

        inline JsValue setProperty(const JsValue &obj, const JsValue &key, const JsValue &val)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw std::runtime_error("TypeError: Cannot set properties of null");
                ptr->properties[js_value_to_string(key)] = val;
                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw std::runtime_error("TypeError: Cannot set properties of null");

                if (js_value_to_string(key) == "length")
                {
                    size_t new_length = 0;
                    if (val.type() == typeid(int))
                    {
                        int v = std::any_cast<int>(val);
                        if (v < 0)
                            throw std::runtime_error("RangeError: Invalid array length");
                        new_length = static_cast<size_t>(v);
                    }
                    else if (val.type() == typeid(double))
                    {
                        double v = std::any_cast<double>(val);
                        if (v < 0 || v != static_cast<uint32_t>(v))
                            throw std::runtime_error("RangeError: Invalid array length");
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
                        // Not a numeric index, fall through to object-like property setting
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
                // TODO: handle non-numeric properties on arrays if needed
                return val;
            }
            throw std::runtime_error("TypeError: Cannot set properties of non-object type");
        }
    }

    struct JsError
    {
        static JsValue unresolved_reference(const std::string &varName)
        {
            throw std::runtime_error("ReferenceError: " + varName + " is not defined");
        }
        static JsValue immutable_assignment()
        {
            throw std::runtime_error("TypeError: Assignment to constant variable.");
        }
        static JsValue invalid_return_statement()
        {
            throw std::runtime_error("SyntaxError: Return statements are only valid inside functions.");
        }
    };
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

    if (!v.has_value())
    {
        os << "undefined";
        return os;
    }
    if (v.type() == typeid(jspp::Tdz::Uninitialized))
        // This should ideally not be printed if the TDZ logic is correct
        os << "<uninitialized>";
    else if (v.type() == typeid(Undefined))
        os << "undefined";
    else if (v.type() == typeid(Null))
        os << "null";
    else if (v.type() == typeid(bool))
        os << std::boolalpha << std::any_cast<bool>(v);
    else if (v.type() == typeid(int))
        os << std::any_cast<int>(v);
    else if (v.type() == typeid(double))
        os << std::any_cast<double>(v);
    else if (v.type() == typeid(const char *))
        os << std::any_cast<const char *>(v);
    else if (v.type() == typeid(std::string))
        os << std::any_cast<std::string>(v);
    else if (v.type() == typeid(std::shared_ptr<jspp::JsObject>))
        os << "[object Object]";
    else if (v.type() == typeid(std::shared_ptr<jspp::JsArray>))
    {
        os << "[";
        auto &ptr = std::any_cast<const std::shared_ptr<jspp::JsArray> &>(v);
        for (size_t i = 0; i < ptr->elements.size(); ++i)
        {
            os << ptr->elements[i];
            if (i < ptr->elements.size() - 1)
            {
                os << ", ";
            }
        }
        os << "]";
    }
    else
        os << "function(){}";
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
