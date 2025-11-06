#pragma once

#include "types.hpp"
// #include "exception.hpp"

namespace jspp
{

    struct JsUndefined
    {
        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };

    struct JsNull
    {
        AnyValue &operator[](const AnyValue &key)
        {
            return AnyValue{NonValues::undefined};
        }
    };

    struct JsUninitialized
    {
        AnyValue &operator[](const AnyValue &_)
        {
            // return Exception::throw_uninitialized_read_property_error();
            return AnyValue{NonValues::undefined};
        }
    };

    namespace NonValues
    {
        inline constexpr JsUndefined undefined;
        inline constexpr JsNull null;
        inline constexpr JsUninitialized uninitialized;
    }
}
