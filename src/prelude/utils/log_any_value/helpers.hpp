#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include <string>
#include <cctype>

namespace jspp
{
    namespace LogAnyValue
    {
        inline bool is_valid_js_identifier(const std::string &s)
        {
            if (s.empty())
            {
                return false;
            }
            if (!std::isalpha(s[0]) && s[0] != '_' && s[0] != '$')
            {
                return false;
            }
            for (size_t i = 1; i < s.length(); ++i)
            {
                if (!std::isalnum(s[i]) && s[i] != '_' && s[i] != '$')
                {
                    return false;
                }
            }
            return true;
        }

        inline bool is_simple_value(const AnyValue &val)
        {
            return val.is_undefined() || val.is_null() || val.is_uninitialized() ||
                   val.is_boolean() || val.is_number() || val.is_string();
        }

        inline bool is_enumerable_property(const AnyValue &val)
        {
            if (val.is_data_descriptor())
            {
                return val.as_data_descriptor()->enumerable;
            }
            else if (
                val.is_accessor_descriptor())
            {
                return val.as_accessor_descriptor()->enumerable;
            }
            return true;
        }

        inline std::string truncate_string(const std::string &str)
        {
            if (str.length() > MAX_STRING_LENGTH)
            {
                return str.substr(0, MAX_STRING_LENGTH) + "...";
            }
            return str;
        }
    }
}