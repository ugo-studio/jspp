#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>
#include <sstream>
#include <memory>

struct Undefined
{
};
inline Undefined undefined;

struct Null
{
};
inline Null null;

struct TdzUninitialized {};
inline TdzUninitialized tdz_uninitialized;

using JsValue = std::any;

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline std::ostream &operator<<(std::ostream &os, const JsValue &v)
{
    if (v.type() == typeid(std::shared_ptr<JsValue>))
    {
        const auto &ptr = std::any_cast<std::shared_ptr<JsValue>>(v);
        if (ptr)
        {
            return os << *ptr;
        }
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
    if (v.type() == typeid(TdzUninitialized))
    {
        // This should ideally not be printed if the TDZ logic is correct
        os << "<uninitialized>";
    }
    else if (v.type() == typeid(Undefined))
    {
        os << "undefined";
    }
    else if (v.type() == typeid(Null))
    {
        os << "null";
    }
    else if (v.type() == typeid(bool))
    {
        os << std::boolalpha << std::any_cast<bool>(v);
    }
    else if (v.type() == typeid(int))
    {
        os << std::any_cast<int>(v);
    }
    else if (v.type() == typeid(double))
    {
        os << std::any_cast<double>(v);
    }
    else if (v.type() == typeid(const char *))
    {
        os << std::any_cast<const char *>(v);
    }
    else if (v.type() == typeid(std::string))
    {
        os << std::any_cast<std::string>(v);
    }
    else
    {
        os << "function(){}";
    }
    return os;
}

struct Console
{
    template <typename... Args>
    Undefined log(Args... args)
    {
        ((std::cout << JsValue(args) << " "), ...);
        std::cout << std::endl;
        return undefined;
    }

    template <typename... Args>
    Undefined warn(Args... args)
    {
        std::cerr << "\033[33m"; // Yellow
        ((std::cerr << JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }

    template <typename... Args>
    Undefined error(Args... args)
    {
        std::cerr << "\033[31m"; // Red
        ((std::cerr << JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }
};

inline Console console;

inline JsValue operator+(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) + std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) + std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) + std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) + std::any_cast<int>(rhs);
    }
    return undefined;
}

inline JsValue operator*(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) * std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) * std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) * std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) * std::any_cast<int>(rhs);
    }
    return undefined;
}

inline JsValue operator-(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) - std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) - std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) - std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) - std::any_cast<int>(rhs);
    }
    return undefined;
}

inline bool operator<=(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) <= std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) <= std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) <= std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) <= std::any_cast<int>(rhs);
    }
    return false;
}

inline bool operator>(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) > std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) > std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) > std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) > std::any_cast<int>(rhs);
    }
    return false;
}

inline bool operator<(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) < std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) < std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) < std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) < std::any_cast<int>(rhs);
    }
    return false;
}

inline bool operator==(const JsValue &lhs, const JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
    {
        return std::any_cast<int>(lhs) == std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
    {
        return std::any_cast<double>(lhs) == std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
    {
        return std::any_cast<int>(lhs) == std::any_cast<double>(rhs);
    }
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
    {
        return std::any_cast<double>(lhs) == std::any_cast<int>(rhs);
    }
    if (lhs.type() == typeid(bool) && rhs.type() == typeid(bool))
    {
        return std::any_cast<bool>(lhs) == std::any_cast<bool>(rhs);
    }
    return false;
}

inline JsValue checkAndDeref(const std::shared_ptr<JsValue>& var, const std::string& varName) {
    if (!var) {
        // This case should ideally not be hit in normal operation
        throw std::runtime_error("Internal compiler error: null variable pointer for " + varName);
    }
    if ((*var).type() == typeid(TdzUninitialized)) {
        throw std::runtime_error("ReferenceError: Cannot access '" + varName + "' before initialization");
    }
    return *var;
}
