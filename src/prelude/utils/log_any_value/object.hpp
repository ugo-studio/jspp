#pragma once
#include "types.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include "utils/log_any_value/helpers.hpp"
#include "utils/log_any_value/fwd.hpp" // Required for recursive to_log_string call
#include <string>
#include <sstream>
#include <unordered_set>

namespace jspp
{
    namespace LogAnyValue
    {
        inline std::string format_object(const AnyValue &val, std::unordered_set<const void *> &visited, int depth)
        {
            auto obj = val.as_object();

            // If custom toString exists on the object, prefer it
            auto itToString = obj->props.find("toString");
            if (itToString != obj->props.end() && itToString->second.is_function())
            {
                try
                {
                    auto result = itToString->second.as_function()->call(itToString->second, {});
                    return to_log_string(result, visited, depth);
                }
                catch (...)
                {
                    // ignore and fallback to manual formatting
                }
            }

            size_t prop_count = obj->props.size();
            bool use_horizontal_layout = prop_count > 0 && prop_count <= HORIZONTAL_OBJECT_MAX_PROPS;

            if (use_horizontal_layout)
            {
                for (const auto &pair : obj->props)
                {
                    if (!is_simple_value(pair.second))
                    {
                        use_horizontal_layout = false;
                        break;
                    }
                }
            }

            std::string indent(depth * 2, ' ');
            std::string next_indent((depth + 1) * 2, ' ');
            std::stringstream ss;

            if (use_horizontal_layout)
            {
                ss << "{ ";
                size_t current_prop = 0;
                for (const auto &pair : obj->props)
                {
                    if (is_valid_js_identifier(pair.first))
                    {
                        ss << pair.first;
                    }
                    else
                    {
                        ss << "\"" << pair.first << "\"";
                    }
                    ss << ": " << to_log_string(pair.second, visited, depth + 1);
                    if (++current_prop < prop_count)
                        ss << Color::BRIGHT_BLACK << ", " << Color::RESET;
                }
                ss << " }";
            }
            else
            {
                ss << "{";
                if (prop_count > 0)
                {
                    ss << "\n";
                    size_t props_shown = 0;
                    for (const auto &pair : obj->props)
                    {
                        if (props_shown >= MAX_OBJECT_PROPS)
                            break;
                        if (props_shown > 0)
                            ss << ",\n";

                        ss << next_indent;
                        if (is_valid_js_identifier(pair.first))
                        {
                            ss << pair.first;
                        }
                        else
                        {
                            ss << "\"" << pair.first << "\"";
                        }
                        ss << ": " << to_log_string(pair.second, visited, depth + 1);
                        props_shown++;
                    }
                    if (prop_count > MAX_OBJECT_PROPS)
                        ss << ",\n"
                           << next_indent << Color::BRIGHT_BLACK << "... " << (prop_count - MAX_OBJECT_PROPS) << " more properties" << Color::RESET;
                    ss << "\n"
                       << indent;
                }
                ss << "}";
            }
            return ss.str();
        }
    }
}