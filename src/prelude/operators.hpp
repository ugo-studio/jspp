#pragma once

#include "types.hpp"

inline bool jspp::is_truthy(const AnyValue &val)
{
    if (val.is_boolean())
        return val.as_boolean();
    if (val.is_string())
        return !val.as_string()->empty();
    if (val.is_number())
        return val.as_double() != 0.0;
    if (val.is_undefined() || val.is_null())
        return false;
    if (val.is_uninitialized())
        // Exception::throw_uninitialized_read_property_error();
        throw std::runtime_error("TypeError: Cannot read properties of uninitialized");
    return true; // default
}

// inline jspp::JsBoolean jspp::strict_equals(const AnyValue &lhs, const AnyValue &rhs)
// {
//     if (&lhs == &rhs)
//         return JsBoolean{true};
//     if (std::holds_alternative<JsBoolean>(lhs) && std::holds_alternative<JsBoolean>(rhs))
//         return JsBoolean{std::get<JsBoolean>(lhs).value == std::get<JsBoolean>(rhs).value};
//     if (std::holds_alternative<JsNumber>(lhs) && std::holds_alternative<JsNumber>(rhs))
//         return JsBoolean{std::get<JsNumber>(lhs).value == std::get<JsNumber>(rhs).value};
//     if (std::holds_alternative<JsString>(lhs) && std::holds_alternative<JsString>(rhs))
//         return JsBoolean{std::get<JsString>(lhs).value == std::get<JsString>(rhs).value};
//     if ((std::holds_alternative<std::shared_ptr<JsObject>>(lhs) && std::holds_alternative<std::shared_ptr<JsObject>>(rhs)) || (std::holds_alternative<std::shared_ptr<JsArray>>(lhs) && std::holds_alternative<std::shared_ptr<JsArray>>(rhs)) || (std::holds_alternative<std::shared_ptr<JsFunction>>(lhs) && std::holds_alternative<std::shared_ptr<JsFunction>>(rhs)))
//         return JsBoolean{&lhs == &rhs};
//     if ((std::holds_alternative<JsBoolean>(lhs) && !std::holds_alternative<JsBoolean>(rhs)) || (std::holds_alternative<JsNumber>(lhs) && !std::holds_alternative<JsNumber>(rhs)) || (std::holds_alternative<JsString>(lhs) && !std::holds_alternative<JsString>(rhs)) || (std::holds_alternative<std::shared_ptr<JsObject>>(lhs) && !std::holds_alternative<std::shared_ptr<JsObject>>(rhs)) || (std::holds_alternative<std::shared_ptr<JsArray>>(lhs) && !std::holds_alternative<std::shared_ptr<JsArray>>(rhs)) || (std::holds_alternative<std::shared_ptr<JsFunction>>(lhs) && !std::holds_alternative<std::shared_ptr<JsFunction>>(rhs)))
//         return JsBoolean{false};
//     if ((std::holds_alternative<JsUndefined>(lhs) && std::holds_alternative<JsUndefined>(rhs)) || (std::holds_alternative<JsNull>(lhs) && std::holds_alternative<JsNull>(rhs)))
//         return JsBoolean{true};
//     if (std::holds_alternative<JsUninitialized>(lhs) || std::holds_alternative<JsUninitialized>(rhs))
//         // Exception::throw_uninitialized_read_property_error();
//         throw std::runtime_error("TypeError: Cannot read properties of uninitialized");
//     return JsBoolean{false}; // default
// }
// inline jspp::JsBoolean jspp::equals(const AnyValue &lhs, const AnyValue &rhs)
// {
//     // Abstract/Loose Equality (==)
//     if (&lhs == &rhs)
//         return JsBoolean{true};
//     // null == undefined
//     if ((std::holds_alternative<JsNull>(lhs) || std::holds_alternative<JsUndefined>(lhs)) && (std::holds_alternative<JsNull>(rhs) || std::holds_alternative<JsUndefined>(rhs)))
//         return JsBoolean{true};
//     // number == string
//     if (std::holds_alternative<JsNumber>(lhs) && std::holds_alternative<JsString>(rhs))
//         return JsBoolean{std::get<JsNumber>(lhs).to_raw_string() == std::get<JsString>(rhs).to_raw_string()};
//     if (std::holds_alternative<JsString>(lhs) && std::holds_alternative<JsNumber>(rhs))
//         return JsBoolean{std::get<JsString>(lhs).to_raw_string() == std::get<JsNumber>(rhs).to_raw_string()};
//     // boolean == any
//     if (std::holds_alternative<JsBoolean>(lhs))
//         return jspp::equals(jspp::JsNumber{std::get<JsBoolean>(lhs).value ? 1.0 : 0}, rhs);
//     if (std::holds_alternative<JsBoolean>(rhs))
//         return jspp::equals(lhs, jspp::JsNumber{std::get<JsBoolean>(rhs).value ? 1.0 : 0});
//     // object == primitive
//     if ((std::holds_alternative<JsString>(lhs) || std::holds_alternative<std::shared_ptr<JsObject>>(lhs) || std::holds_alternative<std::shared_ptr<JsArray>>(lhs) || std::holds_alternative<std::shared_ptr<JsFunction>>(lhs)) &&
//         (!std::holds_alternative<JsString>(rhs) && !std::holds_alternative<std::shared_ptr<JsObject>>(rhs) && !std::holds_alternative<std::shared_ptr<JsArray>>(rhs) && !std::holds_alternative<std::shared_ptr<JsFunction>>(rhs)))
//         return jspp::equals(jspp::JsString{jspp::Convert::to_raw_string(lhs)}, rhs);
//     if ((std::holds_alternative<JsString>(rhs) || std::holds_alternative<std::shared_ptr<JsObject>>(rhs) || std::holds_alternative<std::shared_ptr<JsArray>>(rhs) || std::holds_alternative<std::shared_ptr<JsFunction>>(rhs)) &&
//         (!std::holds_alternative<JsString>(lhs) && !std::holds_alternative<std::shared_ptr<JsObject>>(lhs) && !std::holds_alternative<std::shared_ptr<JsArray>>(lhs) && !std::holds_alternative<std::shared_ptr<JsFunction>>(lhs)))
//         return jspp::equals(lhs, jspp::JsString{jspp::Convert::to_raw_string(rhs)});
//     return JsBoolean{false}; // default
// }

