#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"
#include "convert.hpp"
#include "object.hpp"
#include <sstream>
#include <set>
#include <algorithm> // For std::min

namespace jspp
{
    // Forward declaration of Prototype namespace and get_custom_prototype function
    namespace Prototype
    {
        jspp::AnyValue get_custom_prototype(const jspp::AnyValue &obj, const jspp::AnyValue &key);
    }

    namespace Log
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
        std::string to_log_string(const AnyValue &val, std::set<const void *> &visited, int depth);
        bool is_simple_value(const AnyValue &val);

        /**
         * @brief Creates a color-coded, JavaScript-like string for logging any Jspp value.
         */
        inline std::string to_log_string(const AnyValue &val)
        {
            std::set<const void *> visited;
            return to_log_string(val, visited, 0);
        }

        /**
         * @brief Checks if a value is a primitive or a simple wrapped type.
         * Arrays and Objects are considered complex.
         */
        inline bool is_simple_value(const AnyValue &val)
        {
            if (!val.has_value())
                return true;
            const auto &type = val.type();

            return !(type == typeid(std::shared_ptr<JsObject>) || type == typeid(std::shared_ptr<JsArray>));
        }

        /**
         * @brief Truncates a string if it's longer than the configured maximum length.
         */
        inline std::string truncate_string(const std::string &str)
        {
            if (str.length() > MAX_STRING_LENGTH)
            {
                return str.substr(0, MAX_STRING_LENGTH) + "...";
            }
            return str;
        }

