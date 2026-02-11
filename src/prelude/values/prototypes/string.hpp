#pragma once

#include "types.hpp"
#include "values/string.hpp" // Make sure this is included
#include "any_value.hpp"
#include "utils/operators.hpp"
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace jspp
{
    namespace StringPrototypes
    {
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_string()->value); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_iterator_fn()
        {
            static AnyValue fn = AnyValue::make_generator([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                          { return AnyValue::from_iterator(thisVal.as_string()->get_iterator()); },
                                                          "Symbol.iterator");
            return fn;
        }

        inline AnyValue &get_length_desc()
        {
            static auto getter = [](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
            { return AnyValue::make_number(thisVal.as_string()->value.length()); };
            static auto setter = [](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
            { return Constants::UNDEFINED; };
            static AnyValue desc = AnyValue::make_accessor_descriptor(getter, setter, false, false);
            return desc;
        }

        inline AnyValue &get_charAt_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             double pos = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                             int index = static_cast<int>(pos);
                                                             if (index < 0 || index >= self->value.length())
                                                             {
                                                                 return AnyValue::make_string("");
                                                             }
                                                             return AnyValue::make_string(std::string(1, self->value[index])); },
                                                         "charAt");
            return fn;
        }

        inline AnyValue &get_concat_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             std::string result = thisVal.as_string()->value;
                                                             for (const auto &arg : args)
                                                             {
                                                                 result += arg.to_std_string();
                                                             }
                                                             return AnyValue::make_string(result); },
                                                         "concat");
            return fn;
        }

        inline AnyValue &get_endsWith_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.empty())
                                                                 return Constants::FALSE;
                                                             std::string search = args[0].to_std_string();
                                                             size_t end_pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : self->value.length();

                                                             if (end_pos > self->value.length())
                                                                 end_pos = self->value.length();
                                                             if (search.length() > end_pos)
                                                                 return Constants::FALSE;

                                                             return AnyValue::make_boolean(self->value.substr(end_pos - search.length(), search.length()) == search); },
                                                         "endsWith");
            return fn;
        }

        inline AnyValue &get_includes_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.empty())
                                                                 return Constants::FALSE;
                                                             std::string search = args[0].to_std_string();
                                                             size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;

                                                             return AnyValue::make_boolean(self->value.find(search, pos) != std::string::npos); },
                                                         "includes");
            return fn;
        }

        inline AnyValue &get_indexOf_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.empty())
                                                                 return AnyValue::make_number(-1);
                                                             std::string search = args[0].to_std_string();
                                                             size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                                                             size_t result = self->value.find(search, pos);
                                                             return result == std::string::npos ? AnyValue::make_number(-1) : AnyValue::make_number(result); },
                                                         "indexOf");
            return fn;
        }

        inline AnyValue &get_lastIndexOf_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.empty())
                                                                 return AnyValue::make_number(-1);
                                                             std::string search = args[0].to_std_string();
                                                             size_t pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : std::string::npos;
                                                             size_t result = self->value.rfind(search, pos);
                                                             return result == std::string::npos ? AnyValue::make_number(-1) : AnyValue::make_number(result); },
                                                         "lastIndexOf");
            return fn;
        }

        inline AnyValue &get_padEnd_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                                                             if (self->value.length() >= target_length)
                                                                 return AnyValue::make_string(self->value);
                                                             std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                                                             std::string result = self->value;
                                                             while (result.length() < target_length)
                                                             {
                                                                 result += pad_string;
                                                             }
                                                             return AnyValue::make_string(result.substr(0, target_length)); },
                                                         "padEnd");
            return fn;
        }

        inline AnyValue &get_padStart_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                                                             if (self->value.length() >= target_length)
                                                                 return AnyValue::make_string(self->value);
                                                             std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                                                             std::string padding;
                                                             while (padding.length() < target_length - self->value.length())
                                                             {
                                                                 padding += pad_string;
                                                             }
                                                             return AnyValue::make_string(padding.substr(0, target_length - self->value.length()) + self->value); },
                                                         "padStart");
            return fn;
        }

        inline AnyValue &get_repeat_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             double count = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                             if (count < 0)
                                                             {
                                                                 // In a real implementation, this should throw a RangeError.
                                                                 return AnyValue::make_string("");
                                                             }
                                                             std::string result = "";
                                                             for (int i = 0; i < count; ++i)
                                                             {
                                                                 result += self->value;
                                                             }
                                                             return AnyValue::make_string(result); },
                                                         "repeat");
            return fn;
        }

        inline AnyValue &get_replace_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.size() < 2)
                                                                 return AnyValue::make_string(self->value);
                                                             std::string search = args[0].to_std_string();
                                                             std::string replacement = args[1].to_std_string();
                                                             std::string result = self->value;
                                                             size_t pos = result.find(search);
                                                             if (pos != std::string::npos)
                                                             {
                                                                 result.replace(pos, search.length(), replacement);
                                                             }
                                                             return AnyValue::make_string(result); },
                                                         "replace");
            return fn;
        }

        inline AnyValue &get_replaceAll_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.size() < 2)
                                                                 return AnyValue::make_string(self->value);
                                                             std::string search = args[0].to_std_string();
                                                             if (search.empty())
                                                                 return AnyValue::make_string(self->value);
                                                             std::string replacement = args[1].to_std_string();
                                                             std::string result = self->value;
                                                             size_t pos = result.find(search);
                                                             while (pos != std::string::npos)
                                                             {
                                                                 result.replace(pos, search.length(), replacement);
                                                                 pos = result.find(search, pos + replacement.length());
                                                             }
                                                             return AnyValue::make_string(result); },
                                                         "replaceAll");
            return fn;
        }

        inline AnyValue &get_slice_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             int len = self->value.length();
                                                             int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                                                             int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                                                             if (start < 0)
                                                                 start += len;
                                                             if (end < 0)
                                                                 end += len;

                                                             start = std::max(0, std::min(len, start));
                                                             end = std::max(0, std::min(len, end));

                                                             if (start >= end)
                                                                 return AnyValue::make_string("");
                                                             return AnyValue::make_string(self->value.substr(start, end - start)); },
                                                         "slice");
            return fn;
        }

        inline AnyValue &get_split_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             std::string separator = (args.empty() || args[0].is_undefined()) ? "" : args[0].to_std_string();
                                                             std::vector<jspp::AnyValue> result_vec;

                                                             if (separator.empty())
                                                             {
                                                                 for (char c : (self->value))
                                                                 {
                                                                     result_vec.push_back(AnyValue::make_string(std::string(1, c)));
                                                                 }
                                                             }
                                                             else
                                                             {
                                                                 std::string temp = (self->value);
                                                                 size_t pos = 0;
                                                                 while ((pos = temp.find(separator)) != std::string::npos)
                                                                 {
                                                                     result_vec.push_back(AnyValue::make_string(temp.substr(0, pos)));
                                                                     temp.erase(0, pos + separator.length());
                                                                 }
                                                                 result_vec.push_back(AnyValue::make_string(temp));
                                                             }
                                                             return AnyValue::make_array(std::move(result_vec)); },
                                                         "split");
            return fn;
        }

        inline AnyValue &get_startsWith_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             if (args.empty())
                                                                 return Constants::FALSE;
                                                             std::string search = args[0].to_std_string();
                                                             size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                                                             if (pos > self->value.length())
                                                                 pos = self->value.length();

                                                             return AnyValue::make_boolean(self->value.rfind(search, pos) == pos); },
                                                         "startsWith");
            return fn;
        }

        inline AnyValue &get_substring_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_string();
                                                             int len = self->value.length();
                                                             int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                                                             int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                                                             start = std::max(0, start);
                                                             end = std::max(0, end);

                                                             if (start > end)
                                                                 std::swap(start, end);

                                                             start = std::min(len, start);
                                                             end = std::min(len, end);

                                                             return AnyValue::make_string(self->value.substr(start, end - start)); },
                                                         "substring");
            return fn;
        }

        inline AnyValue &get_toLowerCase_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             std::string result = thisVal.as_string()->value;
                                                             std::transform(result.begin(), result.end(), result.begin(),
                                                                            [](unsigned char c)
                                                                            { return std::tolower(c); });
                                                             return AnyValue::make_string(result); },
                                                         "toLowerCase");
            return fn;
        }

        inline AnyValue &get_toUpperCase_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             std::string result = thisVal.as_string()->value;
                                                             std::transform(result.begin(), result.end(), result.begin(),
                                                                            [](unsigned char c)
                                                                            { return std::toupper(c); });
                                                             return AnyValue::make_string(result); },
                                                         "toUpperCase");
            return fn;
        }

        inline AnyValue &get_trim_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             const char *whitespace = " \t\n\r\f\v";
                                                             std::string result = thisVal.as_string()->value;
                                                             result.erase(0, result.find_first_not_of(whitespace));
                                                             result.erase(result.find_last_not_of(whitespace) + 1);
                                                             return AnyValue::make_string(result); },
                                                         "trim");
            return fn;
        }

        inline AnyValue &get_trimEnd_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             const char *whitespace = " \t\n\r\f\v";
                                                             std::string result = thisVal.as_string()->value;
                                                             result.erase(result.find_last_not_of(whitespace) + 1);
                                                             return AnyValue::make_string(result); },
                                                         "trimEnd");
            return fn;
        }

        inline AnyValue &get_trimStart_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             const char *whitespace = " \t\n\r\f\v";
                                                             std::string result = thisVal.as_string()->value;
                                                             result.erase(0, result.find_first_not_of(whitespace));
                                                             return AnyValue::make_string(result); },
                                                         "trimStart");
            return fn;
        }

        // This function retrieves a prototype method for a given string instance.
        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() & valueOf() ---
            if (key == "toString" || key == "valueOf" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }

            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return get_iterator_fn();
            }

            // --- length property ---
            if (key == "length")
            {
                return get_length_desc();
            }

            // --- charAt(pos) ---
            if (key == "charAt")
            {
                return get_charAt_fn();
            }

            // --- concat(str1, str2, ...) ---
            if (key == "concat")
            {
                return get_concat_fn();
            }

            // --- endsWith(searchString, endPosition) ---
            if (key == "endsWith")
            {
                return get_endsWith_fn();
            }

            // --- includes(searchString, position) ---
            if (key == "includes")
            {
                return get_includes_fn();
            }

            // --- indexOf(searchString, position) ---
            if (key == "indexOf")
            {
                return get_indexOf_fn();
            }

            // --- lastIndexOf(searchString, position) ---
            if (key == "lastIndexOf")
            {
                return get_lastIndexOf_fn();
            }

            // --- padEnd(targetLength, padString) ---
            if (key == "padEnd")
            {
                return get_padEnd_fn();
            }

            // --- padStart(targetLength, padString) ---
            if (key == "padStart")
            {
                return get_padStart_fn();
            }

            // --- repeat(count) ---
            if (key == "repeat")
            {
                return get_repeat_fn();
            }

            // --- replace(substr, newSubstr) ---
            if (key == "replace")
            {
                return get_replace_fn();
            }

            // --- replaceAll(substr, newSubstr) ---
            if (key == "replaceAll")
            {
                return get_replaceAll_fn();
            }

            // --- slice(beginIndex, endIndex) ---
            if (key == "slice")
            {
                return get_slice_fn();
            }

            // --- split(separator) ---
            if (key == "split")
            {
                return get_split_fn();
            }

            // --- startsWith(searchString, position) ---
            if (key == "startsWith")
            {
                return get_startsWith_fn();
            }

            // --- substring(indexStart, indexEnd) ---
            if (key == "substring")
            {
                return get_substring_fn();
            }

            // --- toLowerCase() ---
            if (key == "toLowerCase")
            {
                return get_toLowerCase_fn();
            }

            // --- toUpperCase() ---
            if (key == "toUpperCase")
            {
                return get_toUpperCase_fn();
            }

            // --- trim() ---
            if (key == "trim")
            {
                return get_trim_fn();
            }

            // --- trimEnd() ---
            if (key == "trimEnd")
            {
                return get_trimEnd_fn();
            }

            // --- trimStart() ---
            if (key == "trimStart")
            {
                return get_trimStart_fn();
            }

            return std::nullopt;
        }
    }
}
