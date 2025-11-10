#pragma once

#include "types.hpp"
#include "values/any_value.hpp"

namespace jspp
{
    struct RuntimeError : std::exception
    {
        AnyValue data;
        RuntimeError(AnyValue d) : data(std::move(d)) {}
        const char *what() const noexcept override { return data.to_std_string().c_str(); }
    };

    namespace Exception
    {
        inline auto make_error(const std::string &message, const std::string &name = "Error")
        {
            auto errorObj = std::make_shared<AnyValue>(AnyValue::make_object({{"message", AnyValue::make_string(message)}, {"name", AnyValue::make_string(name)}}));
            (*errorObj)[WellKnownSymbols::toString] = AnyValue::make_function([errorObj](const std::vector<AnyValue> &) -> AnyValue
                                                                              {
                                                                                  AnyValue name = (*errorObj)["name"];
                                                                                  AnyValue message = (*errorObj)["message"];
                                                                                  std::string str = "";
                                                                                  if (name.is_string())
                                                                                      str = name.to_std_string();
                                                                                  else
                                                                                      str = "Error";
                                                                                  str += ": ";
                                                                                  if (message.is_string())
                                                                                      str = message.to_std_string();
                                                                                  return AnyValue::make_string(str); },
                                                                              WellKnownSymbols::toString);
            return RuntimeError(*errorObj);
        }
        inline AnyValue error_to_value(const std::exception &ex)
        {
            if (const RuntimeError *err = dynamic_cast<const RuntimeError *>(&ex))
            {
                return err->data;
            }
            else
            {
                return AnyValue::make_string(ex.what());
            }
        }

        // --- THROWERS
        inline AnyValue throw_unresolved_reference_error(const std::string &var_name)
        {
            throw make_error(var_name + " is not defined", "ReferenceError");
        }
        inline AnyValue throw_uninitialized_reference_error(const std::string &var_name)
        {
            throw make_error("Cannot access '" + var_name + "' before initialization", "ReferenceError");
        }
        inline AnyValue throw_immutable_assignment_error()
        {
            throw make_error("Assignment to constant variable.", "TypeError");
        }
        inline AnyValue throw_invalid_return_statement_error()
        {
            throw make_error("Return statements are only valid inside functions.", "SyntaxError");
        }

    }
}
