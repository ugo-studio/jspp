#pragma once

#include "types.hpp"

// --- BASIC ARITHEMETIC
inline jspp::AnyValue operator+(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_number(lhs.as_double() + rhs.as_double());
    if (lhs.is_string() || rhs.is_string())
        return jspp::AnyValue::make_string(lhs.to_std_string() + rhs.to_std_string());
    return jspp::AnyValue::make_nan();
}
inline jspp::AnyValue operator-(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_number(lhs.as_double() - rhs.as_double());
    return jspp::AnyValue::make_nan();
}
inline jspp::AnyValue operator*(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_number(lhs.as_double() * rhs.as_double());
    return jspp::AnyValue::make_nan();
}
inline jspp::AnyValue operator/(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_number(lhs.as_double() / rhs.as_double());
    return jspp::AnyValue::make_nan();
}

// --- COMPARISON OPERATORS
inline jspp::AnyValue operator<(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_boolean(lhs.as_double() < rhs.as_double());
    return jspp::AnyValue::make_boolean(false);
}
inline jspp::AnyValue operator>(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_boolean(lhs.as_double() > rhs.as_double());
    return jspp::AnyValue::make_boolean(false);
}
inline jspp::AnyValue operator<=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
    {
        return jspp::AnyValue::make_boolean(lhs.as_double() <= rhs.as_double());
    }
    return lhs.is_equal_to_primitive(rhs);
}
inline jspp::AnyValue operator>=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
    {
        return jspp::AnyValue::make_boolean(lhs.as_double() >= rhs.as_double());
    }
    return lhs.is_equal_to_primitive(rhs);
}
inline jspp::AnyValue operator==(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    return jspp::AnyValue::make_boolean(lhs.is_equal_to(rhs));
}
inline jspp::AnyValue operator!=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    return jspp::AnyValue::make_boolean(!lhs.is_equal_to(rhs));
}

// inline jspp::AnyValue operator%(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsNumber{std::fmod(std::get<jspp::JsNumber>(lhs).value, std::get<jspp::JsNumber>(rhs).value)};
//     return jspp::AnyValue::make_undefined();
// }
// inline jspp::AnyValue operator^(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsNumber{
//             static_cast<double>(
//                 static_cast<int>(std::get<jspp::JsNumber>(lhs).value) ^
//                 static_cast<int>(std::get<jspp::JsNumber>(rhs).value))};
//     return jspp::AnyValue::make_undefined();
// }

// inline jspp::AnyValue operator&(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) & std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline jspp::AnyValue operator|(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) | std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline jspp::AnyValue operator~(const jspp::AnyValue &val)
// {
//     auto actual_val = jspp::Convert::unwrap_number(val);
//     if (actual_val.type() == typeid(int))
//         return jspp::Object::make_number(~std::any_cast<int>(actual_val));
//     return undefined;
// }

// inline jspp::AnyValue operator-(const jspp::AnyValue &val)
// {
//     auto actual_val = jspp::Convert::unwrap_number(val);
//     if (actual_val.type() == typeid(int))
//         return jspp::Object::make_number(-std::any_cast<int>(actual_val));
//     if (actual_val.type() == typeid(double))
//         return jspp::Object::make_number(-std::any_cast<double>(actual_val));
//     return undefined;
// }

// inline jspp::AnyValue operator<<(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) << std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline jspp::AnyValue operator>>(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) >> std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline jspp::AnyValue jspp::pow(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::pow(std::any_cast<int>(actual_lhs), std::any_cast<int>(actual_rhs)));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::pow(std::any_cast<double>(actual_lhs), std::any_cast<double>(actual_rhs)));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::pow(std::any_cast<int>(actual_lhs), std::any_cast<double>(actual_rhs)));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::pow(std::any_cast<double>(actual_lhs), std::any_cast<int>(actual_rhs)));
//     return undefined;
// }

// inline jspp::AnyValue &operator++(jspp::AnyValue &val)
// {
//     // For in-place operators, it's more complex. We'll modify the original value
//     // if it's a number, but this implementation assumes it's not a JsNumber object.
//     // A more robust solution might require changing the JsNumber's internal value.
//     auto actual_val = jspp::Convert::unwrap_number(val);
//     if (actual_val.type() == typeid(int))
//         val = jspp::Object::make_number(std::any_cast<int>(actual_val) + 1);
//     else if (actual_val.type() == typeid(double))
//         val = jspp::Object::make_number(std::any_cast<double>(actual_val) + 1);
//     return val;
// }

// inline jspp::AnyValue operator++(jspp::AnyValue &val, int)
// {
//     jspp::AnyValue old = val;
//     ++val;
//     return old;
// }

// inline jspp::AnyValue &operator--(jspp::AnyValue &val)
// {
//     auto actual_val = jspp::Convert::unwrap_number(val);
//     if (actual_val.type() == typeid(int))
//         val = jspp::Object::make_number(std::any_cast<int>(actual_val) - 1);
//     else if (actual_val.type() == typeid(double))
//         val = jspp::Object::make_number(std::any_cast<double>(actual_val) - 1);
//     return val;
// }

// inline jspp::AnyValue operator--(jspp::AnyValue &val, int)
// {
//     jspp::AnyValue old = val;
//     --val;
//     return old;
// }

// inline jspp::AnyValue &operator+=(jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     lhs = lhs + rhs;
//     return lhs;
// }

// inline jspp::AnyValue &operator-=(jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     lhs = lhs - rhs;
//     return lhs;
// }

// inline jspp::AnyValue &operator*=(jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     lhs = lhs * rhs;
//     return lhs;
// }

// inline jspp::AnyValue &operator/=(jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     lhs = lhs / rhs;
//     return lhs;
// }

// inline jspp::AnyValue &operator%=(jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     lhs = lhs % rhs;
//     return lhs;
// }
