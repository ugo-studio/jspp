#pragma once

#include "types.hpp"
#include "values/function.hpp"

namespace jspp
{

    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline const AnyValue &deref(const std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized())
            {
                throw std::runtime_error("ReferenceError: Cannot access '" + name + "' before initialization");
                // return Exception::throw_uninitialized_reference_error(name); // must also return const AnyValue&
            }
            return *var;
        }
        inline AnyValue &deref(std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized())
            {
                throw std::runtime_error("ReferenceError: Cannot access '" + name + "' before initialization");
            }
            return *var;
        }

    }
}