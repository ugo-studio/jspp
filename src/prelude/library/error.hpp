#pragma once

#include "types.hpp"
#include "any_value.hpp"

// Declare Error variable
inline jspp::AnyValue Error;

// Constructor logic
inline auto errorConstructor = [](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
{
    // Access global Error to get prototype
    jspp::AnyValue proto = Error.get_own_property("prototype");

    jspp::AnyValue target = thisVal;
    bool is_construct_call = false;

    if (target.is_object())
    {
        auto obj = target.as_object();
        if (!obj->proto.is_null() && !obj->proto.is_undefined() && is_strictly_equal_to_primitive(obj->proto, proto))
        {
            is_construct_call = true;
        }
    }

    if (!is_construct_call)
    {
        target = jspp::AnyValue::make_object_with_proto({}, proto);
    }

    std::string name = "Error";
    std::string message = "";
    if (!args.empty() && !args[0].is_undefined())
    {
        message = args[0].to_std_string();
    }

    target.define_data_property("message", jspp::AnyValue::make_string(message), true, false, true);
    target.define_data_property("name", jspp::AnyValue::make_string(name), true, false, true);
    target.define_data_property("stack", jspp::AnyValue::make_string(name + ": " + message + "\n    at <unknown>"), true, false, true);

    if (args.size() > 1 && args[1].is_object())
    {
        jspp::AnyValue cause = args[1].get_own_property("cause");
        if (!cause.is_undefined())
        {
            target.define_data_property("cause", cause, true, false, true);
        }
    }

    return target;
};

// Static Error.isError(val) implementation
inline auto isErrorFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                      {
    if (args.empty()) return jspp::Constants::FALSE;
    
    jspp::AnyValue val = args[0];
    if (!val.is_object()) return jspp::Constants::FALSE;
    
    // Check if it inherits from Error.prototype
    // This is essentially 'instanceof Error'
    jspp::AnyValue proto = Error.get_own_property("prototype");
    
    // Walk prototype chain
    if (val.is_object()) {
        auto current = val.as_object()->proto;
        while (!current.is_null()) {
             if (is_strictly_equal_to_primitive(current, proto)) return jspp::Constants::TRUE;
             if (current.is_object()) current = current.as_object()->proto;
             else break;
        }
    }
    
    return jspp::Constants::FALSE; }, "isError");

// toString method for Error.prototype
inline auto errorToStringFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                            {
    std::string name = "Error";
    std::string msg = "";
    
    jspp::AnyValue n = thisVal.get_own_property("name");
    if (!n.is_undefined()) name = n.to_std_string();
    
    jspp::AnyValue m = thisVal.get_own_property("message");
    if (!m.is_undefined()) msg = m.to_std_string();
    
    if (name.empty() && msg.empty()) return jspp::AnyValue::make_string("Error");
    if (name.empty()) return jspp::AnyValue::make_string(msg);
    if (msg.empty()) return jspp::AnyValue::make_string(name);
    
    return jspp::AnyValue::make_string(name + ": " + msg); }, "toString");

// Initialize Error class and its prototype
struct ErrorInit
{
    ErrorInit()
    {
        // Initialize Error class
        Error = jspp::AnyValue::make_class(errorConstructor, "Error");

        // Define Error.prototype.toString
        auto proto = Error.get_own_property("prototype");
        proto.define_data_property("toString", errorToStringFn, true, false, true);
        proto.define_data_property(jspp::WellKnownSymbols::toStringTag->key, errorToStringFn, true, false, true);

        // Define static Error.isError
        Error.define_data_property("isError", isErrorFn, true, false, true);
    }
} errorInit;