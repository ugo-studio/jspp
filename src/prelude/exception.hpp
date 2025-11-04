#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"

namespace jspp
{
    namespace Exception
    {
        inline jspp::AnyValue make_error_with_name(const AnyValue &message, const AnyValue &name)
        {
            auto error = std::make_shared<JsObject>(JsObject{{{"message", message}, {"name", name}}});
            // Define and set prototype methods
            error->prototype[WellKnownSymbols::toString] = DataDescriptor{
                jspp::Object::make_function([=](const std::vector<AnyValue> &) -> jspp::AnyValue
                                            {
                        std::string name_str = "Error";
                        if (error->properties.count("name") > 0) {
                            if (error->properties["name"].type() == typeid(std::string)) {
                                name_str = std::any_cast<std::string>(error->properties["name"]);
                            } else if (error->properties["name"].type() == typeid(const char *)) {
                                name_str = std::any_cast<const char *>(error->properties["name"]);
                            } else if (error->properties["name"].type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                name_str = std::any_cast<std::shared_ptr<jspp::JsString>>(error->properties["name"])->value;
                            }
                        }
                        std::string message_str = "";
                        if (error->properties.count("message") > 0) {
                            if (error->properties["message"].type() == typeid(std::string)) {
                                message_str = std::any_cast<std::string>(error->properties["message"]);
                            } else if (error->properties["message"].type() == typeid(const char *)) {
                                message_str = std::any_cast<const char *>(error->properties["message"]);
                            } else if (error->properties["message"].type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                message_str = std::any_cast<std::shared_ptr<jspp::JsString>>(error->properties["message"])->value;
                            }
                        }
                        std::string stack_str = "";
                        if (error->properties.count("stack") > 0) {
                            if (error->properties["stack"].type() == typeid(std::string)) {
                                stack_str = std::any_cast<std::string>(error->properties["stack"]);
                            } else if (error->properties["stack"].type() == typeid(const char *)) {
                                stack_str = std::any_cast<const char *>(error->properties["stack"]);
                            } else if (error->properties["stack"].type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                stack_str = std::any_cast<std::shared_ptr<jspp::JsString>>(error->properties["stack"])->value;
                            }
                        }
                        if (stack_str.empty()) {
                            return jspp::Object::make_string(name_str + ": " + message_str);
                        }
                        return jspp::Object::make_string(name_str + ": " + message_str + "\n    at " + stack_str); })};
            // return object
            return jspp::AnyValue(error);
        }
        inline jspp::AnyValue make_default_error(const AnyValue &message)
        {
            return Exception::make_error_with_name(message, "Error");
        }
        inline jspp::AnyValue parse_error_from_value(const AnyValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<std::exception>))
            {
                return Exception::make_error_with_name(std::string(std::any_cast<std::exception>(val).what()), "Error");
            }
            return val;
        }
        inline jspp::AnyValue throw_unresolved_reference(const std::string &varName)
        {
            throw Exception::make_error_with_name(varName + " is not defined", "ReferenceError");
        }
        inline jspp::AnyValue throw_immutable_assignment()
        {
            throw Exception::make_error_with_name("Assignment to constant variable.", "TypeError");
        }
        inline jspp::AnyValue throw_invalid_return_statement()
        {
            throw Exception::make_error_with_name("Return statements are only valid inside functions.", "SyntaxError");
        }
    };
}
