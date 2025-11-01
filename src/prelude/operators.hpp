#pragma once

#include "types.hpp"
#include "access.hpp"
#include "convert.hpp"
#include "object.hpp"
#include <cmath> // Required for std::pow

// Define operators for JsValue
inline bool jspp::is_truthy(const jspp::JsValue &val)
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
    if (val.type() == typeid(jspp::Uninitialized))
        return false;
    if (val.type() == typeid(std::shared_ptr<jspp::JsObject>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsObject>>(val) != nullptr;
    }
    if (val.type() == typeid(std::shared_ptr<jspp::JsArray>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsArray>>(val) != nullptr;
    }
    if (val.type() == typeid(std::shared_ptr<jspp::JsString>))
    {
        auto s = std::any_cast<std::shared_ptr<jspp::JsString>>(val);
        return s && !s->value.empty();
    }
    if (val.type() == typeid(std::shared_ptr<jspp::JsFunction>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsFunction>>(val) != nullptr;
    }
    if (val.type() == typeid(std::function<jspp::JsValue(const std::vector<jspp::JsValue> &)>))
    {
        return true;
    }
    return true;
}

inline bool jspp::strict_equals(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    // Strict equality (===)
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);

    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
        return std::any_cast<int>(actual_lhs) == std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
        return std::any_cast<double>(actual_lhs) == std::any_cast<int>(actual_rhs);

    if (actual_lhs.type() != actual_rhs.type())
        return false;

    if (!actual_lhs.has_value() || actual_lhs.type() == typeid(Undefined))
        return true; // both undefined
    if (actual_lhs.type() == typeid(Null))
        return true; // both null

    if (actual_lhs.type() == typeid(int))
        return std::any_cast<int>(actual_lhs) == std::any_cast<int>(actual_rhs);
    if (actual_lhs.type() == typeid(double))
        return std::any_cast<double>(actual_lhs) == std::any_cast<double>(actual_rhs);
    if (actual_lhs.type() == typeid(bool))
        return std::any_cast<bool>(actual_lhs) == std::any_cast<bool>(actual_rhs);
    if (actual_lhs.type() == typeid(std::string))
        return std::any_cast<std::string>(actual_lhs) == std::any_cast<std::string>(actual_rhs);
    if (actual_lhs.type() == typeid(const char *))
        return strcmp(std::any_cast<const char *>(actual_lhs), std::any_cast<const char *>(actual_rhs)) == 0;

    if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsObject>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsObject>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsObject>>(actual_rhs);
    }
    if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsArray>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsArray>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsArray>>(actual_rhs);
    }
    if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsString>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsString>>(actual_rhs);
    }
    if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>))
    {
        return std::any_cast<std::shared_ptr<jspp::JsFunction>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsFunction>>(actual_rhs);
    }

    // Cannot compare functions for equality in C++.
    return false;
}

inline bool jspp::equals(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    // Abstract/Loose Equality (==)
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == actual_rhs.type())
    {
        return strict_equals(actual_lhs, actual_rhs); // Use strict equality if types are same
    }
    // null == undefined
    if ((actual_lhs.type() == typeid(Null) && (!actual_rhs.has_value() || actual_rhs.type() == typeid(Undefined))) ||
        ((!actual_lhs.has_value() || actual_lhs.type() == typeid(Undefined)) && actual_rhs.type() == typeid(Null)))
    {
        return true;
    }
    // number == string
    if ((actual_lhs.type() == typeid(int) || actual_lhs.type() == typeid(double)) && (actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::string) || actual_rhs.type() == typeid(const char *)))
    {
        double l = actual_lhs.type() == typeid(int) ? std::any_cast<int>(actual_lhs) : std::any_cast<double>(actual_lhs);
        std::string s_rhs = jspp::Convert::to_string(actual_rhs);

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
    if ((actual_rhs.type() == typeid(int) || actual_rhs.type() == typeid(double)) && (actual_lhs.type() == typeid(std::string) || actual_lhs.type() == typeid(const char *)))
    {
        return equals(actual_rhs, actual_lhs); // reuse logic
    }
    // boolean == any
    if (actual_lhs.type() == typeid(bool))
    {
        return equals(jspp::JsValue(std::any_cast<bool>(actual_lhs) ? 1 : 0), actual_rhs);
    }
    if (actual_rhs.type() == typeid(bool))
    {
        return equals(actual_lhs, jspp::JsValue(std::any_cast<bool>(actual_rhs) ? 1 : 0));
    }
    // object == primitive
    if ((actual_lhs.type() == typeid(std::shared_ptr<jspp::JsObject>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsArray>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>)) &&
        (actual_rhs.type() != typeid(std::shared_ptr<jspp::JsObject>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsArray>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsString>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsFunction>)))
    {
        return equals(jspp::JsValue(jspp::Convert::to_string(actual_lhs)), actual_rhs);
    }
    if ((actual_rhs.type() == typeid(std::shared_ptr<jspp::JsObject>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsArray>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsFunction>)) &&
        (actual_lhs.type() != typeid(std::shared_ptr<jspp::JsObject>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsArray>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsString>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsFunction>)))
    {
        return equals(actual_lhs, jspp::JsValue(jspp::Convert::to_string(actual_rhs)));
    }
    return false;
}

inline jspp::JsValue operator+(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) % std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator^(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) ^ std::any_cast<int>(actual_rhs));
    return undefined;
}

inline bool operator>=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return (lhs > rhs) || jspp::equals(lhs, rhs);
}

inline bool operator!=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return !jspp::equals(lhs, rhs);
}

inline jspp::JsValue operator&(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) & std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator|(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) | std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator~(const jspp::JsValue &val)
{
    auto actual_val = jspp::Convert::unwrap_number(val);
    if (actual_val.type() == typeid(int))
        return jspp::Object::make_number(~std::any_cast<int>(actual_val));
    return undefined;
}

inline jspp::JsValue operator-(const jspp::JsValue &val)
{
    auto actual_val = jspp::Convert::unwrap_number(val);
    if (actual_val.type() == typeid(int))
        return jspp::Object::make_number(-std::any_cast<int>(actual_val));
    if (actual_val.type() == typeid(double))
        return jspp::Object::make_number(-std::any_cast<double>(actual_val));
    return undefined;
}

inline jspp::JsValue operator<<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) << std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue operator>>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
    if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
        return jspp::Object::make_number(std::any_cast<int>(actual_lhs) >> std::any_cast<int>(actual_rhs));
    return undefined;
}

inline jspp::JsValue jspp::pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    auto actual_lhs = jspp::Convert::unwrap_number(lhs);
    auto actual_rhs = jspp::Convert::unwrap_number(rhs);
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
    auto actual_val = jspp::Convert::unwrap_number(val);
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
    auto actual_val = jspp::Convert::unwrap_number(val);
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