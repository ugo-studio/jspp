#pragma once

#include "types.hpp"
#include "exception.hpp"
#include "object.hpp"
#include "convert.hpp"
#include "well_known_symbols.hpp"
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <functional>

namespace jspp
{
    namespace PrototypeDefaults
    {
        using PrototypeProperty = std::variant<DataDescriptor, AccessorDescriptor>;

        inline std::function<AnyValue(const std::vector<AnyValue> &)> to_handler(std::function<AnyValue()> fn)
        {
            return [fn = std::move(fn)](const std::vector<AnyValue> &)
            {
                return fn();
            };
        }

        inline std::function<AnyValue(const std::vector<AnyValue> &)> to_handler(std::function<AnyValue(AnyValue)> fn)
        {
            return [fn = std::move(fn)](const std::vector<AnyValue> &args)
            {
                return fn(args.empty() ? undefined : args[0]);
            };
        }

        inline std::optional<PrototypeProperty> string_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &str_obj = std::any_cast<const std::shared_ptr<JsString> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function([str_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string(str_obj->value); })};
            }
            if (key_str == "length")
            {
                return AccessorDescriptor{
                    PrototypeDefaults::to_handler(std::function<AnyValue()>([str_obj]() mutable
                                                                            { return Object::make_number((int)str_obj->value.length()); })),
                    undefined,
                    false,
                    true};
            }
            if (key_str == "valueOf")
            {
                return DataDescriptor{Object::make_function([str_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string(str_obj->value); })};
            }
            if (key_str == "charAt")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_string("");
                        }
                        int index = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            index = std::any_cast<int>(unwrapped_val);
                        }
                        if (index < 0 || index >= (int)str_obj->value.length())
                        {
                            return Object::make_string("");
                        }
                        return Object::make_string(std::string(1, str_obj->value[index]));
                    })};
            }
            if (key_str == "concat")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        std::string result = str_obj->value;
                        for (const auto &arg : args)
                        {
                            result += Convert::to_string(arg);
                        }
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "includes")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_boolean(false);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        return Object::make_boolean(str_obj->value.find(searchString) != std::string::npos);
                    })};
            }
            if (key_str == "indexOf")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_number(-1);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t pos = 0;
                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                pos = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        size_t found = str_obj->value.find(searchString, pos);
                        if (found == std::string::npos)
                        {
                            return Object::make_number(-1);
                        }
                        return Object::make_number((int)found);
                    })};
            }
            if (key_str == "lastIndexOf")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_number(-1);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t pos = std::string::npos;
                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                pos = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        size_t found = str_obj->value.rfind(searchString, pos);
                        if (found == std::string::npos)
                        {
                            return Object::make_number(-1);
                        }
                        return Object::make_number((int)found);
                    })};
            }
            if (key_str == "padEnd")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        int targetLength = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                targetLength = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if (str_obj->value.length() >= targetLength)
                        {
                            return Object::make_string(str_obj->value);
                        }
                        std::string result = str_obj->value;
                        while (result.length() < targetLength)
                        {
                            result += padString;
                        }
                        return Object::make_string(result.substr(0, targetLength));
                    })};
            }
            if (key_str == "padStart")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        int targetLength = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                targetLength = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if (str_obj->value.length() >= targetLength)
                        {
                            return Object::make_string(str_obj->value);
                        }
                        std::string result = padString;
                        while (result.length() < targetLength - str_obj->value.length())
                        {
                            result += padString;
                        }
                        return Object::make_string(result.substr(0, targetLength - str_obj->value.length()) + str_obj->value);
                    })};
            }
            if (key_str == "repeat")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        int count = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                count = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        if (count < 0)
                        {
                            throw Exception::make_error_with_name("Invalid count value", "RangeError");
                        }
                        std::string result = "";
                        for (int i = 0; i < count; ++i)
                        {
                            result += str_obj->value;
                        }
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "replace")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_string(str_obj->value);
                        }
                        std::string search = Convert::to_string(args[0]);
                        std::string replace = "";
                        if (args.size() > 1)
                        {
                            replace = Convert::to_string(args[1]);
                        }
                        size_t pos = str_obj->value.find(search);
                        if (pos == std::string::npos)
                        {
                            return Object::make_string(str_obj->value);
                        }
                        std::string result = str_obj->value;
                        result.replace(pos, search.length(), replace);
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "replaceAll")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_string(str_obj->value);
                        }
                        std::string search = Convert::to_string(args[0]);
                        std::string replace = "";
                        if (args.size() > 1)
                        {
                            replace = Convert::to_string(args[1]);
                        }
                        std::string result = str_obj->value;
                        size_t pos = 0;
                        while ((pos = result.find(search, pos)) != std::string::npos)
                        {
                            result.replace(pos, search.length(), replace);
                            pos += replace.length();
                        }
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "split")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        std::string separator = "";
                        if (!args.empty())
                        {
                            separator = Convert::to_string(args[0]);
                        }
                        std::vector<AnyValue> result;
                        std::string s = str_obj->value;
                        size_t pos = 0;
                        std::string token;
                        while ((pos = s.find(separator)) != std::string::npos)
                        {
                            token = s.substr(0, pos);
                            result.push_back(Object::make_string(token));
                            s.erase(0, pos + separator.length());
                        }
                        result.push_back(Object::make_string(s));
                        return Object::make_array(result);
                    })};
            }
            if (key_str == "startsWith")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_boolean(false);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        return Object::make_boolean(str_obj->value.rfind(searchString, 0) == 0);
                    })};
            }
            if (key_str == "substring")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        int start = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                start = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        int end = str_obj->value.length();
                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                end = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        start = std::max(0, std::min((int)str_obj->value.length(), start));
                        end = std::max(0, std::min((int)str_obj->value.length(), end));

                        if (start > end)
                        {
                            std::swap(start, end);
                        }
                        return Object::make_string(str_obj->value.substr(start, end - start));
                    })};
            }
            if (key_str == "toLowerCase")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        std::transform(s.begin(), s.end(), s.begin(),
                                       [](unsigned char c)
                                       { return std::tolower(c); });
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "toUpperCase")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        std::transform(s.begin(), s.end(), s.begin(),
                                       [](unsigned char c)
                                       { return std::toupper(c); });
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "trim")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
                                                        { return !std::isspace(ch); }));
                        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
                                             { return !std::isspace(ch); })
                                    .base(),
                                s.end());
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "trimEnd")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
                                             { return !std::isspace(ch); })
                                    .base(),
                                s.end());
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "trimStart")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
                                                        { return !std::isspace(ch); }));
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "startsWith")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_boolean(false);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t pos = 0;
                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                int p = std::any_cast<int>(unwrapped_val);
                                if (p > 0)
                                {
                                    pos = static_cast<size_t>(p);
                                }
                            }
                        }

                        if (pos + searchString.length() > str_obj->value.length())
                        {
                            return Object::make_boolean(false);
                        }

                        return Object::make_boolean(str_obj->value.compare(pos, searchString.length(), searchString) == 0);
                    })};
            }
            if (key_str == "endsWith")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_boolean(false);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t len = str_obj->value.length();
                        size_t end_pos = len;

                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                int p = std::any_cast<int>(unwrapped_val);
                                end_pos = static_cast<size_t>(p);
                            }
                        }

                        size_t effective_len = std::min(end_pos, len);
                        if (searchString.length() > effective_len)
                        {
                            return Object::make_boolean(false);
                        }

                        size_t start_pos = effective_len - searchString.length();
                        return Object::make_boolean(str_obj->value.compare(start_pos, searchString.length(), searchString) == 0);
                    })};
            }
            if (key_str == "at")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return undefined;
                        }
                        int index = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            index = std::any_cast<int>(unwrapped_val);
                        }

                        if (index < 0)
                        {
                            index += str_obj->value.length();
                        }

                        if (index < 0 || index >= (int)str_obj->value.length())
                        {
                            return undefined;
                        }
                        return Object::make_string(std::string(1, str_obj->value[index]));
                    })};
            }
            if (key_str == "slice")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        int start = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                start = std::any_cast<int>(unwrapped_val);
                            }
                        }

                        int end = str_obj->value.length();
                        if (args.size() > 1)
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[1]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                end = std::any_cast<int>(unwrapped_val);
                            }
                        }

                        if (start < 0)
                        {
                            start += str_obj->value.length();
                        }
                        if (end < 0)
                        {
                            end += str_obj->value.length();
                        }

                        start = std::max(0, std::min((int)str_obj->value.length(), start));
                        end = std::max(0, std::min((int)str_obj->value.length(), end));

                        if (start >= end)
                        {
                            return Object::make_string("");
                        }

                        return Object::make_string(str_obj->value.substr(start, end - start));
                    })};
            }
            if (key_str == "search")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_number(0);
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t found = str_obj->value.find(searchString);
                        if (found == std::string::npos)
                        {
                            return Object::make_number(-1);
                        }
                        return Object::make_number((int)found);
                    })};
            }
            if (key_str == "match")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_array({Object::make_string("")});
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        size_t found = str_obj->value.find(searchString);
                        if (found == std::string::npos)
                        {
                            return null;
                        }
                        return Object::make_array({Object::make_string(searchString)});
                    })};
            }
            if (key_str == "matchAll")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_array({});
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        std::vector<AnyValue> matches;
                        size_t pos = 0;
                        while ((pos = str_obj->value.find(searchString, pos)) != std::string::npos)
                        {
                            matches.push_back(Object::make_array({Object::make_string(searchString)}));
                            pos += searchString.length();
                        }
                        return Object::make_array(matches);
                    })};
            }
            if (key_str == "trimLeft")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
                                                        { return !std::isspace(ch); }));
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "trimRight")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
                                             { return !std::isspace(ch); })
                                    .base(),
                                s.end());
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "toLocaleLowerCase")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        std::transform(s.begin(), s.end(), s.begin(),
                                       [](unsigned char c)
                                       { return std::tolower(c); });
                        return Object::make_string(s);
                    })};
            }
            if (key_str == "toLocaleUpperCase")
            {
                return DataDescriptor{Object::make_function(
                    [str_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        std::string s = str_obj->value;
                        std::transform(s.begin(), s.end(), s.begin(),
                                       [](unsigned char c)
                                       { return std::toupper(c); });
                        return Object::make_string(s);
                    })};
            }

            return std::nullopt;
        };

        inline std::optional<PrototypeProperty> number_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &num_obj = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function([num_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string(Convert::to_string(num_obj->value)); })};
            }
            if (key_str == "valueOf")
            {
                return DataDescriptor{Object::make_function([num_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_number(num_obj->value); })};
            }
            if (key_str == "toFixed")
            {
                return DataDescriptor{Object::make_function(
                    [num_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        double num_val = std::holds_alternative<int>(num_obj->value) ? static_cast<double>(std::get<int>(num_obj->value)) : std::get<double>(num_obj->value);
                        int digits = 0;
                        if (!args.empty())
                        {
                            auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                            if (unwrapped_val.type() == typeid(int))
                            {
                                digits = std::any_cast<int>(unwrapped_val);
                            }
                        }
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(digits) << num_val;
                        return Object::make_string(ss.str());
                    })};
            }
            return std::nullopt;
        }

        inline std::optional<PrototypeProperty> boolean_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &bool_obj = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function([bool_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string(bool_obj->value ? "true" : "false"); })};
            }
            if (key_str == "valueOf")
            {
                return DataDescriptor{Object::make_function([bool_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_boolean(bool_obj->value); })};
            }
            return std::nullopt;
        }

        inline std::optional<PrototypeProperty> object_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &obj_obj = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function([](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string("[object Object]"); })};
            }
            if (key_str == "hasOwnProperty")
            {
                return DataDescriptor{Object::make_function(
                    [obj_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty())
                        {
                            return Object::make_boolean(false);
                        }
                        std::string prop = Convert::to_string(args[0]);
                        return Object::make_boolean(obj_obj->properties.count(prop) > 0);
                    })};
            }
            return std::nullopt;
        }

        inline std::optional<PrototypeProperty> array_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &arr_obj = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        // This is equivalent to join()
                        std::string result = "";
                        for (size_t i = 0; i < arr_obj->items.size(); ++i)
                        {
                            if (arr_obj->items[i].type() != typeid(Undefined) && arr_obj->items[i].type() != typeid(Null))
                            {
                                result += Convert::to_string(arr_obj->items[i]);
                            }
                            if (i < arr_obj->items.size() - 1)
                            {
                                result += ",";
                            }
                        }
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "length")
            {
                return AccessorDescriptor{
                    PrototypeDefaults::to_handler(std::function<AnyValue()>([arr_obj]() mutable
                                                                            { return Object::make_number((int)arr_obj->items.size()); })),
                    PrototypeDefaults::to_handler(std::function<jspp::AnyValue(jspp::AnyValue)>([arr_obj](auto val) mutable -> jspp::AnyValue
                                                                                                {
                                                                auto unwrapped_val = jspp::Convert::unwrap_number(val);
                                                                size_t new_length = 0;
                                                                if (unwrapped_val.type() == typeid(int))
                                                                {
                                                                    int v = std::any_cast<int>(unwrapped_val);
                                                                    if (v < 0)
                                                                    {
                                                                        throw Exception::make_error_with_name("Invalid array length", "RangeError");
                                                                    }
                                                                    new_length = static_cast<size_t>(v);
                                                                }
                                                                else if (unwrapped_val.type() == typeid(double))
                                                                {
                                                                    double v = std::any_cast<double>(unwrapped_val);
                                                                    if (v < 0 || v != static_cast<int>(v))
                                                                    {
                                                                        throw Exception::make_error_with_name("Invalid array length", "RangeError");
                                                                    }
                                                                    new_length = static_cast<size_t>(static_cast<int>(v));
                                                                }
                                                                else
                                                                {
                                                                    // Other types could be converted to number in a more complete implementation
                                                                    return val;
                                                                }
                                                                arr_obj->items.resize(new_length, undefined);
                                                                return val; })),
                    false,
                    true};
            }
            if (key_str == "push")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        for (const auto &arg : args)
                        {
                            arr_obj->items.push_back(arg);
                        }
                        return Object::make_number((int)arr_obj->items.size());
                    })};
            }
            if (key_str == "pop")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj](const std::vector<AnyValue> &) mutable -> jspp::AnyValue
                    {
                        if (arr_obj->items.empty())
                        {
                            return undefined;
                        }
                        AnyValue val = arr_obj->items.back();
                        arr_obj->items.pop_back();
                        return val;
                    })};
            }
            if (key_str == "join")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        std::string separator = ",";
                        if (!args.empty())
                        {
                            separator = Convert::to_string(args[0]);
                        }
                        std::string result = "";
                        for (size_t i = 0; i < arr_obj->items.size(); ++i)
                        {
                            if (arr_obj->items[i].type() != typeid(Undefined) && arr_obj->items[i].type() != typeid(Null))
                            {
                                result += Convert::to_string(arr_obj->items[i]);
                            }
                            if (i < arr_obj->items.size() - 1)
                            {
                                result += separator;
                            }
                        }
                        return Object::make_string(result);
                    })};
            }
            if (key_str == "map")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj, obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty() || args[0].type() != typeid(std::shared_ptr<JsFunction>))
                        {
                            throw Exception::make_error_with_name("callback is not a function", "TypeError");
                        }
                        auto &callback = std::any_cast<const std::shared_ptr<JsFunction> &>(args[0]);
                        auto new_arr = Object::make_array({});
                        for (size_t i = 0; i < arr_obj->items.size(); ++i)
                        {
                            new_arr->items.push_back(callback->call({arr_obj->items[i], Object::make_number((int)i), obj}));
                        }
                        return new_arr;
                    })};
            }
            if (key_str == "filter")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj, obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty() || args[0].type() != typeid(std::shared_ptr<JsFunction>))
                        {
                            throw Exception::make_error_with_name("callback is not a function", "TypeError");
                        }
                        auto &callback = std::any_cast<const std::shared_ptr<JsFunction> &>(args[0]);
                        auto new_arr = Object::make_array({});
                        for (size_t i = 0; i < arr_obj->items.size(); ++i)
                        {
                            if (is_truthy(callback->call({arr_obj->items[i], Object::make_number((int)i), obj})))
                            {
                                new_arr->items.push_back(arr_obj->items[i]);
                            }
                        }
                        return new_arr;
                    })};
            }
            if (key_str == "reduce")
            {
                return DataDescriptor{Object::make_function(
                    [arr_obj, obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        if (args.empty() || args[0].type() != typeid(std::shared_ptr<JsFunction>))
                        {
                            throw Exception::make_error_with_name("callback is not a function", "TypeError");
                        }
                        auto &callback = std::any_cast<const std::shared_ptr<JsFunction> &>(args[0]);
                        AnyValue accumulator;
                        size_t start_index = 0;
                        if (args.size() > 1)
                        {
                            accumulator = args[1];
                        }
                        else
                        {
                            if (arr_obj->items.empty())
                            {
                                throw Exception::make_error_with_name("Reduce of empty array with no initial value", "TypeError");
                            }
                            accumulator = arr_obj->items[0];
                            start_index = 1;
                        }
                        for (size_t i = start_index; i < arr_obj->items.size(); ++i)
                        {
                            accumulator = callback->call({accumulator, arr_obj->items[i], Object::make_number((int)i), obj});
                        }
                        return accumulator;
                    })};
            }
            return std::nullopt;
        }

        inline std::optional<PrototypeProperty> function_prototype(const AnyValue &obj, const std::string &key_str)
        {
            auto &func_obj = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
            if (key_str == WellKnownSymbols::toString || key_str == "toString")
            {
                return DataDescriptor{Object::make_function([func_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string("function " + func_obj->name + "() { [native code] }"); })};
            }
            if (key_str == "name")
            {
                return AccessorDescriptor{
                    PrototypeDefaults::to_handler(std::function<AnyValue()>([func_obj]() mutable
                                                                            { return Object::make_string(func_obj->name); })),
                    undefined, // name cannot be changed
                    false,
                    true};
            }
            if (key_str == "call")
            {
                return DataDescriptor{Object::make_function(
                    [func_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        // NOTE: `this` context is not handled yet.
                        std::vector<AnyValue> call_args;
                        if (args.size() > 1)
                        {
                            call_args.assign(args.begin() + 1, args.end());
                        }
                        return func_obj->call(call_args);
                    })};
            }
            if (key_str == "apply")
            {
                return DataDescriptor{Object::make_function(
                    [func_obj](const std::vector<AnyValue> &args) mutable -> jspp::AnyValue
                    {
                        // NOTE: `this` context is not handled yet.
                        if (args.size() > 1)
                        {
                            auto &arr_like = args[1];
                            if (arr_like.type() == typeid(std::shared_ptr<JsArray>))
                            {
                                auto &arr = std::any_cast<const std::shared_ptr<JsArray> &>(arr_like);
                                return func_obj->call(arr->items);
                            }
                        }
                        return func_obj->call({});
                    })};
            }
            return std::nullopt;
        }
    }
}