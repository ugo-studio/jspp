#pragma once

#include "types.hpp"
#include "utils/well_known_symbols.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include "utils/log_any_value/fwd.hpp"
#include "utils/log_any_value/helpers.hpp"
#include "utils/log_any_value/primitives.hpp"
#include "utils/log_any_value/function.hpp"
#include "utils/log_any_value/object.hpp"
#include "utils/log_any_value/array.hpp"

#include <unordered_set>

namespace jspp
{
    namespace LogAnyValue
    {
        inline std::string to_log_string(const AnyValue &val)
        {
            std::unordered_set<const void *> visited;
            return to_log_string(val, visited, 0);
        }

        inline std::string to_log_string(const AnyValue &val, std::unordered_set<const void *> &visited, int depth)
        {
            // 1. Try Primitives
            auto primitiveStr = format_primitive(val, depth);
            if (primitiveStr.has_value())
            {
                return primitiveStr.value();
            }

            // 2. Functions
            if (val.is_function())
            {
                return format_function(val);
            }

            // 3. Depth limit
            if (depth > MAX_DEPTH)
            {
                if (val.is_object())
                    return Color::CYAN + std::string("[Object]") + Color::RESET;
                if (val.is_array())
                    return Color::CYAN + std::string("[Array]") + Color::RESET;
            }

            // 4. Circular reference detection
            const void *ptr_address = nullptr;
            if (val.is_object())
                ptr_address = val.as_object();
            else if (val.is_array())
                ptr_address = val.as_array();

            if (ptr_address)
            {
                if (visited.count(ptr_address))
                    return Color::CYAN + std::string("[Circular]") + Color::RESET;
                visited.insert(ptr_address);
            }

            // 5. Complex Types (Objects & Arrays)
            if (val.is_object())
            {
                return format_object(val, visited, depth);
            }

            if (val.is_array())
            {
                return format_array(val, visited, depth);
            }

            // 6. DataDescriptor
            if (val.is_data_descriptor())
            {
                auto desc = val.as_data_descriptor();
                if (desc->enumerable)
                {
                    return to_log_string((*desc->value), visited, depth);
                }
                else
                {
                    // Will not be printed if the method works well
                    return Color::BRIGHT_BLACK + "<non-enumerable>" + Color::RESET;
                }
            }

            // Fallback
            return val.to_std_string();
        }
    }
}