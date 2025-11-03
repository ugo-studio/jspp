#pragma once

#include "types.hpp"
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

        inline std::optional<PrototypeProperty> get_string_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &str_obj = std::any_cast<const std::shared_ptr<JsString> &>(obj);
            if (key_str == "length")
            {
                return AccessorDescriptor{
                    PrototypeDefaults::to_handler(std::function<AnyValue()>([str_obj]() mutable
                                                                            { return Object::make_number((int)str_obj->value.length()); })),
                    undefined,
                    false,
                    true};
            }
            if (key_str == "toString" || key_str == WellKnownSymbols::toString)
            {
                return DataDescriptor{Object::make_function([str_obj](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string(str_obj->value); })};
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
                            start = str_obj->value.length() + start;
                        }
                        if (end < 0)
                        {
                            end = str_obj->value.length() + end;
                        }
                        return Object::make_string(str_obj->value.substr(start, end - start));
                    })};
            }
            return std::nullopt;
        };

        inline std::optional<PrototypeProperty> get_number_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &num_obj = std::any_cast<const std::shared_ptr<JsNumber> &>(obj);
            if (key_str == "toString")
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

        inline std::optional<PrototypeProperty> get_boolean_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &bool_obj = std::any_cast<const std::shared_ptr<JsBoolean> &>(obj);
            if (key_str == "toString")
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

        inline std::optional<PrototypeProperty> get_object_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &obj_obj = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
            if (key_str == "toString")
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

        inline std::optional<PrototypeProperty> get_function_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &func_obj = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
            if (key_str == "toString")
            {
                return DataDescriptor{Object::make_function([](const std::vector<AnyValue> &_) mutable -> jspp::AnyValue
                                                            { return Object::make_string("function () { [native code] }"); })};
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

        inline std::optional<PrototypeProperty> get_array_prototye(const AnyValue &obj, const std::string &key_str)
        {
            auto &arr_obj = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
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
            if (key_str == "toString")
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

    }
}