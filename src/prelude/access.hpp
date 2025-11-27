#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"
#include "values/function.hpp"
#include "values/symbol.hpp"
#include "error.hpp"
#include "any_value.hpp"
#include <ranges>

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
                auto len = obj.as_string()->length();
                for (auto i = 0; i < len; ++i)
                {
                    keys.push_back(std::to_string(i));
                }
            }

            return keys;
        }
        inline JsIterator<AnyValue> *get_object_values(const AnyValue &obj)
        {
            // auto generatorFunc = obj.get_own_property(WellKnownSymbols::iterator->key);
            // if (generatorFunc.is_function())
            // {
            //     auto it = generatorFunc.as_function()->call({});
            //     if (it.is_iterator())
            //     {
            //         return it.as_iterator();
            //     }
            // }
            throw RuntimeError::make_error("#<object> is not iterable", "TypeError");
        }
    }
}