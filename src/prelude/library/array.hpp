#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"
#include "utils/access.hpp"

namespace jspp {
    extern AnyValue Array; 
    void init_array();
}

using jspp::Array;
