#pragma once

#include "types.hpp"
#include "js_value.hpp"
#include "operators.hpp"
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace jspp
{
    namespace StringPrototypes
    {
        // This function retrieves a prototype method for a given string instance.
        // It captures the string instance to act as the 'this' context for the method.
        inline std::optional<JsValue> get(const std::string &key, const std::unique_ptr<std::string> &self)
        {
            // --- toString() & valueOf() ---
            if (key == "toString" || key == "valueOf")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               { return JsValue::make_string(*self); },
                                               key);
            }

            // --- length property ---
            if (key == "length")
            {
                return JsValue::make_accessor_descriptor([&self](const std::vector<JsValue>) -> JsValue
                                                          { return JsValue::make_number(self->length()); },
                                                          [&self](const std::vector<JsValue>) -> JsValue
                                                          { return JsValue::make_undefined(); },
                                                          false,
                                                          false);
            }

            // --- charAt(pos) ---
            if (key == "charAt")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    double pos = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    int index = static_cast<int>(pos);
                    if (index < 0 || index >= self->length()) {
                        return JsValue::make_string("");
                    }
                    return JsValue::make_string(std::string(1, (*self)[index])); },
                                               key);
            }

            // --- concat(str1, str2, ...) ---
            if (key == "concat")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    std::string result = *self;
                    for (const auto& arg : args)
                    {
                        result += arg.to_std_string();
                    }
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- endsWith(searchString, endPosition) ---
            if (key == "endsWith")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if(args.empty()) return JsValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t end_pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : self->length();
                    
                    if (end_pos > self->length()) end_pos = self->length();
                    if (search.length() > end_pos) return JsValue::make_boolean(false);

                    return JsValue::make_boolean(self->substr(end_pos - search.length(), search.length()) == search); },
                                               key);
            }

            // --- includes(searchString, position) ---
            if (key == "includes")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if(args.empty()) return JsValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                    
                    return JsValue::make_boolean(self->find(search, pos) != std::string::npos); },
                                               key);
            }

            // --- indexOf(searchString, position) ---
            if (key == "indexOf")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if (args.empty()) return JsValue::make_number(-1);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                    size_t result = self->find(search, pos);
                    return result == std::string::npos ? JsValue::make_number(-1) : JsValue::make_number(result); },
                                               key);
            }

            // --- lastIndexOf(searchString, position) ---
            if (key == "lastIndexOf")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if (args.empty()) return JsValue::make_number(-1);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : std::string::npos;
                    size_t result = self->rfind(search, pos);
                    return result == std::string::npos ? JsValue::make_number(-1) : JsValue::make_number(result); },
                                               key);
            }

            // --- padEnd(targetLength, padString) ---
            if (key == "padEnd")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                    if (self->length() >= target_length) return JsValue::make_string(*self);
                    std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                    std::string result = *self;
                    while (result.length() < target_length)
                    {
                        result += pad_string;
                    }
                    return JsValue::make_string(result.substr(0, target_length)); },
                                               key);
            }

            // --- padStart(targetLength, padString) ---
            if (key == "padStart")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                    if (self->length() >= target_length) return JsValue::make_string(*self);
                    std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                    std::string padding;
                    while (padding.length() < target_length - self->length())
                    {
                        padding += pad_string;
                    }
                    return JsValue::make_string(padding.substr(0, target_length - self->length()) + *self); },
                                               key);
            }

            // --- repeat(count) ---
            if (key == "repeat")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    double count = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    if (count < 0) {
                        // In a real implementation, this should throw a RangeError.
                        return JsValue::make_string("");
                    }
                    std::string result = "";
                    for (int i = 0; i < count; ++i)
                    {
                        result += *self;
                    }
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- replace(substr, newSubstr) ---
            if (key == "replace")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if (args.size() < 2) return JsValue::make_string(*self);
                    std::string search = args[0].to_std_string();
                    std::string replacement = args[1].to_std_string();
                    std::string result = *self;
                    size_t pos = result.find(search);
                    if (pos != std::string::npos)
                    {
                        result.replace(pos, search.length(), replacement);
                    }
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- replaceAll(substr, newSubstr) ---
            if (key == "replaceAll")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if (args.size() < 2) return JsValue::make_string(*self);
                    std::string search = args[0].to_std_string();
                    if (search.empty()) return JsValue::make_string(*self);
                    std::string replacement = args[1].to_std_string();
                    std::string result = *self;
                    size_t pos = result.find(search);
                    while (pos != std::string::npos)
                    {
                        result.replace(pos, search.length(), replacement);
                        pos = result.find(search, pos + replacement.length());
                    }
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- slice(beginIndex, endIndex) ---
            if (key == "slice")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    int len = self->length();
                    int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                    int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                    if (start < 0) start += len;
                    if (end < 0) end += len;

                    start = std::max(0, std::min(len, start));
                    end = std::max(0, std::min(len, end));

                    if (start >= end) return JsValue::make_string("");
                    return JsValue::make_string(self->substr(start, end - start)); },
                                               key);
            }

            // --- split(separator) ---
            if (key == "split")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    std::string separator = (args.empty() || args[0].is_undefined()) ? "" : args[0].to_std_string();
                    std::vector<std::optional<JsValue>> result_vec;
                    
                    if (separator.empty()) {
                        for (char c : (*self)) {
                            result_vec.push_back(JsValue::make_string(std::string(1, c)));
                        }
                    } else {
                        std::string temp = (*self);
                        size_t pos = 0;
                        while ((pos = temp.find(separator)) != std::string::npos) {
                            result_vec.push_back(JsValue::make_string(temp.substr(0, pos)));
                            temp.erase(0, pos + separator.length());
                        }
                        result_vec.push_back(JsValue::make_string(temp));
                    }
                    return JsValue::make_array(result_vec); },
                                               key);
            }

            // --- startsWith(searchString, position) ---
            if (key == "startsWith")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    if(args.empty()) return JsValue::make_boolean(false);
                    std::string search = args[0].to_std_string();
                    size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                    if (pos > self->length()) pos = self->length();

                    return JsValue::make_boolean(self->rfind(search, pos) == pos); },
                                               key);
            }

            // --- substring(indexStart, indexEnd) ---
            if (key == "substring")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    int len = self->length();
                    int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                    int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                    start = std::max(0, start);
                    end = std::max(0, end);
                    
                    if (start > end) std::swap(start, end);

                    start = std::min(len, start);
                    end = std::min(len, end);

                    return JsValue::make_string(self->substr(start, end - start)); },
                                               key);
            }

            // --- toLowerCase() ---
            if (key == "toLowerCase")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    std::string result = *self;
                    std::transform(result.begin(), result.end(), result.begin(),
                                   [](unsigned char c){ return std::tolower(c); });
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- toUpperCase() ---
            if (key == "toUpperCase")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    std::string result = *self;
                    std::transform(result.begin(), result.end(), result.begin(),
                                   [](unsigned char c){ return std::toupper(c); });
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- trim() ---
            if (key == "trim")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    const char* whitespace = " \t\n\r\f\v";
                    std::string result = *self;
                    result.erase(0, result.find_first_not_of(whitespace));
                    result.erase(result.find_last_not_of(whitespace) + 1);
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- trimEnd() ---
            if (key == "trimEnd")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    const char* whitespace = " \t\n\r\f\v";
                    std::string result = *self;
                    result.erase(result.find_last_not_of(whitespace) + 1);
                    return JsValue::make_string(result); },
                                               key);
            }

            // --- trimStart() ---
            if (key == "trimStart")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                    const char* whitespace = " \t\n\r\f\v";
                    std::string result = *self;
                    result.erase(0, result.find_first_not_of(whitespace));
                    return JsValue::make_string(result); },
                                               key);
            }

            return std::nullopt;
        }
    }
}