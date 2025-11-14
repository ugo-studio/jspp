#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"
#include "any_value.hpp"
#include <sstream>
#include <unordered_set>
#include <algorithm>

namespace jspp
{
    namespace LogString
    {
        // --- Configuration for Logging Verbosity ---
        const int MAX_DEPTH = 5;
        const size_t MAX_STRING_LENGTH = 100;
        const size_t MAX_ARRAY_ITEMS = 50;
        const size_t MAX_OBJECT_PROPS = 30;
        // --- Configuration for Horizontal Layout ---
        const size_t HORIZONTAL_ARRAY_MAX_ITEMS = 10;
        const size_t HORIZONTAL_OBJECT_MAX_PROPS = 5;

        // ANSI Color Codes for terminal output
        namespace Color
        {
            const std::string RESET = "\033[0m";
            const std::string GREEN = "\033[32m";
            const std::string YELLOW = "\033[33m";
            const std::string CYAN = "\033[36m";
            const std::string MAGENTA = "\033[35m";
            const std::string BRIGHT_BLACK = "\033[90m"; // Grey
        }

        // Forward declarations
        inline std::string to_log_string(const AnyValue &val);
        inline std::string to_log_string(const AnyValue &val, std::unordered_set<const void *> &visited, int depth);
        inline bool is_simple_value(const AnyValue &val);

