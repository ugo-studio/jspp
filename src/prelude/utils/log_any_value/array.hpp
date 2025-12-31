#pragma once
#include "types.hpp"
#include "any_value.hpp"
#include "utils/log_any_value/config.hpp"
#include "utils/log_any_value/helpers.hpp"
#include "utils/log_any_value/fwd.hpp"
#include <string>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <optional>

namespace jspp
{
    namespace LogAnyValue
    {
        inline std::string format_array(const AnyValue &val, std::unordered_set<const void *> &visited, int depth)
        {
            auto arr = val.as_array();
            size_t item_count = static_cast<size_t>(arr->length);
            size_t prop_count = arr->props.size();

            // // If custom toString exists on the object, prefer it
            // auto itToString = arr->props.find("toString");
            // if (depth > 0 && itToString != arr->props.end() && itToString->second.is_function())
            // {
            //     try
            //     {
            //         auto result = itToString->second.as_function()->call(itToString->second, {});
            //         return to_log_string(result, visited, depth);
            //     }
            //     catch (...)
            //     {
            //         // ignore and fallback to manual formatting
            //     }
            // }

            std::string indent(depth * 2, ' ');
            std::string next_indent((depth + 1) * 2, ' ');
            std::stringstream ss;

            // Horizontal layout for small and simple arrays
            bool use_horizontal_layout = item_count <= HORIZONTAL_ARRAY_MAX_ITEMS;
            if (use_horizontal_layout)
            {
                for (size_t i = 0; i < item_count; ++i)
                {
                    AnyValue itemVal = AnyValue::make_uninitialized();
                    if (i < arr->dense.size())
                    {
                        itemVal = arr->dense[i];
                    }
                    else
                    {
                        auto it = arr->sparse.find(static_cast<uint32_t>(i));
                        if (it != arr->sparse.end())
                        {
                            itemVal = it->second;
                        }
                    }
                    if (!itemVal.is_uninitialized() && !is_simple_value(itemVal))
                    {
                        use_horizontal_layout = false;
                        break;
                    }
                }
            }

            if (use_horizontal_layout)
            {
                ss << "[ ";
                size_t empty_count = 0;
                bool needs_comma = false;

                for (size_t i = 0; i < item_count; ++i)
                {
                    AnyValue itemVal = AnyValue::make_uninitialized();
                    if (i < arr->dense.size())
                    {
                        itemVal = arr->dense[i];
                    }
                    else
                    {
                        auto it = arr->sparse.find(static_cast<uint32_t>(i));
                        if (it != arr->sparse.end())
                        {
                            itemVal = it->second;
                        }
                    }

                    if (!itemVal.is_uninitialized())
                    {
                        if (empty_count > 0)
                        {
                            if (needs_comma)
                                ss << Color::BRIGHT_BLACK << ", " << Color::RESET;
                            ss << Color::BRIGHT_BLACK << empty_count << " x empty item" << (empty_count > 1 ? "s" : "") << Color::RESET;
                            needs_comma = true;
                            empty_count = 0;
                        }
                        if (needs_comma)
                            ss << Color::BRIGHT_BLACK << ", " << Color::RESET;
                        ss << to_log_string(itemVal, visited, depth + 1);
                        needs_comma = true;
                    }
                    else
                    {
                        empty_count++;
                    }
                }

                if (empty_count > 0)
                {
                    if (needs_comma)
                        ss << Color::BRIGHT_BLACK << ", " << Color::RESET;
                    ss << Color::BRIGHT_BLACK << empty_count << " x empty item" << (empty_count > 1 ? "s" : "") << Color::RESET;
                }

                // Print properties
                if (prop_count > 0)
                {
                    ss << Color::BRIGHT_BLACK << ", " << Color::RESET;

                    size_t current_prop = 0;
                    for (const auto &pair : arr->props)
                    {
                        if (!is_enumerable_property(pair.second))
                            continue;

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
                }

                ss << " ]";
                return ss.str();
            }

            // Bun-like multi-line layout
            ss << "[\n";

            const size_t items_to_show = std::min(item_count, MAX_ARRAY_ITEMS);
            size_t empty_count = 0;
            bool first_item_printed = false;

            for (size_t i = 0; i < items_to_show; ++i)
            {
                AnyValue itemVal = AnyValue::make_uninitialized();
                if (i < arr->dense.size())
                {
                    itemVal = arr->dense[i];
                }
                else
                {
                    auto it = arr->sparse.find(static_cast<uint32_t>(i));
                    if (it != arr->sparse.end())
                    {
                        itemVal = it->second;
                    }
                }

                if (!itemVal.is_uninitialized())
                {
                    if (empty_count > 0)
                    {
                        if (first_item_printed)
                            ss << Color::BRIGHT_BLACK << ",\n"
                               << Color::RESET;
                        ss << next_indent << Color::BRIGHT_BLACK << empty_count << " x empty item" << (empty_count > 1 ? "s" : "") << Color::RESET;
                        first_item_printed = true;
                        empty_count = 0;
                    }
                    if (first_item_printed)
                        ss << Color::BRIGHT_BLACK << ",\n"
                           << Color::RESET;
                    ss << next_indent << to_log_string(itemVal, visited, depth + 1);
                    first_item_printed = true;
                }
                else
                {
                    empty_count++;
                }
            }

            if (empty_count > 0)
            {
                if (first_item_printed)
                    ss << Color::BRIGHT_BLACK << ",\n"
                       << Color::RESET;
                ss << next_indent << Color::BRIGHT_BLACK << empty_count << " x empty item" << (empty_count > 1 ? "s" : "") << Color::RESET;
                first_item_printed = true;
            }

            if (item_count > items_to_show)
            {
                if (first_item_printed)
                    ss << Color::BRIGHT_BLACK << ",\n"
                       << Color::RESET;
                ss << next_indent << Color::BRIGHT_BLACK << "... " << (item_count - items_to_show) << " more items" << Color::RESET;
            }
            // Print properties
            else if (prop_count > 0)
            {
                if (first_item_printed)
                    ss << Color::BRIGHT_BLACK << ",\n"
                       << Color::RESET;

                size_t current_prop = 0;
                for (const auto &pair : arr->props)
                {
                    if (current_prop >= MAX_OBJECT_PROPS)
                        break;
                    if (!is_enumerable_property(pair.second))
                        continue;

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
                    if (++current_prop < prop_count)
                        ss << Color::BRIGHT_BLACK << ",\n"
                           << Color::RESET;
                }
            }
            ss << "\n";
            ss << indent << "]";
            return ss.str();
        }
    }
}