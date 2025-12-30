#pragma once

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

#include "library/promise.hpp"
#include "library/timer.hpp"
#include "library/math.hpp"

inline auto global = jspp::AnyValue::make_object({
    {"Symbol", Symbol},
    {"Function", Function},
    {"console", console},
    {"performance", performance},
    {"Error", Error},
    {"Promise", Promise},
    {"setTimeout", setTimeout},
    {"clearTimeout", clearTimeout},
    {"setInterval", setInterval},
    {"clearInterval", clearInterval},
    {"Math", Math},
    {"Object", Object},
    {"Array", Array},
});

struct GlobalInit {
    GlobalInit() {
        auto objectProto = ::Object.get_own_property("prototype");
        
        // Tie built-in prototypes to Object.prototype
        ::Array.get_own_property("prototype").set_prototype(objectProto);
        ::Function.get_own_property("prototype").set_prototype(objectProto);
        ::Error.get_own_property("prototype").set_prototype(objectProto);
        ::Promise.get_own_property("prototype").set_prototype(objectProto);
        ::Symbol.get_own_property("prototype").set_prototype(objectProto);
    }
} globalInit;
