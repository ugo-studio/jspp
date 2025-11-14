#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "operators.hpp"
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

namespace jspp
{
    namespace StringPrototypes
    {
        // This function retrieves a prototype method for a given string instance.
        // It captures the string instance to act as the 'this' context for the method.
        inline std::optional<AnyValue> get(const std::string &key, const std::unique_ptr<std::string> &self)
        {
            // --- toString() & valueOf() ---
            if (key == "toString" || key == "valueOf")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               { return AnyValue::make_string(*self); },
                                               key);
            }

            // --- length property ---
            if (key == "length")
            {
                return AnyValue::make_accessor_descriptor([&self](const std::vector<AnyValue>) -> AnyValue
                                                          { return AnyValue::make_number(self->length()); },
                                                          [&self](const std::vector<AnyValue>) -> AnyValue
                                                          { return AnyValue::make_undefined(); },
                                                          false,
                                                          false);
            }

            // --- charAt(pos) ---
            if (key == "charAt")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    double pos = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    int index = static_cast<int>(pos);
                    if (index < 0 || index >= self->length()) {
                        return AnyValue::make_string("");
                    }
                    return AnyValue::make_string(std::string(1, (*self)[index])); },
                                               key);
            }

            // --- toUpperCase() ---
            if (key == "toUpperCase")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    std::transform(self->begin(), self->end(), self->begin(), ::toupper);
                    return AnyValue::make_string(*self); },
                                               key);
            }

            // --- toLowerCase() ---
            if (key == "toLowerCase")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    std::transform(self->begin(), self->end(), self->begin(), ::tolower);
                    return AnyValue::make_string(*self); },
                                               key);
            }

            // --- slice(beginIndex, endIndex) ---
            if (key == "slice")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    int len = self->length();
                    int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                    int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                    if (start < 0) start += len;
                    if (end < 0) end += len;

                    start = std::max(0, std::min(len, start));
                    end = std::max(0, std::min(len, end));

                    if (start >= end) return AnyValue::make_string("");
                    return AnyValue::make_string(self->substr(start, end - start)); },
                                               key);
            }

            // --- trim() ---
            if (key == "trim")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    const char* whitespace = " \t\n\r\f\v";
                    self->erase(0, self->find_first_not_of(whitespace));
                    self->erase(self->find_last_not_of(whitespace) + 1);
                    return AnyValue::make_string(*self); },
                                               key);
            }

            // --- startsWith(searchString, position) ---
            if (key == "startsWith")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    if(args.empty()) return AnyValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                    if (pos > self->length()) pos = self->length();

                    return AnyValue::make_boolean(self->rfind(search, pos) == pos); },
                                               key);
            }

            // --- endsWith(searchString, endPosition) ---
            if (key == "endsWith")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    if(args.empty()) return AnyValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t end_pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : self->length();
                    
                    if (end_pos > self->length()) end_pos = self->length();
                    if (search.length() > end_pos) return AnyValue::make_boolean(false);

                    return AnyValue::make_boolean(self->substr(end_pos - search.length(), search.length()) == search); },
                                               key);
            }

            // --- includes(searchString, position) ---
            if (key == "includes")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    if(args.empty()) return AnyValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                    
                    return AnyValue::make_boolean(self->find(search, pos) != std::string::npos); },
                                               key);
            }

            // --- split(separator) ---
            if (key == "split")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &args) -> AnyValue
                                               {
                    std::string separator = (args.empty() || args[0].is_undefined()) ? "" : args[0].to_std_string();
                    std::vector<std::optional<AnyValue>> result_vec;
                    
                    if (separator.empty()) {
                        for (char c : (*self)) {
                            result_vec.push_back(AnyValue::make_string(std::string(1, c)));
                        }
                    } else {
                        std::string temp = (*self);
                        size_t pos = 0;
                        while ((pos = temp.find(separator)) != std::string::npos) {
                            result_vec.push_back(AnyValue::make_string(temp.substr(0, pos)));
                            temp.erase(0, pos + separator.length());
                        }
                        result_vec.push_back(AnyValue::make_string(temp));
                    }
                    return AnyValue::make_array(result_vec); },
                                               key);
            }

            return std::nullopt;
        }
    }
}