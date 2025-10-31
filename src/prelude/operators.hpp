#pragma once

#include "types.hpp"
#include "access.hpp"
#include "convert.hpp"
#include "object.hpp"

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
    if (lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || rhs.type() == typeid(std::shared_ptr<jspp::JsString>))
    {
        return jspp::Object::make_string(jspp::Convert::to_string(lhs) + jspp::Convert::to_string(rhs));
    }
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

inline jspp::JsValue operator/(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) / std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) / std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) / std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) / std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator%(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) % std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator^(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) ^ std::any_cast<int>(rhs);
    return undefined;
}

inline bool operator>=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return (lhs > rhs) || jspp::Access::equals(lhs, rhs);
}

inline bool operator!=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return !jspp::Access::equals(lhs, rhs);
}

inline jspp::JsValue operator&(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) & std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator|(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) | std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator~(const jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        return ~std::any_cast<int>(val);
    return undefined;
}

inline jspp::JsValue operator<<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) << std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator>>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) >> std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<int>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<int>(rhs));
    return undefined;
}

inline jspp::JsValue &operator++(jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        val = std::any_cast<int>(val) + 1;
    else if (val.type() == typeid(double))
        val = std::any_cast<double>(val) + 1;
    return val;
}

inline jspp::JsValue operator++(jspp::JsValue &val, int)
{
    jspp::JsValue old = val;
    ++val;
    return old;
}

inline jspp::JsValue &operator--(jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        val = std::any_cast<int>(val) - 1;
    else if (val.type() == typeid(double))
        val = std::any_cast<double>(val) - 1;
    return val;
}

inline jspp::JsValue operator--(jspp::JsValue &val, int)
{
    jspp::JsValue old = val;
    --val;
    return old;
}

inline jspp::JsValue &operator+=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline jspp::JsValue &operator-=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline jspp::JsValue &operator*=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

inline jspp::JsValue &operator/=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

inline jspp::JsValue &operator%=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs % rhs;
    return lhs;
}

// Define operators for osstream
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline jspp::JsValue jspp::pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<int>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<int>(rhs));
    return undefined;
}


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
    os << jspp::Convert::to_string(v);
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
