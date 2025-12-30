#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include "utils/log_any_value/helpers.hpp"
#include <string>
#include <optional>

namespace jspp
{
    namespace LogAnyValue
    {
        inline std::optional<std::string> format_primitive(const AnyValue &val, int depth)
        {
            if (val.is_uninitialized()) {
                Exception::throw_uninitialized_reference("#<Object>");
                // return Color::BRIGHT_BLACK + std::string("<uninitialized>") + Color::RESET;
            }
            if (val.is_undefined())
                return Color::BRIGHT_BLACK + std::string("undefined") + Color::RESET;
            if (val.is_null())
                return Color::MAGENTA + std::string("null") + Color::RESET;
            if (val.is_boolean())
                return Color::YELLOW + std::string(val.as_boolean() ? "true" : "false") + Color::RESET;
            if (val.is_number())
                return Color::YELLOW + val.to_std_string() + Color::RESET;
            if (val.is_symbol())
                return Color::BLUE + val.to_std_string() + Color::RESET;
            if (val.is_accessor_descriptor())
                return Color::BLUE + std::string("[Getter/Setter]") + Color::RESET;

            if (val.is_string())
            {
                const std::string &s = val.as_string()->value;
                if (depth == 0)
                    return truncate_string(s);
                return Color::GREEN + std::string("\"") + truncate_string(s) + "\"" + Color::RESET;
            }

            return std::nullopt; // Not a primitive
        }
    }
}