#pragma once

#include "types.hpp"
#include "any_value.hpp"

namespace jspp {
    extern AnyValue process;
    void setup_process_argv(int argc, char** argv);
}

using jspp::process;
