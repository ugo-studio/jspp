#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp {
    extern AnyValue Function;
    void init_function_lib();
}

using jspp::Function;
