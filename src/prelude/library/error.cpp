#include "jspp.hpp"
#include "library/error.hpp"

namespace jspp {
    jspp::AnyValue Error;

    auto errorConstructor = [](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
    {
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
            target = jspp::AnyValue::make_object({}).set_prototype(proto);
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

    jspp::AnyValue isErrorFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
    {
        if (args.empty()) return jspp::Constants::FALSE;
        jspp::AnyValue val = args[0];
        if (!val.is_object()) return jspp::Constants::FALSE;
        jspp::AnyValue proto = Error.get_own_property("prototype");
        if (val.is_object()) {
            auto current = val.as_object()->proto;
            while (!current.is_null()) {
                 if (is_strictly_equal_to_primitive(current, proto)) return jspp::Constants::TRUE;
                 if (current.is_object()) current = current.as_object()->proto;
                 else break;
            }
        }
        return jspp::Constants::FALSE; 
    }), "isError");

    jspp::AnyValue errorToStringFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
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
        return jspp::AnyValue::make_string(name + ": " + msg); 
    }), "toString");

    struct ErrorInit
    {
        ErrorInit()
        {
            Error = jspp::AnyValue::make_class(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>(errorConstructor), "Error");
            auto proto = Error.get_own_property("prototype");
            proto.define_data_property("toString", errorToStringFn, true, false, true);
            proto.define_data_property(jspp::AnyValue::from_symbol(jspp::WellKnownSymbols::toStringTag), errorToStringFn, true, false, true);
            Error.define_data_property("isError", isErrorFn, true, false, true);
        }
    };
    static ErrorInit errorInit;
}