inline jspp::AnyValue operator+(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_number(lhs.as_double() + rhs.as_double());
    return jspp::AnyValue::make_undefined();
}
// inline jspp::AnyValue operator*(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsNumber{std::get<jspp::JsNumber>(lhs).value * std::get<jspp::JsNumber>(rhs).value};
//     return jspp::AnyValue::make_undefined();
// }
// inline jspp::AnyValue operator-(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsNumber{std::get<jspp::JsNumber>(lhs).value - std::get<jspp::JsNumber>(rhs).value};
//     return jspp::AnyValue::make_undefined();
// }

// inline jspp::JsBoolean operator<=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsBoolean{(std::get<jspp::JsNumber>(lhs).value <= std::get<jspp::JsNumber>(rhs).value) || jspp::equals(std::get<jspp::JsNumber>(lhs), std::get<jspp::JsNumber>(rhs)).value};
//     return jspp::JsBoolean{false};
// }
// inline jspp::JsBoolean operator>=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsBoolean{(std::get<jspp::JsNumber>(lhs).value >= std::get<jspp::JsNumber>(rhs).value) || jspp::equals(std::get<jspp::JsNumber>(lhs), std::get<jspp::JsNumber>(rhs)).value};
//     return jspp::JsBoolean{false};
// }
inline jspp::AnyValue operator<(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
{
    if (lhs.is_number() && rhs.is_number())
        return jspp::AnyValue::make_boolean(lhs.as_double() < rhs.as_double());
    return jspp::AnyValue::make_boolean(false);
}
// inline jspp::JsBoolean operator>(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsBoolean{std::get<jspp::JsNumber>(lhs).value > std::get<jspp::JsNumber>(rhs).value};
//     return jspp::JsBoolean{false};
// }
// inline jspp::JsBoolean operator!=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsBoolean{!jspp::equals(std::get<jspp::JsNumber>(lhs), std::get<jspp::JsNumber>(rhs)).value};
//     return jspp::JsBoolean{false};
// }

// inline jspp::AnyValue operator/(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     if (std::holds_alternative<jspp::JsNumber>(lhs) && std::holds_alternative<jspp::JsNumber>(rhs))
//         return jspp::JsNumber{std::get<jspp::JsNumber>(lhs).value / std::get<jspp::JsNumber>(rhs).value};
//     return jspp::AnyValue::make_undefined();
// }
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

// // Define operators for osstream
// template <class... Ts>
// struct overloaded : Ts...
// {
//     using Ts::operator()...;
// };
// template <class... Ts>
// overloaded(Ts...) -> overloaded<Ts...>;

// inline std::ostream &operator<<(std::ostream &os, const jspp::AnyValue &v)
// {
//     os << v.convert_to_raw_string();
//     return os;
// }
