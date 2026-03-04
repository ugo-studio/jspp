#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp {
    extern AnyValue setTimeout;
    extern AnyValue clearTimeout;
    extern AnyValue setInterval;
    extern AnyValue clearInterval;
}

using jspp::setTimeout;
using jspp::clearTimeout;
using jspp::setInterval;
using jspp::clearInterval;
