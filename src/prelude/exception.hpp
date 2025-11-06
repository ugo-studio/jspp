#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"

namespace jspp
{
    // // Forward declarations
    // struct JsObject;
    // struct JsString;
    // struct JsFunction;

    namespace Exception
    {
        inline AnyValue make_error(const std::string &message, const std::string &name = "Error")
        {
            auto errorObj = std::make_shared<JsObject>(JsObject{{{"message", JsString{message}}, {"name", JsString{name}}}});
            errorObj->props[WellKnownSymbols::toString] = JsFunction{[errorObj](const std::vector<AnyValue> &) -> AnyValue
                                                                     {
                                                                         auto errorStr = JsString{"Error"};
                                                                         if (errorObj->props.count("name") > 0)
                                                                         {
                                                                             auto &name = errorObj->props["name"];
                                                                             if (std::holds_alternative<std::string>(name))
                                                                             {
                                                                                 errorStr.value = std::get<std::string>(name);
                                                                             }
                                                                         }
                                                                         errorStr.value += ": ";
                                                                         if (errorObj->props.count("message") > 0)
                                                                         {
                                                                             auto &message = errorObj->props["message"];
                                                                             if (std::holds_alternative<std::string>(message))
                                                                             {
                                                                                 errorStr.value += std::get<std::string>(message);
                                                                             }
                                                                         }
                                                                         return errorStr;
                                                                     }};
            return *errorObj;
        }

        inline AnyValue parse_error_from_value(const std::variant<AnyValue, std::exception> &ex)
        {

            if (std::holds_alternative<std::exception>(ex))
            {
                auto ex_ptr = std::get<std::exception>(ex);
                return Exception::make_error(std::string{ex_ptr.what()});
            }
            return std::get<AnyValue>(ex);
        }

        inline jspp::AnyValue throw_unresolved_reference_error(const std::string &varName)
        {
            throw Exception::make_error(varName + " is not defined", "ReferenceError");
        }
        inline jspp::AnyValue throw_uninitialized_reference_error(const std::string &varName)
        {
            throw Exception::make_error("Cannot access '" + varName + "' before initialization", "ReferenceError");
        }
        inline jspp::AnyValue throw_uninitialized_read_property_error(const std::string &varName)
        {
            throw Exception::make_error("Cannot read properties of uninitialized (reading '" + varName + "')", "TypeError");
        }
        inline jspp::AnyValue throw_uninitialized_read_property_error()
        {
            throw Exception::make_error("Cannot read properties of uninitialized", "TypeError");
        }
        inline jspp::AnyValue throw_immutable_assignment_error()
        {
            throw Exception::make_error("Assignment to constant variable.", "TypeError");
        }
        inline jspp::AnyValue throw_invalid_return_statement_error()
        {
            throw Exception::make_error("Return statements are only valid inside functions.", "SyntaxError");
        }

    }
}
