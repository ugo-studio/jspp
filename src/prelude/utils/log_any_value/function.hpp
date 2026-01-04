#pragma once
#include "types.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include <string>

namespace jspp
{
    namespace LogAnyValue
    {
        inline std::string format_function(const AnyValue &val)
        {
            auto fn = val.as_function();

            if (fn->is_class)
            {
                std::string extends_part = "";
                if (!fn->proto.is_uninitialized() && !fn->proto.is_undefined() && !fn->proto.is_null())
                {
                    if (fn->proto.is_function())
                    {
                        auto parent = fn->proto.as_function();
                        std::string name = parent->name.value_or("");
                        if (!name.empty())
                        {
                            extends_part = " extends " + name;
                        }
                    }
                }
                std::string name = fn->name.value_or("");
                return Color::CYAN + std::string("[class ") + (name.empty() ? "(anonymous)" : name) + extends_part + "]" + Color::RESET;
            }

            auto type_part = fn->is_generator ? "GeneratorFunction" : "Function";
            auto name_part = fn->name.has_value() ? ": " + fn->name.value() : " (anonymous)";
            return Color::CYAN + "[" + type_part + name_part + "]" + Color::RESET;
        }
    }
}
