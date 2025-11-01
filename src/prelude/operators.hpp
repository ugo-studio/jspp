#pragma once

#include "types.hpp"
#include "access.hpp"
#include "convert.hpp"
#include "object.hpp"
#include <cmath> // Required for std::pow

jspp::JsValue parse_number_value_if_number(const jspp::JsValue &val)
{
    if (val.type() == typeid(std::shared_ptr<jspp::JsNumber>))
    {
        auto ptr = std::any_cast<std::shared_ptr<jspp::JsNumber>>(val);
        if (std::holds_alternative<int>(ptr->value))
        {
            return std::get<int>(ptr->value);
        }
        else if (std::holds_alternative<double>(ptr->value))
        {
            return std::get<double>(ptr->value);
        }
    }
    return val;
}

// Define operators for JsValue
inline jspp::JsValue operator+(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) + std::any_cast<int>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) + std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) + std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) + std::any_cast<int>(actual_rhs));
    if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>))
    {
        return jspp::Object::make_string(jspp::Convert::to_string(actual_lhs) + jspp::Convert::to_string(actual_rhs));
    }
    return undefined;
}
inline jspp::JsValue operator*(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) * std::any_cast<int>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) * std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) * std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) * std::any_cast<int>(actual_rhs));
    return undefined;
}
inline jspp::JsValue operator-(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) - std::any_cast<int>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) - std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) - std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) - std::any_cast<int>(actual_rhs));
    return undefined;
}
inline bool operator<=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return std::any_cast<int>(actual_lhs) <= std::any_cast<int>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return std::any_cast<double>(actual_lhs) <= std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return std::any_cast<int>(actual_lhs) <= std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return std::any_cast<double>(actual_lhs) <= std::any_cast<int>(actual_rhs);
    return false;
}
inline bool operator>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return std::any_cast<int>(actual_lhs) > std::any_cast<int>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return std::any_cast<double>(actual_lhs) > std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return std::any_cast<int>(actual_lhs) > std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return std::any_cast<double>(actual_lhs) > std::any_cast<int>(actual_rhs);
    return false;
}
inline bool operator<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return std::any_cast<int>(actual_lhs) < std::any_cast<int>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return std::any_cast<double>(actual_lhs) < std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return std::any_cast<int>(actual_lhs) < std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return std::any_cast<double>(actual_lhs) < std::any_cast<int>(actual_rhs);
    return false;
}

inline jspp::JsValue operator/(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) / std::any_cast<int>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) / std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) / std::any_cast<double>(actual_rhs));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<double>(actual_lhs) / std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator%(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) % std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator^(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) ^ std::any_cast<int>(actual_rhs));
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
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) & std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator|(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) | std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator~(const jspp::JsValue &val)
{
    auto actual_val = parse_number_value_if_number(val);
    if (actual_val.type() == typeid(int))
        return jspp::Object::make_number(~std::any_cast<int>(actual_val));
    return undefined;
}

inline jspp::JsValue operator<<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) << std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator>>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) >> std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue jspp::pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = parse_number_value_if_number(lhs);
    auto actual_rhs = parse_number_value_if_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::pow(std::any_cast<int>(actual_lhs), std::any_cast<int>(actual_rhs)));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::pow(std::any_cast<double>(actual_lhs), std::any_cast<double>(actual_rhs)));
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return jspp::Object::make_number(std::pow(std::any_cast<int>(actual_lhs), std::any_cast<double>(actual_rhs)));
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::pow(std::any_cast<double>(actual_lhs), std::any_cast<int>(actual_rhs)));
    return undefined;
}

inline jspp::JsValue &operator++(jspp::JsValue &val)
{
    // For in-place operators, it's more complex. We'll modify the original value
    // if it's a number, but this implementation assumes it's not a JsNumber object.
    // A more robust solution might require changing the JsNumber's internal value.
    auto actual_val = parse_number_value_if_number(val);
    if (actual_val.type() == typeid(int))
        val = jspp::Object::make_number(std::any_cast<int>(actual_val) + 1);
    else if (actual_val.type() == typeid(double))
        val = jspp::Object::make_number(std::any_cast<double>(actual_val) + 1);
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
    auto actual_val = parse_number_value_if_number(val);
    if (actual_val.type() == typeid(int))
        val = jspp::Object::make_number(std::any_cast<int>(actual_val) - 1);
    else if (actual_val.type() == typeid(double))
        val = jspp::Object::make_number(std::any_cast<double>(actual_val) - 1);
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