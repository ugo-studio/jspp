#pragma once

#include "types.hpp"
#include "error.hpp"
#include "any_value.hpp"

const char *jspp::RuntimeError::what() const noexcept
{
    return data->to_std_string().c_str();
}

jspp::RuntimeError jspp::RuntimeError::make_error(const std::string &message, const std::string &name = "Error")
{
    auto errorObj = std::make_shared<AnyValue>(AnyValue::make_object({{"message", AnyValue::make_string(message)}, {"name", AnyValue::make_string(name)}}));
    (*errorObj).set_own_property("toString", AnyValue::make_function([errorObj](const std::vector<AnyValue> &) -> AnyValue
                                                                     {
                                                                                  AnyValue name = (*errorObj).get_own_property("name");
                                                                                  AnyValue message = (*errorObj).get_own_property("message");
                                                                                  std::string str = "";
                                                                                  if (name.is_string())
                                                                                      str = name.to_std_string();
                                                                                  else
                                                                                      str = "Error";
                                                                                  str += ": ";
                                                                                  if (message.is_string())
                                                                                      str += message.to_std_string();
                                                                                  return AnyValue::make_string(str); },
                                                                     "toString"));
    return RuntimeError(errorObj);
}
jspp::AnyValue jspp::RuntimeError::error_to_value(const std::exception &ex)
{
    if (const jspp::RuntimeError *err = dynamic_cast<const jspp::RuntimeError *>(&ex))
    {
        return (*err->data);
    }
    else
    {
        return AnyValue::make_string(ex.what());
    }
}

// --- THROWERS
jspp::AnyValue jspp::RuntimeError::throw_unresolved_reference_error(const std::string &var_name)
{
    throw RuntimeError::make_error(var_name + " is not defined", "ReferenceError");
}
jspp::AnyValue jspp::RuntimeError::throw_uninitialized_reference_error(const std::string &var_name)
{
    throw RuntimeError::make_error("Cannot access '" + var_name + "' before initialization", "ReferenceError");
}
jspp::AnyValue jspp::RuntimeError::throw_immutable_assignment_error()
{
    throw RuntimeError::make_error("Assignment to constant variable.", "TypeError");
}
jspp::AnyValue jspp::RuntimeError::throw_invalid_return_statement_error()
{
    throw RuntimeError::make_error("Return statements are only valid inside functions.", "SyntaxError");
}
