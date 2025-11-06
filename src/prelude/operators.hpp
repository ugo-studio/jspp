#pragma once

#include "types.hpp"

namespace jspp
{
    inline bool is_truthy(const AnyValue &val)
    {
        if (std::holds_alternative<JsBoolean>(val))
            return std::get<JsBoolean>(val).value;
        if (std::holds_alternative<JsString>(val))
            return !std::get<JsString>(val).value.empty();
        if (std::holds_alternative<JsNumber>(val))
            return std::get<JsNumber>(val).value != 0.0;
        if (std::holds_alternative<JsUndefined>(val) || std::holds_alternative<JsNull>(val))
            return false;
        if (std::holds_alternative<JsUninitialized>(val))
            Exception::throw_uninitialized_read_property_error();
        return true; // default
    }

    inline bool strict_equals(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (std::holds_alternative<JsBoolean>(lhs) && std::holds_alternative<JsBoolean>(rhs))
            return std::get<JsBoolean>(lhs).value == std::get<JsBoolean>(rhs).value;
        if (std::holds_alternative<JsNumber>(lhs) && std::holds_alternative<JsNumber>(rhs))
            return std::get<JsNumber>(lhs).value == std::get<JsNumber>(rhs).value;
        if (std::holds_alternative<JsString>(lhs) && std::holds_alternative<JsString>(rhs))
            return std::get<JsString>(lhs).value == std::get<JsString>(rhs).value;
        if ((std::holds_alternative<JsObject>(lhs) && std::holds_alternative<JsObject>(rhs)) || (std::holds_alternative<JsArray>(lhs) && std::holds_alternative<JsArray>(rhs)) || (std::holds_alternative<JsFunction>(lhs) && std::holds_alternative<JsFunction>(rhs)))
            return &lhs == &rhs;
        if ((std::holds_alternative<JsBoolean>(lhs) && !std::holds_alternative<JsBoolean>(rhs)) || (std::holds_alternative<JsNumber>(lhs) && !std::holds_alternative<JsNumber>(rhs)) || (std::holds_alternative<JsString>(lhs) && !std::holds_alternative<JsString>(rhs)) || (std::holds_alternative<JsObject>(lhs) && !std::holds_alternative<JsObject>(rhs)) || (std::holds_alternative<JsArray>(lhs) && !std::holds_alternative<JsArray>(rhs)) || (std::holds_alternative<JsFunction>(lhs) && !std::holds_alternative<JsFunction>(rhs)))
            return false;
        if ((std::holds_alternative<JsUndefined>(lhs) && std::holds_alternative<JsUndefined>(rhs)) || (std::holds_alternative<JsNull>(lhs) && std::holds_alternative<JsNull>(rhs)))
            return true;
        if (std::holds_alternative<JsUninitialized>(lhs) || std::holds_alternative<JsUninitialized>(rhs))
            Exception::throw_uninitialized_read_property_error();
        return false; // default
    }
}

// inline bool jspp::strict_equals(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     // Strict equality (===)
//     auto actual_lhs = jspp::Convert::unwrap_number(jspp::Convert::unwrap_boolean(lhs));
//     auto actual_rhs = jspp::Convert::unwrap_number(jspp::Convert::unwrap_boolean(rhs));

//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return std::any_cast<int>(actual_lhs) == std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return std::any_cast<double>(actual_lhs) == std::any_cast<int>(actual_rhs);

//     if (actual_lhs.type() != actual_rhs.type())
//         return false;

//     if (!actual_lhs.has_value() || actual_lhs.type() == typeid(Undefined))
//         return true; // both undefined
//     if (actual_lhs.type() == typeid(Null))
//         return true; // both null

//     if (actual_lhs.type() == typeid(int))
//         return std::any_cast<int>(actual_lhs) == std::any_cast<int>(actual_rhs);
//     if (actual_lhs.type() == typeid(double))
//         return std::any_cast<double>(actual_lhs) == std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(bool))
//         return std::any_cast<bool>(actual_lhs) == std::any_cast<bool>(actual_rhs);
//     if (actual_lhs.type() == typeid(std::string))
//         return std::any_cast<std::string>(actual_lhs) == std::any_cast<std::string>(actual_rhs);
//     if (actual_lhs.type() == typeid(const char *))
//         return strcmp(std::any_cast<const char *>(actual_lhs), std::any_cast<const char *>(actual_rhs)) == 0;

