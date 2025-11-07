#pragma once

#include "types.hpp"
#include "values/function.hpp"
// #include "exception.hpp"

namespace jspp
{

    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline const AnyValue &deref(const std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized())
            {
                throw std::runtime_error("ReferenceError: Cannot access '" + name + "' before initialization");
                // return Exception::throw_uninitialized_reference_error(name); // must also return const AnyValue&
            }
            return *var;
        }

        // Helper function to call JsFunction
        inline AnyValue call_function(const AnyValue &var, const std::vector<AnyValue> &args, const std::string &name)
        {
            if (var.is_function())
            {
                return var.as_function()->call(args);
            }
            throw std::runtime_error("TypeError: " + name + " is not a function");
            // throw Exception::make_error(name + " is not a function", "TypeError");
        }

    }
}