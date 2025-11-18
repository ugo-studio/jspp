#pragma once

#include "types.hpp"
#include "js_value.hpp"
#include <cstdint> // For int32_t
#include <cmath>   // For fmod, isnan, isinf, floor, abs, pow

namespace jspp
{
    // Private namespace for helper functions that implement JS type conversions.
    namespace Operators_Private
    {
        // Implements the ToNumber abstract operation from ECMA-262.
        inline double ToNumber(const JsValue &val)
        {
            if (val.is_number())
                return val.as_double();
            if (val.is_null())
                return 0.0;
            if (val.is_undefined() || val.is_uninitialized())
                return std::numeric_limits<double>::quiet_NaN();
            if (val.is_boolean())
                return val.as_boolean() ? 1.0 : 0.0;
            if (val.is_string())
            {
                const std::string &s = *val.as_string();
                // JS considers empty or whitespace-only strings as 0.
                if (s.empty() || std::all_of(s.begin(), s.end(), isspace))
                    return 0.0;
                try
                {
                    size_t pos;
                    double num = std::stod(s, &pos);
                    // Ensure the entire string was parsed, allowing for trailing whitespace.
                    while (pos < s.length() && std::isspace(s[pos]))
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
            // In a full engine, objects would be converted via valueOf/toString.
            // Here we simplify and return NaN.
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Implements the ToInt32 abstract operation from ECMA-262.
        inline int32_t ToInt32(const JsValue &val)
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
    }

    // --- BASIC ARITHEMETIC
    inline JsValue operator+(const JsValue &lhs, const JsValue &rhs)
    {
        // Special case for addition: string concatenation has priority
        if (lhs.is_string() || rhs.is_string())
            return JsValue::make_string(lhs.to_std_string() + rhs.to_std_string());
        if (lhs.is_number() && rhs.is_number())
            return JsValue::make_number(lhs.as_double() + rhs.as_double());
        // Fallback to numeric conversion
        return JsValue::make_number(Operators_Private::ToNumber(lhs) + Operators_Private::ToNumber(rhs));
    }
    inline JsValue operator-(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToNumber(lhs) - Operators_Private::ToNumber(rhs));
    }
    inline JsValue operator*(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToNumber(lhs) * Operators_Private::ToNumber(rhs));
    }
    inline JsValue operator/(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToNumber(lhs) / Operators_Private::ToNumber(rhs));
    }
    inline JsValue operator%(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(std::fmod(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)));
    }

    // --- UNARY OPERATORS
    inline JsValue operator-(const JsValue &val)
    {
        return JsValue::make_number(-Operators_Private::ToNumber(val));
    }
    inline JsValue operator~(const JsValue &val)
    {
        return JsValue::make_number(~Operators_Private::ToInt32(val));
    }

    // --- EXPONENTIATION
    inline JsValue pow(const JsValue &lhs, const JsValue &rhs)
    {
        double base = Operators_Private::ToNumber(lhs);
        double exp = Operators_Private::ToNumber(rhs);
        return JsValue::make_number(std::pow(base, exp));
    }

    // --- COMPARISON OPERATORS
    inline JsValue operator<(const JsValue &lhs, const JsValue &rhs)
    {
        // Simplified Abstract Relational Comparison
        if (lhs.is_string() && rhs.is_string())
            return JsValue::make_boolean(*lhs.as_string() < *rhs.as_string());

        double l = Operators_Private::ToNumber(lhs);
        double r = Operators_Private::ToNumber(rhs);

        if (std::isnan(l) || std::isnan(r))
            return JsValue::make_boolean(false); // Comparison with NaN is false

        return JsValue::make_boolean(l < r);
    }
    inline JsValue operator>(const JsValue &lhs, const JsValue &rhs)
    {
        return rhs < lhs;
    }
    inline JsValue operator<=(const JsValue &lhs, const JsValue &rhs)
    {
        // a <= b is equivalent to !(b < a)
        JsValue result = rhs < lhs;
        return JsValue::make_boolean(!result.as_boolean());
    }
    inline JsValue operator>=(const JsValue &lhs, const JsValue &rhs)
    {
        // a >= b is equivalent to !(a < b)
        JsValue result = lhs < rhs;
        return JsValue::make_boolean(!result.as_boolean());
    }
    inline JsValue operator==(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_boolean(lhs.is_equal_to(rhs));
    }
    inline JsValue operator!=(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_boolean(!lhs.is_equal_to(rhs));
    }

    // --- BITWISE OPERATORS
    inline JsValue operator^(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToInt32(lhs) ^ Operators_Private::ToInt32(rhs));
    }
    inline JsValue operator&(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToInt32(lhs) & Operators_Private::ToInt32(rhs));
    }
    inline JsValue operator|(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToInt32(lhs) | Operators_Private::ToInt32(rhs));
    }

    // --- SHIFT OPERATORS
    inline JsValue operator<<(const JsValue &lhs, const JsValue &rhs)
    {
        // The right operand is treated as an unsigned 32-bit integer, and only the lower 5 bits are used.
        return JsValue::make_number(Operators_Private::ToInt32(lhs) << (Operators_Private::ToInt32(rhs) & 0x1F));
    }
    inline JsValue operator>>(const JsValue &lhs, const JsValue &rhs)
    {
        return JsValue::make_number(Operators_Private::ToInt32(lhs) >> (Operators_Private::ToInt32(rhs) & 0x1F));
    }

    // --- INCREMENT / DECREMENT
    inline JsValue &operator++(JsValue &val) // pre-increment
    {
        double num = Operators_Private::ToNumber(val);
        val = JsValue::make_number(num + 1.0);
        return val;
    }
    inline JsValue operator++(JsValue &val, int) // post-increment
    {
        JsValue old = JsValue::make_number(Operators_Private::ToNumber(val));
        ++val;
        return old;
    }
    inline JsValue &operator--(JsValue &val) // pre-decrement
    {
        double num = Operators_Private::ToNumber(val);
        val = JsValue::make_number(num - 1.0);
        return val;
    }
    inline JsValue operator--(JsValue &val, int) // post-decrement
    {
        JsValue old = JsValue::make_number(Operators_Private::ToNumber(val));
        --val;
        return old;
    }

    // --- COMPOUND ASSIGNMENT
    inline JsValue &operator+=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }
    inline JsValue &operator-=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }
    inline JsValue &operator*=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }
    inline JsValue &operator/=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }
    inline JsValue &operator%=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs % rhs;
        return lhs;
    }
    inline JsValue &operator^=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }
    inline JsValue &operator&=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }
    inline JsValue &operator|=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }
    inline JsValue &operator<<=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs << rhs;
        return lhs;
    }
    inline JsValue &operator>>=(JsValue &lhs, const JsValue &rhs)
    {
        lhs = lhs >> rhs;
        return lhs;
    }
}