//     if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsObject>))
//     {
//         return std::any_cast<std::shared_ptr<jspp::JsObject>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsObject>>(actual_rhs);
//     }
//     if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsArray>))
//     {
//         return std::any_cast<std::shared_ptr<jspp::JsArray>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsArray>>(actual_rhs);
//     }
//     if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>))
//     {
//         return std::any_cast<std::shared_ptr<jspp::JsString>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsString>>(actual_rhs);
//     }
//     if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>))
//     {
//         return std::any_cast<std::shared_ptr<jspp::JsFunction>>(actual_lhs) == std::any_cast<std::shared_ptr<jspp::JsFunction>>(actual_rhs);
//     }

//     // Cannot compare functions for equality in C++.
//     return false;
// }

// inline bool jspp::equals(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     // Abstract/Loose Equality (==)
//     auto actual_lhs = jspp::Convert::unwrap_number(jspp::Convert::unwrap_boolean(lhs));
//     auto actual_rhs = jspp::Convert::unwrap_number(jspp::Convert::unwrap_boolean(rhs));
//     if (actual_lhs.type() == actual_rhs.type())
//     {
//         return strict_equals(actual_lhs, actual_rhs); // Use strict equality if types are same
//     }
//     // null == undefined
//     if ((actual_lhs.type() == typeid(Null) && (!actual_rhs.has_value() || actual_rhs.type() == typeid(Undefined))) ||
//         ((!actual_lhs.has_value() || actual_lhs.type() == typeid(Undefined)) && actual_rhs.type() == typeid(Null)))
//     {
//         return true;
//     }
//     // number == string
//     if ((actual_lhs.type() == typeid(int) || actual_lhs.type() == typeid(double)) && (actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::string) || actual_rhs.type() == typeid(const char *)))
//     {
//         double l = actual_lhs.type() == typeid(int) ? std::any_cast<int>(actual_lhs) : std::any_cast<double>(actual_lhs);
//         std::string s_rhs = jspp::Convert::to_string(actual_rhs);

//         s_rhs.erase(s_rhs.begin(), std::find_if(s_rhs.begin(), s_rhs.end(), [](int ch)
//                                                 { return !std::isspace(ch); }));
//         s_rhs.erase(std::find_if(s_rhs.rbegin(), s_rhs.rend(), [](int ch)
//                                  { return !std::isspace(ch); })
//                         .base(),
//                     s_rhs.end());

//         if (s_rhs.empty())
//         {
//             return l == 0;
//         }

//         try
//         {
//             double r = std::stod(s_rhs);
//             return l == r;
//         }
//         catch (...)
//         {
//             return false;
//         }
//     }
//     if ((actual_rhs.type() == typeid(int) || actual_rhs.type() == typeid(double)) && (actual_lhs.type() == typeid(std::string) || actual_lhs.type() == typeid(const char *)))
//     {
//         return equals(actual_rhs, actual_lhs); // reuse logic
//     }
//     // boolean == any
//     if (actual_lhs.type() == typeid(bool))
//     {
//         return equals(jspp::AnyValue(std::any_cast<bool>(actual_lhs) ? 1 : 0), actual_rhs);
//     }
//     if (actual_rhs.type() == typeid(bool))
//     {
//         return equals(actual_lhs, jspp::AnyValue(std::any_cast<bool>(actual_rhs) ? 1 : 0));
//     }
//     // object == primitive
//     if ((actual_lhs.type() == typeid(std::shared_ptr<jspp::JsObject>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsArray>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>)) &&
//         (actual_rhs.type() != typeid(std::shared_ptr<jspp::JsObject>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsArray>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsString>) && actual_rhs.type() != typeid(std::shared_ptr<jspp::JsFunction>)))
//     {
//         return equals(jspp::AnyValue(jspp::Convert::to_string(actual_lhs)), actual_rhs);
//     }
//     if ((actual_rhs.type() == typeid(std::shared_ptr<jspp::JsObject>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsArray>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsFunction>)) &&
//         (actual_lhs.type() != typeid(std::shared_ptr<jspp::JsObject>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsArray>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsString>) && actual_lhs.type() != typeid(std::shared_ptr<jspp::JsFunction>)))
//     {
//         return equals(actual_lhs, jspp::AnyValue(jspp::Convert::to_string(actual_rhs)));
//     }
//     return false;
// }