        /**
         * @brief The recursive implementation for creating a log string.
         */
        inline std::string to_log_string(const AnyValue &val, std::set<const void *> &visited, int depth)
        {
            if (!val.has_value())
                return Color::BRIGHT_BLACK + "undefined" + Color::RESET;

            const std::type_info &type = val.type();

            if (depth > MAX_DEPTH)
            {
                if (type == typeid(std::shared_ptr<JsObject>))
                    return Color::CYAN + "[Object]" + Color::RESET;
                if (type == typeid(std::shared_ptr<JsArray>))
                    return Color::CYAN + "[Array]" + Color::RESET;
            }

            // Handle primitive and simple types
            if (type == typeid(Uninitialized))
                return Color::BRIGHT_BLACK + "<uninitialized>" + Color::RESET;
            if (type == typeid(Undefined))
                return Color::BRIGHT_BLACK + "undefined" + Color::RESET;
            if (type == typeid(Null))
                return Color::MAGENTA + "null" + Color::RESET;
            if (type == typeid(bool))
                return Color::YELLOW + (std::any_cast<bool>(val) ? "true" : "false") + Color::RESET;
            if (type == typeid(int) || type == typeid(double) || type == typeid(std::shared_ptr<JsNumber>))
                return Color::YELLOW + jspp::Convert::to_string(val) + Color::RESET;

            // Handle strings with truncation and conditional quoting
            auto format_string = [&](const std::string &s)
            {
                // Top-level strings (depth 0) are not quoted.
                if (depth == 0)
                    return truncate_string(s);
                return Color::GREEN + "\"" + truncate_string(s) + "\"" + Color::RESET;
            };
            if (type == typeid(std::string) || type == typeid(const char *) || type == typeid(std::shared_ptr<JsString>))
                return format_string(jspp::Convert::to_string(val));

            // Handle other wrapped primitives
            if (type == typeid(std::shared_ptr<JsBoolean>))
                return Color::YELLOW + (std::any_cast<std::shared_ptr<JsBoolean>>(val)->value ? "true" : "false") + Color::RESET;
            if (type == typeid(std::shared_ptr<JsFunction>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsFunction>>(val);
                auto name_part = ptr->name.size() > 0 ? ": " + ptr->name : "";
                return Color::CYAN + "[Function" + name_part + "]" + Color::RESET;
            }

            const void *ptr_address = nullptr;
            if (type == typeid(std::shared_ptr<JsObject>))
                ptr_address = std::any_cast<std::shared_ptr<JsObject>>(val).get();
            else if (type == typeid(std::shared_ptr<JsArray>))
                ptr_address = std::any_cast<std::shared_ptr<JsArray>>(val).get();

            if (ptr_address)
            {
                if (visited.count(ptr_address))
                    return Color::CYAN + "[Circular]" + Color::RESET;
                visited.insert(ptr_address);
            }

            std::string indent(depth * 2, ' ');
            std::string next_indent((depth + 1) * 2, ' ');
            std::stringstream ss;

            // Handle JsObject
            if (type == typeid(std::shared_ptr<JsObject>))
            {
                auto ptr = std::any_cast<std::shared_ptr<JsObject>>(val);

                // Use the custom `toString` prototype if available; (exclude defaults)
                auto toStringFn = jspp::Prototype::get_custom_prototype(val, WellKnownSymbols::toString);
                if (toStringFn.type() == typeid(std::shared_ptr<jspp::JsFunction>))
                {
                    auto fn = std::any_cast<std::shared_ptr<jspp::JsFunction>>(toStringFn);
                    return Convert::to_string(fn->call({}));
                }

                // Else parse object manually
                size_t prop_count = ptr->properties.size();

                bool use_horizontal_layout = prop_count <= HORIZONTAL_OBJECT_MAX_PROPS && prop_count > 0;
                if (use_horizontal_layout)
                {
                    for (const auto &pair : ptr->properties)
                    {
                        AnyValue prop_val;
                        if (std::holds_alternative<DataDescriptor>(pair.second))
                            prop_val = std::get<DataDescriptor>(pair.second).value;
                        else if (std::holds_alternative<AnyValue>(pair.second))
                            prop_val = std::get<AnyValue>(pair.second);
                        else
                        {
                            use_horizontal_layout = false;
                            break;
                        } // Accessors force vertical layout

                        if (!is_simple_value(prop_val))
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
                    for (const auto &pair : ptr->properties)
                    {
                        ss << pair.first << ": ";
                        if (std::holds_alternative<DataDescriptor>(pair.second))
                            ss << to_log_string(std::get<DataDescriptor>(pair.second).value, visited, depth + 1);
                        else if (std::holds_alternative<AnyValue>(pair.second))
                            ss << to_log_string(std::get<AnyValue>(pair.second), visited, depth + 1);

                        if (++current_prop < prop_count)
                            ss << ", ";
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
                        for (const auto &pair : ptr->properties)
                        {
                            if (props_shown >= MAX_OBJECT_PROPS)
                                break;
                            if (props_shown > 0)
                                ss << ",\n";

                            ss << next_indent << pair.first << ": ";
                            if (std::holds_alternative<DataDescriptor>(pair.second))
                                ss << to_log_string(std::get<DataDescriptor>(pair.second).value, visited, depth + 1);
                            else if (std::holds_alternative<AccessorDescriptor>(pair.second))
                                ss << Color::CYAN << "[Getter/Setter]" << Color::RESET;
                            else if (std::holds_alternative<AnyValue>(pair.second))
                                ss << to_log_string(std::get<AnyValue>(pair.second), visited, depth + 1);
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

            // Handle JsArray
            if (type == typeid(std::shared_ptr<JsArray>))
            {
                auto ptr = std::any_cast<std::shared_ptr<JsArray>>(val);
                size_t item_count = ptr->items.size();

                bool use_horizontal_layout = item_count <= HORIZONTAL_ARRAY_MAX_ITEMS && item_count > 0;
                if (use_horizontal_layout)
                {
                    for (const auto &item : ptr->items)
                    {
                        if (!is_simple_value(item))
                        {
                            use_horizontal_layout = false;
                            break;
                        }
                    }
                }

                if (use_horizontal_layout)
                {
                    ss << "[ ";
                    for (size_t i = 0; i < item_count; ++i)
                    {
                        ss << to_log_string(ptr->items[i], visited, depth + 1);
                        if (i < item_count - 1)
                            ss << ", ";
                    }
                    ss << " ]";
                }
                else
                {
                    ss << "[";
                    if (item_count > 0)
                    {
                        ss << "\n";
                        size_t items_to_show = std::min(item_count, MAX_ARRAY_ITEMS);
                        for (size_t i = 0; i < items_to_show; ++i)
                        {
                            ss << next_indent << to_log_string(ptr->items[i], visited, depth + 1);
                            if (i < items_to_show - 1)
                                ss << ",\n";
                        }
                        if (item_count > MAX_ARRAY_ITEMS)
                            ss << ",\n"
                               << next_indent << Color::BRIGHT_BLACK << "... " << (item_count - MAX_ARRAY_ITEMS) << " more items" << Color::RESET;
                        ss << "\n"
                           << indent;
                    }
                    ss << "]";
                }
                return ss.str();
            }

            return jspp::Convert::to_string(val);
        }
    }
}