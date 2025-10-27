#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>
#include <sstream>

struct Undefined
{
};
inline Undefined undefined;

struct Null
{
};
inline Null null;

using JsVariant = std::any;

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline std::ostream &operator<<(std::ostream &os, const JsVariant &v)
{
    if (!v.has_value())
    {
        os << "undefined";
        return os;
    }
    if (v.type() == typeid(Undefined))
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
        ((std::cout << JsVariant(args) << " "), ...);
        std::cout << std::endl;
        return undefined;
    }

    template <typename... Args>
    Undefined warn(Args... args)
    {
        std::cerr << "\033[33m"; // Yellow
        ((std::cerr << JsVariant(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }

    template <typename... Args>
    Undefined error(Args... args)
    {
        std::cerr << "\033[31m"; // Red
        ((std::cerr << JsVariant(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }
};

inline Console console;

inline JsVariant operator+(const JsVariant &lhs, const JsVariant &rhs)
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
