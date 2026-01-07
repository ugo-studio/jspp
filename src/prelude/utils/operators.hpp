#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include <cstdint>   // For int32_t
#include <cmath>     // For fmod, isnan, isinf, floor, abs, pow
#include <string>    // For std::to_string, std::stod
#include <algorithm> // For std::all_of
#include <limits>    // For numeric_limits

namespace jspp
{
    // Private namespace for helper functions that implement JS type conversions.
    namespace Operators_Private
    {
        // Implements the ToNumber abstract operation from ECMA-262.
        inline double ToNumber(const AnyValue &val)
        {
            switch (val.get_type())
            {
            case JsType::Number:
                return val.as_double();
            case JsType::Null:
                return 0.0;
            case JsType::Uninitialized:
            case JsType::Undefined:
                return std::numeric_limits<double>::quiet_NaN();
            case JsType::Boolean:
                return val.as_boolean() ? 1.0 : 0.0;
            case JsType::String:
            {
                const std::string &s = val.as_string()->value;
                if (s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c)
                                             { return std::isspace(c); }))
                    return 0.0;
                try
                {
                    size_t pos;
                    double num = std::stod(s, &pos);
                    while (pos < s.length() && std::isspace(static_cast<unsigned char>(s[pos])))
                        pos++;
                    if (pos != s.length())
                        return std::numeric_limits<double>::quiet_NaN();
                    return num;
                }
                catch (...)
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
            }
            default:
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        // Implements the ToInt32 abstract operation from ECMA-262.
        inline int32_t ToInt32(const AnyValue &val)
        {
            double num = ToNumber(val);

            if (std::isnan(num) || std::isinf(num) || num == 0)
                return 0;

            double posInt = std::signbit(num) ? -std::floor(std::abs(num)) : std::floor(std::abs(num));
            double int32bit = fmod(posInt, 4294967296.0); // 2^32

            if (int32bit >= 2147483648.0) // 2^31
                return static_cast<int32_t>(int32bit - 4294967296.0);
            else
                return static_cast<int32_t>(int32bit);
        }
        // Implements the ToUint32 abstract operation from ECMA-262.
        inline uint32_t ToUint32(const AnyValue &val)
        {
            double num = ToNumber(val);
            if (std::isnan(num) || std::isinf(num) || num == 0)
                return 0;
            double posInt = std::signbit(num) ? -std::floor(std::abs(num)) : std::floor(std::abs(num));
            uint32_t uint32bit = static_cast<uint32_t>(fmod(posInt, 4294967296.0));
            return uint32bit;
        }
    }

    // --- TRUTHY CHECKER ---
    inline const bool is_truthy(const double &val) noexcept
    {
        return val != 0.0 && !std::isnan(val);
    }
    inline const bool is_truthy(const std::string &val) noexcept
    {
        return !val.empty();
    }
    inline const bool is_truthy(const AnyValue &val) noexcept
    {
        switch (val.get_type())
        {
        case JsType::Number:
            return is_truthy(val.as_double());
        case JsType::String:
            return is_truthy(val.as_string()->value);
        case JsType::Boolean:
            return val.as_boolean();
        case JsType::Null:
        case JsType::Undefined:
        case JsType::Uninitialized:
            return false;
        default:
            return true;
        }
    }

    // --- BASIC EQUALITY ---

