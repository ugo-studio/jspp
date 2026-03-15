#include "jspp.hpp"
#include "values/prototypes/boolean.hpp"

namespace jspp
{

    // --- JsBoolean Implementation ---

    namespace JsBoolean
    {
        std::string to_std_string(bool value)
        {
            return value ? "true" : "false";
        }

        std::string to_std_string(const AnyValue &value)
        {
            return to_std_string(value.as_boolean());
        }

    }

    // --- BooleanPrototypes Implementation ---

    namespace BooleanPrototypes
    {

        AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         { return AnyValue::make_string(JsBoolean::to_std_string(thisVal.as_boolean())); },
                                                         "toString");
            return fn;
        }

        AnyValue &get_valueOf_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_boolean(thisVal.as_boolean()); },
                                                         "valueOf");
            return fn;
        }

        std::optional<AnyValue> get(const std::string &key)
        {
            if (key == "toString")
                return get_toString_fn();
            if (key == "valueOf")
                return get_valueOf_fn();
            return std::nullopt;
        }

        std::optional<AnyValue> get(const AnyValue &key)
        {
            if (key == "toString")
                return get_toString_fn();
            if (key == "valueOf")
                return get_valueOf_fn();
            return std::nullopt;
        }

    } // namespace BooleanPrototypes

} // namespace jspp
