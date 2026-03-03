#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/access.hpp"
#include "exception.hpp"

namespace jspp
{
    extern AnyValue Object;
    void init_object();
}

using jspp::Object;
