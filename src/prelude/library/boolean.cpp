#include "jspp.hpp"
#include "library/boolean.hpp"

namespace jspp
{
    // TODO: implement boolean constructor
    jspp::AnyValue Boolean = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                     {
                                                                         if (args.empty())
                                                                             return jspp::Constants::FALSE;
                                                                         return jspp::AnyValue::make_boolean(jspp::is_truthy(args[0])); }),
        "Boolean");

    struct BooleanInit
    {
        BooleanInit()
        { // TODO: implement boolean prototypes
          // auto proto = Boolean.get_own_property("prototype");
          // proto.define_data_property("valueOf", jspp::BooleanPrototypes::get("valueOf").value(), true, false, true);
          // proto.define_data_property("toString", jspp::BooleanPrototypes::get("toString").value(), true, false, true);
        }
    };

    void init_boolean()
    {
        static BooleanInit booleanInit;
    }

}