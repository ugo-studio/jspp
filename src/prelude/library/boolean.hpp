#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"
#include "utils/access.hpp"

namespace jspp
{
    extern AnyValue Boolean;
    void init_boolean();
}

using jspp::Boolean;
