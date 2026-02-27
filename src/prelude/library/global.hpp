#pragma once

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"

#include "library/promise.hpp"
#include "library/timer.hpp"
#include "library/math.hpp"

// Define other function constructors by getting their prototypes from literal instances
inline auto GeneratorFunction = jspp::AnyValue::make_generator([](jspp::AnyValue, std::vector<jspp::AnyValue>) -> jspp::JsIterator<jspp::AnyValue>
                                                                { co_return jspp::Constants::UNDEFINED; })
                                    .get_own_property("prototype")
                                    .get_own_property("constructor");
inline auto AsyncFunction = jspp::AnyValue::make_async_function([](jspp::AnyValue, std::vector<jspp::AnyValue>) -> jspp::JsPromise
                                                               { co_return jspp::Constants::UNDEFINED; })
                                .get_own_property("prototype")
                                .get_own_property("constructor");
inline auto AsyncGeneratorFunction = jspp::AnyValue::make_async_generator([](jspp::AnyValue, std::vector<jspp::AnyValue>) -> jspp::JsAsyncIterator<jspp::AnyValue>
                                                                         { co_return jspp::Constants::UNDEFINED; })
                                         .get_own_property("prototype")
                                         .get_own_property("constructor");

inline auto global = jspp::AnyValue::make_object({
    {"Symbol", Symbol},
    {"process", process},
    {"Function", Function},
    {"GeneratorFunction", GeneratorFunction},
    {"AsyncFunction", AsyncFunction},
    {"AsyncGeneratorFunction", AsyncGeneratorFunction},
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

struct GlobalInit
{
    GlobalInit()
    {
        auto objectProto = ::Object.get_own_property("prototype");
        auto functionProto = ::Function.get_own_property("prototype");
        auto arrayProto = ::Array.get_own_property("prototype");

        auto toStringTagSym = jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::toStringTag);

        // Set identification tags
        objectProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("Object"), true, false, true);
        functionProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("Function"), true, false, true);
        arrayProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("Array"), true, false, true);

        // Tie built-in prototypes to Object.prototype
        arrayProto.set_prototype(objectProto);
        functionProto.set_prototype(objectProto);
        ::Error.get_own_property("prototype").set_prototype(objectProto);
        ::Promise.get_own_property("prototype").set_prototype(objectProto);
        ::Symbol.get_own_property("prototype").set_prototype(objectProto);

        // Set up function hierarchy
        auto generatorFunctionProto = ::GeneratorFunction.get_own_property("prototype");
        generatorFunctionProto.set_prototype(functionProto);
        generatorFunctionProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("GeneratorFunction"), true, false, true);

        auto asyncFunctionProto = ::AsyncFunction.get_own_property("prototype");
        asyncFunctionProto.set_prototype(functionProto);
        asyncFunctionProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("AsyncFunction"), true, false, true);

        auto asyncGeneratorFunctionProto = ::AsyncGeneratorFunction.get_own_property("prototype");
        asyncGeneratorFunctionProto.set_prototype(functionProto);
        asyncGeneratorFunctionProto.define_data_property(toStringTagSym, jspp::AnyValue::make_string("AsyncGeneratorFunction"), true, false, true);

        // Also set internal [[Prototype]] of constructors to Function.prototype
        ::Object.set_prototype(functionProto);
        ::Array.set_prototype(functionProto);
        ::Function.set_prototype(functionProto);
        ::GeneratorFunction.set_prototype(functionProto);
        ::AsyncFunction.set_prototype(functionProto);
        ::AsyncGeneratorFunction.set_prototype(functionProto);
        ::Error.set_prototype(functionProto);
        ::Promise.set_prototype(functionProto);
        ::Symbol.set_prototype(functionProto);
    }
} globalInit;
