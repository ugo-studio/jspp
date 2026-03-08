#pragma once

#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <bit>
#include <numbers>
#if __has_include(<stdfloat>)
#include <stdfloat>
#endif

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"
#include "utils/access.hpp"

namespace jspp {
    namespace Library {
        double GetArgAsDouble(std::span<const jspp::AnyValue> args, size_t index);
        jspp::AnyValue MathFunc1(std::span<const jspp::AnyValue> args, double (*func)(double));
        jspp::AnyValue MathFunc2(std::span<const jspp::AnyValue> args, double (*func)(double, double));
    }
}

namespace jspp {
    extern AnyValue Math; void init_math();
}
