#pragma once

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

#include "library/promise.hpp"
#include "library/timer.hpp"
#include "library/math.hpp"

namespace jspp {
    extern AnyValue GeneratorFunction;
    extern AnyValue AsyncFunction;
    extern AnyValue AsyncGeneratorFunction;
    extern AnyValue global;

    void initialize_runtime();
}

using jspp::global;
using jspp::GeneratorFunction;
using jspp::AsyncFunction;
using jspp::AsyncGeneratorFunction;
