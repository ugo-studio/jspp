#pragma once

#include "types.hpp"
#include "any_value.hpp"

// Define Function constructor
// In a full implementation, this would support 'new Function(args, body)'
inline auto Function = jspp::AnyValue::make_class([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                  { return jspp::Constants::UNDEFINED; }, "Function");

struct FunctionInit
{
    FunctionInit()
    {
        auto proto = Function.get_own_property("prototype");

        // Function.prototype.call
        proto.define_data_property("call", jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                         {
            if (!thisVal.is_function()) throw jspp::Exception::make_exception("Function.prototype.call called on non-function", "TypeError");
            if (args.empty()) return thisVal.call(jspp::Constants::UNDEFINED, {});
            auto receiver = args[0];
            return thisVal.call(receiver, args.subspan(1)); }, "call"),
                                   true, false, true);

        // Function.prototype.apply
        proto.define_data_property("apply", jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                          {
            if (!thisVal.is_function()) throw jspp::Exception::make_exception("Function.prototype.apply called on non-function", "TypeError");
            auto receiver = args.size() > 0 ? args[0] : jspp::Constants::UNDEFINED;
            auto applyArgs = args.size() > 1 ? args[1] : jspp::Constants::UNDEFINED;
            
            if (applyArgs.is_null() || applyArgs.is_undefined()) return thisVal.call(receiver, {});
            if (!applyArgs.is_array()) throw jspp::Exception::make_exception("CreateListFromArrayLike called on non-object", "TypeError");
            
            auto arr = applyArgs.as_array();
            std::vector<jspp::AnyValue> vec;
            for(uint64_t i = 0; i < arr->length; ++i) vec.push_back(arr->get_property(static_cast<uint32_t>(i)));
            return thisVal.call(receiver, vec); }, "apply"),
                                   true, false, true);

        // Function.prototype.bind
        proto.define_data_property("bind", jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                         {
            if (!thisVal.is_function()) throw jspp::Exception::make_exception("Function.prototype.bind called on non-function", "TypeError");
            auto receiver = args.size() > 0 ? args[0] : jspp::Constants::UNDEFINED;
            std::vector<jspp::AnyValue> boundArgs(args.begin() + (args.empty() ? 0 : 1), args.end());
            
            return jspp::AnyValue::make_function([thisVal, receiver, boundArgs](jspp::AnyValue, std::span<const jspp::AnyValue> callArgs) -> jspp::AnyValue {
                std::vector<jspp::AnyValue> combinedArgs = boundArgs;
                combinedArgs.insert(combinedArgs.end(), callArgs.begin(), callArgs.end());
                return thisVal.call(receiver, combinedArgs);
            }, thisVal.as_function()->name); }, "bind"),
                                   true, false, true);

        // Function.prototype.toString
        auto toStringFn = jspp::FunctionPrototypes::get("toString");
        if (toStringFn.has_value())
        {
            proto.define_data_property("toString", toStringFn.value(), true, false, true);
        }
    }
} functionInit;
