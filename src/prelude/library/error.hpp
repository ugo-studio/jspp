#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp {
    extern AnyValue Error; 
    void init_error();
    extern AnyValue isErrorFn;
    extern AnyValue errorToStringFn;
}

using jspp::Error;
