#pragma once

#include "types.hpp"
#include "error.hpp"
#include "js_value.hpp"

const char *jspp::RuntimeError::what() const noexcept
{
    return data->to_std_string().c_str();
}

jspp::RuntimeError jspp::RuntimeError::make_error(const std::string &message, const std::string &name = "Error")
{
    auto errorObj = std::make_shared<JsValue>(JsValue::make_object({{"message", JsValue::make_string(message)}, {"name", JsValue::make_string(name)}}));
    (*errorObj).set_own_property(WellKnownSymbols::toString, JsValue::make_function([errorObj](const std::vector<JsValue> &) -> JsValue
                                                                                     {
                                                                                  JsValue name = (*errorObj).get_own_property("name");
                                                                                  JsValue message = (*errorObj).get_own_property("message");
                                                                                  std::string str = "";
                                                                                  if (name.is_string())
                                                                                      str = name.to_std_string();
                                                                                  else
                                                                                      str = "Error";
                                                                                  str += ": ";
                                                                                  if (message.is_string())
                                                                                      str += message.to_std_string();
                                                                                  return JsValue::make_string(str); },
                                                                                     WellKnownSymbols::toString));
    return RuntimeError(errorObj);
}
jspp::JsValue jspp::RuntimeError::error_to_value(const std::exception &ex)
{
    if (const jspp::RuntimeError *err = dynamic_cast<const jspp::RuntimeError *>(&ex))
    {
        return (*err->data);
    }
    else
    {
        return JsValue::make_string(ex.what());
    }
}

// --- THROWERS
jspp::JsValue jspp::RuntimeError::throw_unresolved_reference_error(const std::string &var_name)
{
    throw RuntimeError::make_error(var_name + " is not defined", "ReferenceError");
}
jspp::JsValue jspp::RuntimeError::throw_uninitialized_reference_error(const std::string &var_name)
{
    throw RuntimeError::make_error("Cannot access '" + var_name + "' before initialization", "ReferenceError");
}
jspp::JsValue jspp::RuntimeError::throw_immutable_assignment_error()
{
    throw RuntimeError::make_error("Assignment to constant variable.", "TypeError");
}
jspp::JsValue jspp::RuntimeError::throw_invalid_return_statement_error()
{
    throw RuntimeError::make_error("Return statements are only valid inside functions.", "SyntaxError");
}
