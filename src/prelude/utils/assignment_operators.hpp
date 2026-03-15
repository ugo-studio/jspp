#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"

namespace jspp
{

    // --- FRIEND IMPLEMENTATIONS ---

    inline AnyValue &operator+=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::add(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator-=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::sub(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator*=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::mul(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator/=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::div(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator%=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::mod(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator&=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::bitwise_and(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator|=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::bitwise_or(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator^=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::bitwise_xor(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator<<=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::left_shift(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator>>=(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::right_shift(lhs, rhs);
        return lhs;
    }

    inline AnyValue &unsigned_right_shift_assign(AnyValue &lhs, const AnyValue &rhs)
    {
        lhs = jspp::unsigned_right_shift(lhs, rhs);
        return lhs;
    }

    inline AnyValue &operator++(AnyValue &val)
    {
        val = jspp::add(val, 1.0);
        return val;
    }

    inline AnyValue operator++(AnyValue &val, int)
    {
        AnyValue temp = val;
        val = jspp::add(val, 1.0);
        return temp;
    }

    inline AnyValue &operator--(AnyValue &val)
    {
        val = jspp::sub(val, 1.0);
        return val;
    }

    inline AnyValue operator--(AnyValue &val, int)
    {
        AnyValue temp = val;
        val = jspp::sub(val, 1.0);
        return temp;
    }

    // --- OVERLOADS FOR PRIMITIVES ---

    inline AnyValue &operator+=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::add(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator+=(AnyValue &lhs, const int &rhs)
    {
        return lhs += static_cast<double>(rhs);
    }

    inline AnyValue &operator-=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::sub(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator-=(AnyValue &lhs, const int &rhs)
    {
        return lhs -= static_cast<double>(rhs);
    }

    inline AnyValue &operator*=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::mul(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator*=(AnyValue &lhs, const int &rhs)
    {
        return lhs *= static_cast<double>(rhs);
    }

    inline AnyValue &operator/=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::div(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator/=(AnyValue &lhs, const int &rhs)
    {
        return lhs /= static_cast<double>(rhs);
    }

    inline AnyValue &operator%=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::mod(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator%=(AnyValue &lhs, const int &rhs)
    {
        return lhs %= static_cast<double>(rhs);
    }

    inline AnyValue &operator&=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::bitwise_and(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator&=(AnyValue &lhs, const int &rhs)
    {
        return lhs &= static_cast<double>(rhs);
    }

    inline AnyValue &operator|=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::bitwise_or(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator|=(AnyValue &lhs, const int &rhs)
    {
        return lhs |= static_cast<double>(rhs);
    }

    inline AnyValue &operator^=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::bitwise_xor(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator^=(AnyValue &lhs, const int &rhs)
    {
        return lhs ^= static_cast<double>(rhs);
    }

    inline AnyValue &operator<<=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::left_shift(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator<<=(AnyValue &lhs, const int &rhs)
    {
        return lhs <<= static_cast<double>(rhs);
    }

    inline AnyValue &operator>>=(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::right_shift(lhs, rhs);
        return lhs;
    }
    inline AnyValue &operator>>=(AnyValue &lhs, const int &rhs)
    {
        return lhs >>= static_cast<double>(rhs);
    }

    inline AnyValue &unsigned_right_shift_assign(AnyValue &lhs, const double &rhs)
    {
        lhs = jspp::unsigned_right_shift(lhs, rhs);
        return lhs;
    }
    inline AnyValue &unsigned_right_shift_assign(AnyValue &lhs, const int &rhs)
    {
        return unsigned_right_shift_assign(lhs, static_cast<double>(rhs));
    }
}