// inline jspp::AnyValue operator+(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) + std::any_cast<int>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) + std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) + std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) + std::any_cast<int>(actual_rhs));
//     if (actual_lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || actual_rhs.type() == typeid(std::shared_ptr<jspp::JsString>))
//     {
//         return jspp::Object::make_string(jspp::Convert::to_string(actual_lhs) + jspp::Convert::to_string(actual_rhs));
//     }
//     return undefined;
// }
// inline jspp::AnyValue operator*(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(static_cast<double>(std::any_cast<int>(actual_lhs)) * static_cast<double>(std::any_cast<int>(actual_rhs)));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) * std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) * std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) * std::any_cast<int>(actual_rhs));
//     return undefined;
// }
// inline jspp::AnyValue operator-(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) - std::any_cast<int>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) - std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) - std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) - std::any_cast<int>(actual_rhs));
//     return undefined;
// }
// inline bool operator<=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return std::any_cast<int>(actual_lhs) <= std::any_cast<int>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return std::any_cast<double>(actual_lhs) <= std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return std::any_cast<int>(actual_lhs) <= std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return std::any_cast<double>(actual_lhs) <= std::any_cast<int>(actual_rhs);
//     return false;
// }
// inline bool operator>(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return std::any_cast<int>(actual_lhs) > std::any_cast<int>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return std::any_cast<double>(actual_lhs) > std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return std::any_cast<int>(actual_lhs) > std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return std::any_cast<double>(actual_lhs) > std::any_cast<int>(actual_rhs);
//     return false;
// }
// inline bool operator<(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return std::any_cast<int>(actual_lhs) < std::any_cast<int>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return std::any_cast<double>(actual_lhs) < std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return std::any_cast<int>(actual_lhs) < std::any_cast<double>(actual_rhs);
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return std::any_cast<double>(actual_lhs) < std::any_cast<int>(actual_rhs);
//     return false;
// }

// inline jspp::AnyValue operator/(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) / std::any_cast<int>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) / std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) / std::any_cast<double>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<double>(actual_lhs) / std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline jspp::AnyValue operator%(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) % std::any_cast<int>(actual_rhs));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::fmod(std::any_cast<double>(actual_lhs), std::any_cast<double>(actual_rhs)));
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(double))
//         return jspp::Object::make_number(std::fmod(static_cast<double>(std::any_cast<int>(actual_lhs)), std::any_cast<double>(actual_rhs)));
//     if (actual_lhs.type() == typeid(double) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::fmod(std::any_cast<double>(actual_lhs), static_cast<double>(std::any_cast<int>(actual_rhs))));
//     return undefined;
// }

// inline jspp::AnyValue operator^(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     auto actual_lhs = jspp::Convert::unwrap_number(lhs);
//     auto actual_rhs = jspp::Convert::unwrap_number(rhs);
//     if (actual_lhs.type() == typeid(int) && actual_rhs.type() == typeid(int))
//         return jspp::Object::make_number(std::any_cast<int>(actual_lhs) ^ std::any_cast<int>(actual_rhs));
//     return undefined;
// }

// inline bool operator>=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     return (lhs > rhs) || jspp::equals(lhs, rhs);
// }

// inline bool operator!=(const jspp::AnyValue &lhs, const jspp::AnyValue &rhs)
// {
//     return !jspp::equals(lhs, rhs);
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
//     if (v.type() == typeid(std::shared_ptr<jspp::AnyValue>))
//     {
//         const auto &ptr = std::any_cast<std::shared_ptr<jspp::AnyValue>>(v);
//         if (ptr)
//             return os << *ptr;
//         else
//         {
//             os << "undefined";
//             return os;
//         }
//     }
//     os << jspp::Convert::to_string(v);
//     return os;
// }
