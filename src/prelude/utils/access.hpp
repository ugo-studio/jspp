#pragma once

#include "types.hpp"
#include "utils/well_known_symbols.hpp"
#include "values/function.hpp"
#include "values/symbol.hpp"
#include "error.hpp"
#include "any_value.hpp"
#include <ranges>

namespace jspp
{
    namespace Access
    {
        // Helper function to check for TDZ and deref heap-allocated variables
        inline const AnyValue &deref_ptr(const std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline AnyValue &deref_ptr(std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline const AnyValue &deref_ptr(const std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }
        inline AnyValue &deref_ptr(std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return *var;
        }

        // Helper function to check for TDZ on stack-allocated variables
        inline const AnyValue &deref_stack(const AnyValue &var, const std::string &name)
        {
            if (var.is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return var;
        }
        inline AnyValue &deref_stack(AnyValue &var, const std::string &name)
        {
            if (var.is_uninitialized()) [[unlikely]]
            {
                RuntimeError::throw_uninitialized_reference_error(name);
            }
            return var;
        }

        inline std::vector<std::string> get_object_keys(const AnyValue &obj)
        {
            std::vector<std::string> keys;

            if (obj.is_object())
            {
                auto ptr = obj.as_object();
                for (const auto &pair : ptr->props)
                {
                    if (!JsSymbol::is_internal_key(pair.first))
                        keys.push_back(pair.first);
                }
            }
            if (obj.is_function())
            {
                auto ptr = obj.as_function();
                for (const auto &pair : ptr->props)
                {
                    if (!JsSymbol::is_internal_key(pair.first))
                        keys.push_back(pair.first);
                }
            }
            if (obj.is_array())
            {
                auto len = obj.as_array()->length;
                for (auto i = 0; i < len; ++i)
                {
                    keys.push_back(std::to_string(i));
                }
            }
            if (obj.is_string())
            {
                auto len = obj.as_string()->value.length();
                for (auto i = 0; i < len; ++i)
                {
                    keys.push_back(std::to_string(i));
                }
            }

            return keys;
        }
        inline AnyValue get_object_value_iterator(const AnyValue &obj, const std::string &name)
        {
            if (obj.is_iterator())
            {
                return obj;
            }

            auto gen_fn = obj.get_own_property(WellKnownSymbols::iterator->key);
            if (gen_fn.is_generator())
            {
                auto iter = gen_fn.as_function()->call(gen_fn, {});
                if (iter.is_iterator())
                {
                    return iter;
                }
            }

            throw jspp::RuntimeError::make_error(name + " is not iterable", "TypeError");
        }
    }
}