    // Operator === (returns primitive boolean)
    inline const bool is_strictly_equal_to_primitive(const AnyValue &lhs, const double &rhs) noexcept
    {
        if (lhs.is_number())
            return lhs.as_double() == rhs;
        return false;
    }
    inline const bool is_strictly_equal_to_primitive(const double &lhs, const AnyValue &rhs) noexcept
    {
        if (rhs.is_number())
            return lhs == rhs.as_double();
        return false;
    }
    inline const bool is_strictly_equal_to_primitive(const double &lhs, const double &rhs) noexcept
    {
        return lhs == rhs;
    }
    inline const bool is_strictly_equal_to_primitive(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        JsType type = lhs.get_type();
        if (type != rhs.get_type())
            return false;
        switch (type)
        {
        case JsType::Boolean:
            return lhs.as_boolean() == rhs.as_boolean();
        case JsType::Number:
            return lhs.as_double() == rhs.as_double();
        case JsType::String:
            return lhs.as_string()->value == rhs.as_string()->value;
        case JsType::Array:
            return lhs.as_array() == rhs.as_array();
        case JsType::Object:
            return lhs.as_object() == rhs.as_object();
        case JsType::Function:
            return lhs.as_function() == rhs.as_function();
        case JsType::Iterator:
            return lhs.as_iterator() == rhs.as_iterator();
        case JsType::AsyncIterator:
            return lhs.as_async_iterator() == rhs.as_async_iterator();
        case JsType::Promise:
            return lhs.as_promise() == rhs.as_promise();
        case JsType::Symbol:
            return lhs.as_symbol() == rhs.as_symbol();
        case JsType::DataDescriptor:
            return lhs.as_data_descriptor() == rhs.as_data_descriptor();
        case JsType::AccessorDescriptor:
            return lhs.as_accessor_descriptor() == rhs.as_accessor_descriptor();
        default:
            return true;
        }
    }

    // Operator == (returns primitive boolean)
    inline const bool is_equal_to_primitive(const AnyValue &lhs, const double &rhs) noexcept
    {
        return is_equal_to_primitive(lhs, AnyValue::make_number(rhs));
    }
    inline const bool is_equal_to_primitive(const double &lhs, const AnyValue &rhs) noexcept
    {
        return is_equal_to_primitive(AnyValue::make_number(lhs), rhs);
    }
    inline const bool is_equal_to_primitive(const double &lhs, const double &rhs) noexcept
    {
        return lhs == rhs;
    }
    inline const bool is_equal_to_primitive(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        JsType lhs_type = lhs.get_type();
        JsType rhs_type = rhs.get_type();
        if (lhs_type == rhs_type)
        {
            return is_strictly_equal_to_primitive(lhs, rhs);
        }
        if ((lhs_type == JsType::Null && rhs_type == JsType::Undefined) || (lhs_type == JsType::Undefined && rhs_type == JsType::Null))
        {
            return true;
        }
        if (lhs_type == JsType::Number && rhs_type == JsType::String)
        {
            return lhs.as_double() == Operators_Private::ToNumber(rhs);
        }
        if (lhs_type == JsType::String && rhs_type == JsType::Number)
        {
            return Operators_Private::ToNumber(lhs) == rhs.as_double();
        }
        if (lhs_type == JsType::Boolean)
        {
            return is_equal_to_primitive(lhs.as_boolean() ? 1.0 : 0.0, rhs);
        }
        if (rhs_type == JsType::Boolean)
        {
            return is_equal_to_primitive(lhs, AnyValue::make_number(rhs.as_boolean() ? 1.0 : 0.0));
        }
        if ((lhs_type == JsType::Object || lhs_type == JsType::Array || lhs_type == JsType::Function || lhs_type == JsType::Promise || lhs_type == JsType::Iterator) &&
            (rhs_type == JsType::String || rhs_type == JsType::Number || rhs_type == JsType::Symbol))
        {
            return is_equal_to_primitive(AnyValue::make_string(lhs.to_std_string()), rhs);
        }
        if ((rhs_type == JsType::Object || rhs_type == JsType::Array || rhs_type == JsType::Function || rhs_type == JsType::Promise || rhs_type == JsType::Iterator) &&
            (lhs_type == JsType::String || lhs_type == JsType::Number || lhs_type == JsType::Symbol))
        {
            return is_equal_to_primitive(lhs, AnyValue::make_string(rhs.to_std_string()));
        }
        if (lhs_type == JsType::DataDescriptor || lhs_type == JsType::AccessorDescriptor)
        {
            return is_strictly_equal_to_primitive(lhs, rhs);
        }
        return false;
    }

