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

    // --- PRIMITIVE ARITHMETIC OPERATORS ---
    inline double add_primitive(const double &lhs, const double &rhs) { return lhs + rhs; }
    inline double add_primitive(const AnyValue &lhs, const AnyValue &rhs) { return Operators_Private::ToNumber(lhs) + Operators_Private::ToNumber(rhs); }
    inline double add_primitive(const AnyValue &lhs, const double &rhs) { return Operators_Private::ToNumber(lhs) + rhs; }
    inline double add_primitive(const double &lhs, const AnyValue &rhs) { return lhs + Operators_Private::ToNumber(rhs); }

    inline double sub_primitive(const double &lhs, const double &rhs) { return lhs - rhs; }
    inline double sub_primitive(const AnyValue &lhs, const AnyValue &rhs) { return Operators_Private::ToNumber(lhs) - Operators_Private::ToNumber(rhs); }
    inline double sub_primitive(const AnyValue &lhs, const double &rhs) { return Operators_Private::ToNumber(lhs) - rhs; }
    inline double sub_primitive(const double &lhs, const AnyValue &rhs) { return lhs - Operators_Private::ToNumber(rhs); }

    inline double mul_primitive(const double &lhs, const double &rhs) { return lhs * rhs; }
    inline double mul_primitive(const AnyValue &lhs, const AnyValue &rhs) { return Operators_Private::ToNumber(lhs) * Operators_Private::ToNumber(rhs); }
    inline double mul_primitive(const AnyValue &lhs, const double &rhs) { return Operators_Private::ToNumber(lhs) * rhs; }
    inline double mul_primitive(const double &lhs, const AnyValue &rhs) { return lhs * Operators_Private::ToNumber(rhs); }

    inline double div_primitive(const double &lhs, const double &rhs) { return lhs / rhs; }
    inline double div_primitive(const AnyValue &lhs, const AnyValue &rhs) { return Operators_Private::ToNumber(lhs) / Operators_Private::ToNumber(rhs); }
    inline double div_primitive(const AnyValue &lhs, const double &rhs) { return Operators_Private::ToNumber(lhs) / rhs; }
    inline double div_primitive(const double &lhs, const AnyValue &rhs) { return lhs / Operators_Private::ToNumber(rhs); }

    inline double mod_primitive(const double &lhs, const double &rhs) { return std::fmod(lhs, rhs); }
    inline double mod_primitive(const AnyValue &lhs, const AnyValue &rhs) { return std::fmod(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline double mod_primitive(const AnyValue &lhs, const double &rhs) { return std::fmod(Operators_Private::ToNumber(lhs), rhs); }
    inline double mod_primitive(const double &lhs, const AnyValue &rhs) { return std::fmod(lhs, Operators_Private::ToNumber(rhs)); }

    inline double pow_primitive(const double &lhs, const double &rhs) { return std::pow(lhs, rhs); }
    inline double pow_primitive(const AnyValue &lhs, const AnyValue &rhs) { return std::pow(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline double pow_primitive(const AnyValue &lhs, const double &rhs) { return std::pow(Operators_Private::ToNumber(lhs), rhs); }
    inline double pow_primitive(const double &lhs, const AnyValue &rhs) { return std::pow(lhs, Operators_Private::ToNumber(rhs)); }

    // --- PRIMITIVE COMPARISON OPERATORS ---
    inline bool less_than_primitive(const double &lhs, const double &rhs) { return lhs < rhs; }
    inline bool less_than_primitive(const AnyValue &lhs, const AnyValue &rhs) { return less_than_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool less_than_primitive(const AnyValue &lhs, const double &rhs) { return less_than_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool less_than_primitive(const double &lhs, const AnyValue &rhs) { return less_than_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    inline bool greater_than_primitive(const double &lhs, const double &rhs) { return lhs > rhs; }
    inline bool greater_than_primitive(const AnyValue &lhs, const AnyValue &rhs) { return greater_than_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool greater_than_primitive(const AnyValue &lhs, const double &rhs) { return greater_than_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool greater_than_primitive(const double &lhs, const AnyValue &rhs) { return greater_than_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    inline bool less_than_or_equal_primitive(const double &lhs, const double &rhs) { return lhs <= rhs; }
    inline bool less_than_or_equal_primitive(const AnyValue &lhs, const AnyValue &rhs) { return less_than_or_equal_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool less_than_or_equal_primitive(const AnyValue &lhs, const double &rhs) { return less_than_or_equal_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool less_than_or_equal_primitive(const double &lhs, const AnyValue &rhs) { return less_than_or_equal_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    inline bool greater_than_or_equal_primitive(const double &lhs, const double &rhs) { return lhs >= rhs; }
    inline bool greater_than_or_equal_primitive(const AnyValue &lhs, const AnyValue &rhs) { return greater_than_or_equal_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool greater_than_or_equal_primitive(const AnyValue &lhs, const double &rhs) { return greater_than_or_equal_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool greater_than_or_equal_primitive(const double &lhs, const AnyValue &rhs) { return greater_than_or_equal_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    inline bool equal_primitive(const double &lhs, const double &rhs) { return lhs == rhs; }
    inline bool equal_primitive(const AnyValue &lhs, const AnyValue &rhs) { return equal_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool equal_primitive(const AnyValue &lhs, const double &rhs) { return equal_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool equal_primitive(const double &lhs, const AnyValue &rhs) { return equal_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    inline bool not_equal_primitive(const double &lhs, const double &rhs) { return lhs != rhs; }
    inline bool not_equal_primitive(const AnyValue &lhs, const AnyValue &rhs) { return not_equal_primitive(Operators_Private::ToNumber(lhs), Operators_Private::ToNumber(rhs)); }
    inline bool not_equal_primitive(const AnyValue &lhs, const double &rhs) { return not_equal_primitive(Operators_Private::ToNumber(lhs), rhs); }
    inline bool not_equal_primitive(const double &lhs, const AnyValue &rhs) { return not_equal_primitive(lhs, Operators_Private::ToNumber(rhs)); }

    // --- PRIMITIVE BITWISE OPERATORS ---
    inline double bitwise_and_primitive(const double &lhs, const double &rhs)
    {
        return static_cast<double>(static_cast<int32_t>(lhs) & static_cast<int32_t>(rhs));
    }
    inline double bitwise_and_primitive(const AnyValue &lhs, const AnyValue &rhs) { return bitwise_and_primitive(Operators_Private::ToInt32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double bitwise_and_primitive(const AnyValue &lhs, const double &rhs) { return bitwise_and_primitive(Operators_Private::ToInt32(lhs), rhs); }
    inline double bitwise_and_primitive(const double &lhs, const AnyValue &rhs) { return bitwise_and_primitive(lhs, Operators_Private::ToInt32(rhs)); }

    inline double bitwise_or_primitive(const double &lhs, const double &rhs)
    {
        return static_cast<double>(static_cast<int32_t>(lhs) | static_cast<int32_t>(rhs));
    }
    inline double bitwise_or_primitive(const AnyValue &lhs, const AnyValue &rhs) { return bitwise_or_primitive(Operators_Private::ToInt32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double bitwise_or_primitive(const AnyValue &lhs, const double &rhs) { return bitwise_or_primitive(Operators_Private::ToInt32(lhs), rhs); }
    inline double bitwise_or_primitive(const double &lhs, const AnyValue &rhs) { return bitwise_or_primitive(lhs, Operators_Private::ToInt32(rhs)); }

    inline double bitwise_xor_primitive(const double &lhs, const double &rhs)
    {
        return static_cast<double>(static_cast<int32_t>(lhs) ^ static_cast<int32_t>(rhs));
    }
    inline double bitwise_xor_primitive(const AnyValue &lhs, const AnyValue &rhs) { return bitwise_xor_primitive(Operators_Private::ToInt32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double bitwise_xor_primitive(const AnyValue &lhs, const double &rhs) { return bitwise_xor_primitive(Operators_Private::ToInt32(lhs), rhs); }
    inline double bitwise_xor_primitive(const double &lhs, const AnyValue &rhs) { return bitwise_xor_primitive(lhs, Operators_Private::ToInt32(rhs)); }

    inline double left_shift_primitive(const double &lhs, const double &rhs)
    {
        return static_cast<double>(static_cast<int32_t>(lhs) << (static_cast<int32_t>(rhs) & 0x1F));
    }
    inline double left_shift_primitive(const AnyValue &lhs, const AnyValue &rhs) { return left_shift_primitive(Operators_Private::ToInt32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double left_shift_primitive(const AnyValue &lhs, const double &rhs) { return left_shift_primitive(Operators_Private::ToInt32(lhs), rhs); }
    inline double left_shift_primitive(const double &lhs, const AnyValue &rhs) { return left_shift_primitive(lhs, Operators_Private::ToInt32(rhs)); }

    inline double right_shift_primitive(const double &lhs, const double &rhs)
    {
        return static_cast<double>(static_cast<int32_t>(lhs) >> (static_cast<int32_t>(rhs) & 0x1F));
    }
    inline double right_shift_primitive(const AnyValue &lhs, const AnyValue &rhs) { return right_shift_primitive(Operators_Private::ToInt32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double right_shift_primitive(const AnyValue &lhs, const double &rhs) { return right_shift_primitive(Operators_Private::ToInt32(lhs), rhs); }
    inline double right_shift_primitive(const double &lhs, const AnyValue &rhs) { return right_shift_primitive(lhs, Operators_Private::ToInt32(rhs)); }

    inline double unsigned_right_shift_primitive(const double &lhs, const double &rhs)
    {
        uint32_t l = static_cast<uint32_t>(fmod(lhs, 4294967296.0));
        return static_cast<double>(l >> (static_cast<int32_t>(rhs) & 0x1F));
    }
    inline double unsigned_right_shift_primitive(const AnyValue &lhs, const AnyValue &rhs) { return unsigned_right_shift_primitive(Operators_Private::ToUint32(lhs), Operators_Private::ToInt32(rhs)); }
    inline double unsigned_right_shift_primitive(const AnyValue &lhs, const double &rhs) { return unsigned_right_shift_primitive(Operators_Private::ToUint32(lhs), rhs); }
    inline double unsigned_right_shift_primitive(const double &lhs, const AnyValue &rhs) { return unsigned_right_shift_primitive(lhs, Operators_Private::ToInt32(rhs)); }

}