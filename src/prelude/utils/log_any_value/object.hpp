#pragma once
#include "types.hpp"
#include "any_value.hpp"
#include "library/error.hpp"
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

            size_t prop_count = obj->storage.size() + obj->symbol_props.size();
            bool use_horizontal_layout = prop_count > 0 && prop_count <= HORIZONTAL_OBJECT_MAX_PROPS;

            for (size_t i = 0; i < obj->storage.size(); ++i)
            {
                const auto &prop_val = obj->storage[i];
                if (!is_enumerable_property(prop_val))
                {
                    prop_count--;
                    continue;
                }
                if (use_horizontal_layout && !is_simple_value(prop_val))
                {
                    use_horizontal_layout = false;
                    break;
                }
            }
            for (const auto &pair : obj->symbol_props)
            {
                if (!is_enumerable_property(pair.second))
                {
                    prop_count--;
                    continue;
                }
                if (use_horizontal_layout && !is_simple_value(pair.second))
                {
                    use_horizontal_layout = false;
                    break;
                }
            }

            std::string indent(depth * 2, ' ');
            std::string next_indent((depth + 1) * 2, ' ');
            std::stringstream ss;
// ... (omitting intermediate parts for brevity in explanation, but including in replacement)
            // Use Symbol.toStringTag for object prefix
            bool has_tag = false;
            try
            {
                auto tag_val = val.get_own_property(AnyValue::from_symbol(WellKnownSymbols::toStringTag));
                if (tag_val.is_string())
                {
                    ss << tag_val.to_std_string() << " ";
                    has_tag = true;
                }
            }
            catch (...)
            {
            }

            // Special handling for Error objects
            try
            {
                const AnyValue args[] = {val};
                auto is_error = is_truthy(isErrorFn.call(isErrorFn, std::span<const jspp::AnyValue>(args, 1)));
                if (is_error)
                {
                    auto result = errorToStringFn.call(val, std::span<const jspp::AnyValue>{});
                    if (result.is_string())
                    {
                        ss << result.to_std_string();
                        if (prop_count == 0)
                        {
                            return ss.str();
                        }
                        ss << " ";
                    }
                }
            }
            catch (...)
            {
                // ignore
            }

            if (use_horizontal_layout)
            {
                ss << "{ ";
                size_t current_prop = 0;
                for (size_t i = 0; i < obj->shape->property_names.size(); ++i)
                {
                    const auto &key = obj->shape->property_names[i];
                    const auto &prop_val = obj->storage[i];

                    if (!is_enumerable_property(prop_val))
                        continue;

                    if (is_valid_js_identifier(key))
                    {
                        ss << key;
                    }
                    else
                    {
                        ss << Color::GREEN << "\"" << key << "\"" << Color::RESET;
                    }
                    ss << Color::BRIGHT_BLACK << ": " << Color::RESET;
                    ss << to_log_string(prop_val, visited, depth + 1);
                    if (++current_prop < prop_count)
                        ss << Color::BRIGHT_BLACK << ", " << Color::RESET;
                }
                for (const auto &pair : obj->symbol_props)
                {
                    if (!is_enumerable_property(pair.second))
                        continue;

                    ss << Color::BLUE << pair.first.to_std_string() << Color::RESET;
                    ss << Color::BRIGHT_BLACK << ": " << Color::RESET;
                    ss << to_log_string(pair.second, visited, depth + 1);
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
                    for (size_t i = 0; i < obj->shape->property_names.size(); ++i)
                    {
                        const auto &key = obj->shape->property_names[i];
                        const auto &prop_val = obj->storage[i];

                        if (props_shown >= MAX_OBJECT_PROPS)
                            break;

                        if (!is_enumerable_property(prop_val))
                            continue;

                        if (props_shown > 0)
                            ss << ",\n";

                        ss << next_indent;
                        if (is_valid_js_identifier(key))
                        {
                            ss << key;
                        }
                        else
                        {
                            ss << Color::GREEN << "\"" << key << "\"" << Color::RESET;
                        }
                        ss << Color::BRIGHT_BLACK << ": " << Color::RESET;
                        ss << to_log_string(prop_val, visited, depth + 1);
                        props_shown++;
                    }
                    for (const auto &pair : obj->symbol_props)
                    {
                        if (props_shown >= MAX_OBJECT_PROPS)
                            break;

                        if (!is_enumerable_property(pair.second))
                            continue;

                        if (props_shown > 0)
                            ss << ",\n";

                        ss << next_indent;
                        ss << Color::BLUE << pair.first.to_std_string() << Color::RESET;
                        ss << Color::BRIGHT_BLACK << ": " << Color::RESET;
                        ss << to_log_string(pair.second, visited, depth + 1);
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