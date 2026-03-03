#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp
{
    extern AnyValue Promise;
    void init_promise();
}

using jspp::Promise;