    // Operator === (returns boolean wrapped in AnyValue)
    inline const AnyValue is_strictly_equal_to(const AnyValue &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_strictly_equal_to(const double &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_strictly_equal_to(const double &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_strictly_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(is_strictly_equal_to_primitive(lhs, rhs));
    }

    // Operator == (returns boolean wrapped in AnyValue)
    inline const AnyValue is_equal_to(const AnyValue &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_equal_to(const double &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_equal_to(const double &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue is_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }

    // Operator !== (returns boolean wrapped in AnyValue)
    inline const AnyValue not_strictly_equal_to(const AnyValue &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_strictly_equal_to(const double &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_strictly_equal_to(const double &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_strictly_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_strictly_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_strictly_equal_to_primitive(lhs, rhs));
    }

    // Operator != (returns boolean wrapped in AnyValue)
    inline const AnyValue not_equal_to(const AnyValue &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_equal_to(const double &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_equal_to(const double &lhs, const double &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs));
    }
    inline const AnyValue not_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept
    {
        return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs));
    }

    // --- BASIC ARITHMETIC ---

    // Operator +
    inline AnyValue operator+(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(lhs.as_double() + rhs.as_double());
        if (lhs.is_string() || rhs.is_string())
            return AnyValue::make_string(lhs.to_std_string() + rhs.to_std_string());
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) + Operators_Private::ToNumber(rhs));
    }
    inline AnyValue operator+(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(lhs.as_double() + rhs);
        if (lhs.is_string())
            return AnyValue::make_string(lhs.to_std_string() + std::to_string(rhs));
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) + rhs);
    }
    inline AnyValue operator+(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(lhs + rhs.as_double());
        if (rhs.is_string())
            return AnyValue::make_string(std::to_string(lhs) + rhs.to_std_string());
        return AnyValue::make_number(lhs + Operators_Private::ToNumber(rhs));
    }

    // Operator -
    inline AnyValue operator-(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(lhs.as_double() - rhs.as_double());
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) - Operators_Private::ToNumber(rhs));
    }
    inline AnyValue operator-(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(lhs.as_double() - rhs);
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) - rhs);
    }
    inline AnyValue operator-(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(lhs - rhs.as_double());
        return AnyValue::make_number(lhs - Operators_Private::ToNumber(rhs));
    }

    // Operator *
    inline AnyValue operator*(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(lhs.as_double() * rhs.as_double());
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) * Operators_Private::ToNumber(rhs));
    }
    inline AnyValue operator*(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(lhs.as_double() * rhs);
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) * rhs);
    }
    inline AnyValue operator*(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(lhs * rhs.as_double());
        return AnyValue::make_number(lhs * Operators_Private::ToNumber(rhs));
    }

    // Operator /
    inline AnyValue operator/(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(lhs.as_double() / rhs.as_double());
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) / Operators_Private::ToNumber(rhs));
    }
    inline AnyValue operator/(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(lhs.as_double() / rhs);
        return AnyValue::make_number(Operators_Private::ToNumber(lhs) / rhs);
    }
    inline AnyValue operator/(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(lhs / rhs.as_double());
        return AnyValue::make_number(lhs / Operators_Private::ToNumber(rhs));
    }

    // Operator %
    inline AnyValue operator%(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(std::fmod(lhs.as_double(), rhs.as_double()));
        return AnyValue::make_number(std::fmod(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)));
    }
    inline AnyValue operator%(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(std::fmod(lhs.as_double(), rhs));
        return AnyValue::make_number(std::fmod(Operators_Private::ToNumber(lhs), rhs));
    }
    inline AnyValue operator%(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(std::fmod(lhs, rhs.as_double()));
        return AnyValue::make_number(std::fmod(lhs, Operators_Private::ToNumber(rhs)));
    }

    // --- UNARY OPERATORS ---
    inline AnyValue operator+(const AnyValue &val)
    {
        return AnyValue::make_number(Operators_Private::ToNumber(val));
    }
    inline AnyValue operator-(const AnyValue &val)
    {
        return AnyValue::make_number(-Operators_Private::ToNumber(val));
    }
    inline AnyValue operator~(const AnyValue &val)
    {
        return AnyValue::make_number(~Operators_Private::ToInt32(val));
    }
    inline AnyValue operator!(const AnyValue &val)
    {
        return AnyValue::make_boolean(!is_truthy(val));
    }

    // --- EXPONENTIATION ---
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs)
    {
        double base = Operators_Private::ToNumber(lhs);
        double exp = Operators_Private::ToNumber(rhs);
        return AnyValue::make_number(std::pow(base, exp));
    }
    inline AnyValue pow(const AnyValue &lhs, const double &rhs)
    {
        double base = Operators_Private::ToNumber(lhs);
        return AnyValue::make_number(std::pow(base, rhs));
    }
    inline AnyValue pow(const double &lhs, const AnyValue &rhs)
    {
        double exp = Operators_Private::ToNumber(rhs);
        return AnyValue::make_number(std::pow(lhs, exp));
    }
    inline AnyValue pow(const double &lhs, const double &rhs)
    {
        return AnyValue::make_number(std::pow(lhs, rhs));
    }

    // --- COMPARISON OPERATORS ---

    // Less than <
    inline AnyValue operator<(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_boolean(lhs.as_double() < rhs.as_double());

        if (lhs.is_string() && rhs.is_string())
            return AnyValue::make_boolean(lhs.as_string()->value < rhs.as_string()->value);

        double l = Operators_Private::ToNumber(lhs);
        double r = Operators_Private::ToNumber(rhs);

        if (std::isnan(l) || std::isnan(r))
            return Constants::FALSE;

        return AnyValue::make_boolean(l < r);
    }
    inline AnyValue operator<(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_boolean(lhs.as_double() < rhs);

        double l = Operators_Private::ToNumber(lhs);
        if (std::isnan(l) || std::isnan(rhs))
            return Constants::FALSE;

        return AnyValue::make_boolean(l < rhs);
    }
    inline AnyValue operator<(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_boolean(lhs < rhs.as_double());

        double r = Operators_Private::ToNumber(rhs);
        if (std::isnan(lhs) || std::isnan(r))
            return Constants::FALSE;

        return AnyValue::make_boolean(lhs < r);
    }

    inline AnyValue operator>(const AnyValue &lhs, const AnyValue &rhs) { return rhs < lhs; }
    inline AnyValue operator>(const AnyValue &lhs, const double &rhs) { return operator<(rhs, lhs); }
    inline AnyValue operator>(const double &lhs, const AnyValue &rhs) { return operator<(rhs, lhs); }

    inline AnyValue operator<=(const AnyValue &lhs, const AnyValue &rhs)
    {
        AnyValue result = rhs < lhs;
        return AnyValue::make_boolean(!result.as_boolean());
    }
    inline AnyValue operator<=(const AnyValue &lhs, const double &rhs)
    {
        AnyValue result = operator<(rhs, lhs);
        return AnyValue::make_boolean(!result.as_boolean());
    }
    inline AnyValue operator<=(const double &lhs, const AnyValue &rhs)
    {
        AnyValue result = operator<(rhs, lhs);
        return AnyValue::make_boolean(!result.as_boolean());
    }

    inline AnyValue operator>=(const AnyValue &lhs, const AnyValue &rhs)
    {
        AnyValue result = lhs < rhs;
        return AnyValue::make_boolean(!result.as_boolean());
    }
    inline AnyValue operator>=(const AnyValue &lhs, const double &rhs)
    {
        AnyValue result = operator<(lhs, rhs);
        return AnyValue::make_boolean(!result.as_boolean());
    }
    inline AnyValue operator>=(const double &lhs, const AnyValue &rhs)
    {
        AnyValue result = operator<(lhs, rhs);
        return AnyValue::make_boolean(!result.as_boolean());
    }

    // Equality ==
    inline AnyValue operator==(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }
    inline AnyValue operator==(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_boolean(lhs.as_double() == rhs);
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, AnyValue::make_number(rhs)));
    }
    inline AnyValue operator==(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_boolean(lhs == rhs.as_double());
        return AnyValue::make_boolean(is_equal_to_primitive(rhs, AnyValue::make_number(lhs)));
    }

    inline AnyValue operator!=(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs)); }
    inline AnyValue operator!=(const AnyValue &lhs, const double &rhs) { return AnyValue::make_boolean(!operator==(lhs, rhs).as_boolean()); }
    inline AnyValue operator!=(const double &lhs, const AnyValue &rhs) { return AnyValue::make_boolean(!operator==(lhs, rhs).as_boolean()); }

    // --- LOGICAL OPERATORS ---
    inline AnyValue operator||(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (is_truthy(lhs))
            return lhs;
        return rhs;
    }
    inline AnyValue operator&&(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (!is_truthy(lhs))
            return lhs;
        return rhs;
    }

    // --- BITWISE OPERATORS ---
    inline AnyValue operator^(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) ^ Operators_Private::ToInt32(rhs));
    }
    inline AnyValue operator^(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) ^ static_cast<int32_t>(rhs));
    }
    inline AnyValue operator^(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<int32_t>(lhs) ^ Operators_Private::ToInt32(rhs));
    }

    inline AnyValue operator&(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) & Operators_Private::ToInt32(rhs));
    }
    inline AnyValue operator&(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) & static_cast<int32_t>(rhs));
    }
    inline AnyValue operator&(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<int32_t>(lhs) & Operators_Private::ToInt32(rhs));
    }

    inline AnyValue operator|(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) | Operators_Private::ToInt32(rhs));
    }
    inline AnyValue operator|(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) | static_cast<int32_t>(rhs));
    }
    inline AnyValue operator|(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<int32_t>(lhs) | Operators_Private::ToInt32(rhs));
    }

    // --- SHIFT OPERATORS ---
    inline AnyValue operator<<(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) << (Operators_Private::ToInt32(rhs) & 0x1F));
    }
    inline AnyValue operator<<(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) << (static_cast<int32_t>(rhs) & 0x1F));
    }
    inline AnyValue operator<<(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<int32_t>(lhs) << (Operators_Private::ToInt32(rhs) & 0x1F));
    }

    inline AnyValue operator>>(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) >> (Operators_Private::ToInt32(rhs) & 0x1F));
    }
    inline AnyValue operator>>(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(Operators_Private::ToInt32(lhs) >> (static_cast<int32_t>(rhs) & 0x1F));
    }
    inline AnyValue operator>>(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<int32_t>(lhs) >> (Operators_Private::ToInt32(rhs) & 0x1F));
    }

    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_number(static_cast<double>(Operators_Private::ToUint32(lhs) >> (Operators_Private::ToInt32(rhs) & 0x1F)));
    }
    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_number(static_cast<double>(Operators_Private::ToUint32(lhs) >> (static_cast<int32_t>(rhs) & 0x1F)));
    }
    inline AnyValue unsigned_right_shift(const double &lhs, const AnyValue &rhs)
    {
        uint32_t l = static_cast<uint32_t>(fmod(lhs, 4294967296.0));
        return AnyValue::make_number(static_cast<double>(l >> (Operators_Private::ToInt32(rhs) & 0x1F)));
    }
    inline AnyValue unsigned_right_shift(const double &lhs, const double &rhs)
    {
        uint32_t l = static_cast<uint32_t>(fmod(lhs, 4294967296.0));
        return AnyValue::make_number(static_cast<double>(l >> (static_cast<int32_t>(rhs) & 0x1F)));
    }

    // --- LOGICAL SHORT-CIRCUITING HELPERS ---
    inline AnyValue logical_and(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (!is_truthy(lhs))
            return lhs;
        return rhs;
    }

    inline AnyValue logical_or(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (is_truthy(lhs))
            return lhs;
        return rhs;
    }

    inline AnyValue nullish_coalesce(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (!lhs.is_null() && !lhs.is_undefined())
            return lhs;
        return rhs;
    }

    // --- LOGICAL ASSIGNMENT HELPERS ---
    inline AnyValue &logical_and_assign(AnyValue &lhs, const AnyValue &rhs)
    {
        if (is_truthy(lhs))
            lhs = rhs;
        return lhs;
    }

    inline AnyValue &logical_or_assign(AnyValue &lhs, const AnyValue &rhs)
    {
        if (!is_truthy(lhs))
            lhs = rhs;
        return lhs;
    }

    inline AnyValue &nullish_coalesce_assign(AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_null() || lhs.is_undefined())
            lhs = rhs;
        return lhs;
    }
}
