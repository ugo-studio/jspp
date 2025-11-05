#pragma once

#include "types.hpp"
#include "object.hpp"
#include "console.hpp"

inline auto global = jspp::Object::make_object({
    {"console", console},
});
inline auto globalThis = global;
