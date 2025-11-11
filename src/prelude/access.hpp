#pragma once

#include "types.hpp"
#include "values/function.hpp"
#include "error.hpp"

namespace jspp
{
    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline const AnyValue &deref(const std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline AnyValue &deref(std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline const AnyValue &deref(const std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline AnyValue &deref(std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }

    }
}