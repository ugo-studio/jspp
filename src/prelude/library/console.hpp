#pragma once

#include <chrono>
#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"
#include "exception.hpp"
#include "utils/log_any_value/log_any_value.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

namespace jspp {
    extern AnyValue logFn;
    extern AnyValue warnFn;
    extern AnyValue errorFn;
    extern AnyValue timeFn;
    extern AnyValue timeEndFn;
    extern AnyValue console; 
    void init_console();
}
