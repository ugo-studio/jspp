#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "operators_primitive.hpp"
#include <cstdint>   // For int32_t
#include <cmath>     // For fmod, isnan, isinf, floor, abs, pow
#include <string>    // For std::to_string, std::stod
#include <algorithm> // For std::all_of
#include <limits>    // For numeric_limits

namespace jspp
{
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

    // Function add
    inline AnyValue add(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_number() && rhs.is_number())
            return AnyValue::make_number(add_primitive(lhs.as_double(), rhs.as_double()));
        if (lhs.is_string() || rhs.is_string())
            return AnyValue::make_string(lhs.to_std_string() + rhs.to_std_string());
        return AnyValue::make_number(add_primitive(lhs, rhs));
    }
    inline AnyValue add(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_number(add_primitive(lhs.as_double(), rhs));
        if (lhs.is_string())
            return AnyValue::make_string(lhs.to_std_string() + std::to_string(rhs));
        return AnyValue::make_number(add_primitive(lhs, rhs));
    }
    inline AnyValue add(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_number(add_primitive(lhs, rhs.as_double()));
        if (rhs.is_string())
            return AnyValue::make_string(std::to_string(lhs) + rhs.to_std_string());
        return AnyValue::make_number(add_primitive(lhs, rhs));
    }
    inline AnyValue add(const double &lhs, const double &rhs)
    {
        return AnyValue::make_number(add_primitive(lhs, rhs));
    }

    // Function sub
    inline AnyValue sub(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(sub_primitive(lhs, rhs)); }
    inline AnyValue sub(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(sub_primitive(lhs, rhs)); }
    inline AnyValue sub(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(sub_primitive(lhs, rhs)); }
    inline AnyValue sub(const double &lhs, const double &rhs) { return AnyValue::make_number(sub_primitive(lhs, rhs)); }

    // Function mul
    inline AnyValue mul(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(mul_primitive(lhs, rhs)); }
    inline AnyValue mul(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(mul_primitive(lhs, rhs)); }
    inline AnyValue mul(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(mul_primitive(lhs, rhs)); }
    inline AnyValue mul(const double &lhs, const double &rhs) { return AnyValue::make_number(mul_primitive(lhs, rhs)); }

    // Function div
    inline AnyValue div(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(div_primitive(lhs, rhs)); }
    inline AnyValue div(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(div_primitive(lhs, rhs)); }
    inline AnyValue div(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(div_primitive(lhs, rhs)); }
    inline AnyValue div(const double &lhs, const double &rhs) { return AnyValue::make_number(div_primitive(lhs, rhs)); }

    // Function mod
    inline AnyValue mod(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(mod_primitive(lhs, rhs)); }
    inline AnyValue mod(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(mod_primitive(lhs, rhs)); }
    inline AnyValue mod(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(mod_primitive(lhs, rhs)); }
    inline AnyValue mod(const double &lhs, const double &rhs) { return AnyValue::make_number(mod_primitive(lhs, rhs)); }

    // --- UNARY OPERATORS ---
    inline AnyValue plus(const AnyValue &val)
    {
        return AnyValue::make_number(Operators_Private::ToNumber(val));
    }
    inline AnyValue negate(const AnyValue &val)
    {
        return AnyValue::make_number(-Operators_Private::ToNumber(val));
    }
    inline AnyValue bitwise_not(const AnyValue &val)
    {
        return AnyValue::make_number(~Operators_Private::ToInt32(val));
    }
    inline AnyValue logical_not(const AnyValue &val)
    {
        return AnyValue::make_boolean(!is_truthy(val));
    }

    // --- EXPONENTIATION ---
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(pow_primitive(lhs, rhs)); }
    inline AnyValue pow(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(pow_primitive(lhs, rhs)); }
    inline AnyValue pow(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(pow_primitive(lhs, rhs)); }
    inline AnyValue pow(const double &lhs, const double &rhs) { return AnyValue::make_number(pow_primitive(lhs, rhs)); }

    // --- COMPARISON OPERATORS ---

    // Less than <
    inline AnyValue less_than(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_string() && rhs.is_string())
            return AnyValue::make_boolean(lhs.as_string()->value < rhs.as_string()->value);

        return AnyValue::make_boolean(less_than_primitive(lhs, rhs));
    }
    inline AnyValue less_than(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(less_than_primitive(lhs, rhs));
    }
    inline AnyValue less_than(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_boolean(less_than_primitive(lhs, rhs));
    }
    inline AnyValue less_than(const double &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(less_than_primitive(lhs, rhs));
    }

    inline AnyValue greater_than(const AnyValue &lhs, const AnyValue &rhs) { return less_than(rhs, lhs); }
    inline AnyValue greater_than(const AnyValue &lhs, const double &rhs) { return less_than(rhs, lhs); }
    inline AnyValue greater_than(const double &lhs, const AnyValue &rhs) { return less_than(rhs, lhs); }
    inline AnyValue greater_than(const double &lhs, const double &rhs) { return AnyValue::make_boolean(greater_than_primitive(lhs, rhs)); }

    inline AnyValue less_than_or_equal(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_string() && rhs.is_string())
             return AnyValue::make_boolean(lhs.as_string()->value <= rhs.as_string()->value);
        return AnyValue::make_boolean(less_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue less_than_or_equal(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(less_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue less_than_or_equal(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_boolean(less_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue less_than_or_equal(const double &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(less_than_or_equal_primitive(lhs, rhs));
    }

    inline AnyValue greater_than_or_equal(const AnyValue &lhs, const AnyValue &rhs)
    {
        if (lhs.is_string() && rhs.is_string())
             return AnyValue::make_boolean(lhs.as_string()->value >= rhs.as_string()->value);
        return AnyValue::make_boolean(greater_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue greater_than_or_equal(const AnyValue &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(greater_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue greater_than_or_equal(const double &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_boolean(greater_than_or_equal_primitive(lhs, rhs));
    }
    inline AnyValue greater_than_or_equal(const double &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(greater_than_or_equal_primitive(lhs, rhs));
    }

    // Equality ==
    inline AnyValue equal(const AnyValue &lhs, const AnyValue &rhs)
    {
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, rhs));
    }
    inline AnyValue equal(const AnyValue &lhs, const double &rhs)
    {
        if (lhs.is_number())
            return AnyValue::make_boolean(equal_primitive(lhs.as_double(), rhs));
        return AnyValue::make_boolean(is_equal_to_primitive(lhs, AnyValue::make_number(rhs)));
    }
    inline AnyValue equal(const double &lhs, const AnyValue &rhs)
    {
        if (rhs.is_number())
            return AnyValue::make_boolean(equal_primitive(lhs, rhs.as_double()));
        return AnyValue::make_boolean(is_equal_to_primitive(rhs, AnyValue::make_number(lhs)));
    }
    inline AnyValue equal(const double &lhs, const double &rhs)
    {
        return AnyValue::make_boolean(equal_primitive(lhs, rhs));
    }

    inline AnyValue not_equal(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_boolean(!is_equal_to_primitive(lhs, rhs)); }
    inline AnyValue not_equal(const AnyValue &lhs, const double &rhs) { return AnyValue::make_boolean(!equal(lhs, rhs).as_boolean()); }
    inline AnyValue not_equal(const double &lhs, const AnyValue &rhs) { return AnyValue::make_boolean(!equal(lhs, rhs).as_boolean()); }
    inline AnyValue not_equal(const double &lhs, const double &rhs) { return AnyValue::make_boolean(not_equal_primitive(lhs, rhs)); }

    // --- BITWISE OPERATORS ---
    inline AnyValue bitwise_xor(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_xor_primitive(lhs, rhs)); }
    inline AnyValue bitwise_xor(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(bitwise_xor_primitive(lhs, rhs)); }
    inline AnyValue bitwise_xor(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_xor_primitive(lhs, rhs)); }
    inline AnyValue bitwise_xor(const double &lhs, const double &rhs) { return AnyValue::make_number(bitwise_xor_primitive(lhs, rhs)); }

    inline AnyValue bitwise_and(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_and_primitive(lhs, rhs)); }
    inline AnyValue bitwise_and(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(bitwise_and_primitive(lhs, rhs)); }
    inline AnyValue bitwise_and(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_and_primitive(lhs, rhs)); }
    inline AnyValue bitwise_and(const double &lhs, const double &rhs) { return AnyValue::make_number(bitwise_and_primitive(lhs, rhs)); }

    inline AnyValue bitwise_or(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_or_primitive(lhs, rhs)); }
    inline AnyValue bitwise_or(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(bitwise_or_primitive(lhs, rhs)); }
    inline AnyValue bitwise_or(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(bitwise_or_primitive(lhs, rhs)); }
    inline AnyValue bitwise_or(const double &lhs, const double &rhs) { return AnyValue::make_number(bitwise_or_primitive(lhs, rhs)); }

    // --- SHIFT OPERATORS ---
    inline AnyValue left_shift(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(left_shift_primitive(lhs, rhs)); }
    inline AnyValue left_shift(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(left_shift_primitive(lhs, rhs)); }
    inline AnyValue left_shift(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(left_shift_primitive(lhs, rhs)); }
    inline AnyValue left_shift(const double &lhs, const double &rhs) { return AnyValue::make_number(left_shift_primitive(lhs, rhs)); }

    inline AnyValue right_shift(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(right_shift_primitive(lhs, rhs)); }
    inline AnyValue right_shift(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(right_shift_primitive(lhs, rhs)); }
    inline AnyValue right_shift(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(right_shift_primitive(lhs, rhs)); }
    inline AnyValue right_shift(const double &lhs, const double &rhs) { return AnyValue::make_number(right_shift_primitive(lhs, rhs)); }

    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const AnyValue &rhs) { return AnyValue::make_number(unsigned_right_shift_primitive(lhs, rhs)); }
    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const double &rhs) { return AnyValue::make_number(unsigned_right_shift_primitive(lhs, rhs)); }
    inline AnyValue unsigned_right_shift(const double &lhs, const AnyValue &rhs) { return AnyValue::make_number(unsigned_right_shift_primitive(lhs, rhs)); }
    inline AnyValue unsigned_right_shift(const double &lhs, const double &rhs) { return AnyValue::make_number(unsigned_right_shift_primitive(lhs, rhs)); }

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