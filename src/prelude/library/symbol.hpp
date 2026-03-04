#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp {
    extern AnyValue Symbol;
    void init_symbol();
}

using jspp::Symbol;
