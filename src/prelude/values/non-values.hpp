#pragma once

#include "types.hpp"
// #include "exception.hpp"

namespace jspp
{

    struct JsUndefined
    {
        std::string to_std_string() const
        {
            return "undefined";
        }

        // AnyValue &operator[](const AnyValue &key)
        // {
        //     return AnyValue{NonValues::undefined};
        // }
    };

    struct JsNull
    {
        std::string to_std_string() const
        {
            return "null";
        }

        // AnyValue &operator[](const AnyValue &key)
        // {
        //     return AnyValue{NonValues::undefined};
        // }
    };

    struct JsUninitialized
    {
        std::string to_std_string() const
        {
            return "<uninitialized>";
        }

        // AnyValue &operator[](const AnyValue &_)
        // {
        //     // return Exception::throw_uninitialized_read_property_error();
        //     return AnyValue{NonValues::undefined};
        // }
    };

    namespace NonValues
    {
        inline constexpr JsUndefined undefined;
        inline constexpr JsNull null;
        inline constexpr JsUninitialized uninitialized;
    }

}