        inline bool is_valid_js_identifier(const std::string& s) {
            if (s.empty()) {
                return false;
            }
            if (!std::isalpha(s[0]) && s[0] != '_' && s[0] != '$') {
                return false;
            }
            for (size_t i = 1; i < s.length(); ++i) {
                if (!std::isalnum(s[i]) && s[i] != '_' && s[i] != '$') {
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

        inline std::string truncate_string(const std::string &str)
        {
            if (str.length() > MAX_STRING_LENGTH)
            {
                return str.substr(0, MAX_STRING_LENGTH) + "...";
            }
            return str;
        }

        inline std::string to_log_string(const AnyValue &val)
        {
            std::unordered_set<const void *> visited;
            return to_log_string(val, visited, 0);
        }

        inline std::string to_log_string(const AnyValue &val, std::unordered_set<const void *> &visited, int depth)
        {
            // Primitives and simple wrapped values
            if (val.is_uninitialized())
                return Color::BRIGHT_BLACK + std::string("<uninitialized>") + Color::RESET;
            if (val.is_undefined())
                return Color::BRIGHT_BLACK + std::string("undefined") + Color::RESET;
            if (val.is_null())
                return Color::MAGENTA + std::string("null") + Color::RESET;
            if (val.is_boolean())
                return Color::YELLOW + std::string(val.as_boolean() ? "true" : "false") + Color::RESET;
            if (val.is_number())
                return Color::YELLOW + val.to_std_string() + Color::RESET;
            if (val.is_string())
            {
                const std::string &s = *val.as_string();
                if (depth == 0)
                    return truncate_string(s);
                return Color::GREEN + std::string("\"") + truncate_string(s) + "\"" + Color::RESET;
            }
            if (val.is_function())
            {
                auto fn = val.as_function();
                auto name_part = fn->name.size() > 0 ? ": " + fn->name : "";
                return Color::CYAN + std::string("[Function") + name_part + "]" + Color::RESET;
            }

            // Depth limit
            if (depth > MAX_DEPTH)
            {
                if (val.is_object())
                    return Color::CYAN + std::string("[Object]") + Color::RESET;
                if (val.is_array())
                    return Color::CYAN + std::string("[Array]") + Color::RESET;
            }

            // Circular reference detection
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

            std::string indent(depth * 2, ' ');
            std::string next_indent((depth + 1) * 2, ' ');
            std::stringstream ss;

            // Objects
            if (val.is_object())
            {
                auto obj = val.as_object();

                // If custom toString exists on the object, prefer it
                auto itToString = obj->props.find(jspp::WellKnownSymbols::toString);
                if (itToString != obj->props.end() && itToString->second.is_function())
                {
                    try
                    {
                        auto result = itToString->second.as_function("toString")->call({});
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

                if (use_horizontal_layout)
                {
                    ss << "{ ";
                    size_t current_prop = 0;
                    for (const auto &pair : obj->props)
                    {
                        if (is_valid_js_identifier(pair.first)) {
                            ss << pair.first;
                        } else {
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
                            if (is_valid_js_identifier(pair.first)) {
                                ss << pair.first;
                            } else {
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

            // Arrays
            if (val.is_array())
            {
                auto arr = val.as_array();
                size_t item_count = static_cast<size_t>(arr->length);

                // If small and simple, keep single-line layout
                bool small_and_simple = item_count > 0 && item_count <= HORIZONTAL_ARRAY_MAX_ITEMS;
                if (small_and_simple)
                {
                    for (size_t i = 0; i < item_count; ++i)
                    {
                        const AnyValue *itemVal = nullptr;
                        if (i < arr->dense.size())
                            itemVal = &arr->dense[i];
                        else
                        {
                            auto it = arr->sparse.find(static_cast<uint32_t>(i));
                            if (it != arr->sparse.end())
                                itemVal = &it->second;
                        }
                        if (itemVal && !is_simple_value(*itemVal))
                        {
                            small_and_simple = false;
                            break;
                        }
                    }
                }

                if (small_and_simple)
                {
                    std::stringstream sline;
                    sline << "[ ";
                    for (size_t i = 0; i < item_count; ++i)
                    {
                        const AnyValue *itemVal = nullptr;
                        if (i < arr->dense.size())
                            itemVal = &arr->dense[i];
                        else
                        {
                            auto it = arr->sparse.find(static_cast<uint32_t>(i));
                            if (it != arr->sparse.end())
                                itemVal = &it->second;
                        }
                        if (itemVal)
                            sline << to_log_string(*itemVal, visited, depth + 1);
                        else
                            sline << Color::BRIGHT_BLACK << "<empty>" << Color::RESET;
                        if (i < item_count - 1)
                            sline << Color::BRIGHT_BLACK << ", " << Color::RESET;
                    }
                    sline << " ]";
                    return sline.str();
                }

                // Bun-like multi-line layout: 10 items per row, ordered 0..length-1
                ss << "[\n";

                const size_t items_to_show = std::min(item_count, MAX_ARRAY_ITEMS);
                for (size_t i = 0; i < items_to_show; ++i)
                {
                    // New row every 10 items
                    if (i % 10 == 0)
                        ss << next_indent;

                    const AnyValue *itemVal = nullptr;
                    if (i < arr->dense.size())
                        itemVal = &arr->dense[i];
                    else
                    {
                        auto it = arr->sparse.find(static_cast<uint32_t>(i));
                        if (it != arr->sparse.end())
                            itemVal = &it->second;
                    }

                    if (itemVal)
                        ss << to_log_string(*itemVal, visited, depth + 1);
                    else
                        ss << Color::BRIGHT_BLACK << "<empty>" << Color::RESET;

                    // Comma rules:
                    // - Comma after each item except the very last when there are no more items to print.
                    // - If we will elide the rest (item_count > items_to_show), keep a comma after the last shown item too (like Bun).
                    bool write_comma = (i < items_to_show - 1) || (item_count > items_to_show);
                    if (write_comma)
                        ss << Color::BRIGHT_BLACK << ", " << Color::RESET;

                    // End of row or last shown item → newline
                    if ((i % 10 == 9) || (i == items_to_show - 1))
                        ss << "\n";
                }

                if (item_count > items_to_show)
                {
                    ss << next_indent << Color::BRIGHT_BLACK << "... " << (item_count - items_to_show) << " more items" << Color::RESET << "\n";
                }

                ss << indent << "]";
                return ss.str();
            }

            // Fallback
            return val.to_std_string();
        }
    }
}