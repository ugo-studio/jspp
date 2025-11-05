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
                        std::string error_str = "Error";

                        if (error->properties.count("name") > 0) {
                            auto &name = error->properties["name"];
                            if (std::holds_alternative<DataDescriptor>(name)) {
                                auto &data_desc = std::get<DataDescriptor>(name);
                                if (data_desc.value.type() == typeid(std::string)) {
                                    error_str = std::any_cast<std::string>(data_desc.value);
                                } else if (data_desc.value.type() == typeid(const char *)) {
                                    error_str = std::any_cast<const char *>(data_desc.value);
                                } else if (data_desc.value.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                    error_str = std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(data_desc.value)->value);
                                }
                            } else if (std::holds_alternative<AccessorDescriptor>(name)) {
                                auto &data_desc = std::get<AccessorDescriptor>(name);
                                if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.get)){
                                    auto val = std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.get)({});
                                    if (val.type() == typeid(std::string)) {
                                        error_str = std::any_cast<std::string>(val);
                                    } else if (val.type() == typeid(const char *)) {
                                        error_str = std::any_cast<const char *>(val);
                                    } else if (val.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                        error_str = std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(val)->value);
                                    }
                                }
                            } else if (std::holds_alternative<AnyValue>(name)) {
                                auto &val = std::get<AnyValue>(name);
                                if (val.type() == typeid(std::string)) {
                                    error_str = std::any_cast<std::string>(val);
                                } else if (val.type() == typeid(const char *)) {
                                    error_str = std::any_cast<const char *>(val);
                                } else if (val.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                    error_str = std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(val)->value);
                                }

                            }
                        }

                        error_str += ": ";

                        if (error->properties.count("message") > 0) {
                            auto &message = error->properties["message"];
                            if (std::holds_alternative<DataDescriptor>(message)) {
                                auto &data_desc = std::get<DataDescriptor>(message);
                                if (data_desc.value.type() == typeid(std::string)) {
                                    error_str += std::any_cast<std::string>(data_desc.value);
                                } else if (data_desc.value.type() == typeid(const char *)) {
                                    error_str += std::any_cast<const char *>(data_desc.value);
                                } else if (data_desc.value.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                    error_str += std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(data_desc.value)->value);
                                }
                            } else if (std::holds_alternative<AccessorDescriptor>(message)) {
                                auto &data_desc = std::get<AccessorDescriptor>(message);
                                if (std::holds_alternative<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.get)){
                                    auto val = std::get<std::function<AnyValue(const std::vector<AnyValue> &)>>(data_desc.get)({});
                                    if (val.type() == typeid(std::string)) {
                                        error_str += std::any_cast<std::string>(val);
                                    } else if (val.type() == typeid(const char *)) {
                                        error_str += std::any_cast<const char *>(val);
                                    } else if (val.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                        error_str += std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(val)->value);
                                    }
                                }
                            } else if (std::holds_alternative<AnyValue>(message)) {
                                auto &val = std::get<AnyValue>(message);
                                if (val.type() == typeid(std::string)) {
                                    error_str += std::any_cast<std::string>(val);
                                } else if (val.type() == typeid(const char *)) {
                                    error_str += std::any_cast<const char *>(val);
                                } else if (val.type() == typeid(std::shared_ptr<jspp::JsString>)) {
                                    error_str += std::any_cast<std::string>(std::any_cast<std::shared_ptr<jspp::JsString>>(val)->value);
                                }

                            }
                        }

                        return jspp::Object::make_string(error_str); })};
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
                auto ex_ptr = std::any_cast<std::shared_ptr<std::exception>>(val);
                if (ex_ptr)
                {
                    return Exception::make_error_with_name(std::string(ex_ptr->what()), "Error");
                }
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
