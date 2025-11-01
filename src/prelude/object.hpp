#pragma once

#include "types.hpp"
#include "prototype.hpp"
#include "convert.hpp"
#include "exception.hpp"

namespace jspp
{
    namespace Object
    {
        inline std::shared_ptr<jspp::JsObject> make_object(const std::map<std::string, JsValue> &properties);
        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<JsValue> &properties);
        inline std::shared_ptr<jspp::JsFunction> make_function(const std::function<JsValue(const std::vector<JsValue> &)> &callable);
        inline std::shared_ptr<jspp::JsString> make_string(const std::string &value);
        inline std::shared_ptr<jspp::JsBoolean> make_boolean(const bool &value);

        inline std::shared_ptr<jspp::JsObject> make_object(const std::map<std::string, JsValue> &properties)
        {

            auto object = std::make_shared<jspp::JsObject>(jspp::JsObject{properties, {}});
            // Define and set prototype methods
            Prototype::set_data_property(
                object->prototype,
                "toString",
                std::function<jspp::JsValue()>([]() -> jspp::JsValue
                                               { return "[object Object]"; }));
            // return object shared pointer
            return object;
        }

        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<JsValue> &properties)
        {
            auto array = std::make_shared<jspp::JsArray>(jspp::JsArray{properties, {}});
            // Define and set prototype methods
            Prototype::set_data_property(
                array->prototype,
                "toString",
                std::function<jspp::JsValue()>([array]() mutable -> jspp::JsValue
                                               {
                std::string str = "[";
                for (size_t i = 0; i < array->items.size(); ++i)
                {
                    str += jspp::Convert::to_string(array->items[i]);
                    if (i < array->items.size() - 1)
                        str += ", ";
                }
                str += "]";
                return str; }));
            Prototype::set_accessor_property(
                array->prototype,
                "length",
                Prototype::to_handler(std::function<jspp::JsValue()>([array]() mutable -> jspp::JsValue
                                                                     { return (int)array->items.size(); })),
                Prototype::to_handler(std::function<jspp::JsValue(jspp::JsValue)>([array](auto val) mutable -> jspp::JsValue
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
                                                                array->items.resize(new_length, undefined);
                                                                return val; })));
            // return object shared pointer
            return array;
        }

        inline std::shared_ptr<jspp::JsFunction> make_function(const std::function<JsValue(const std::vector<JsValue> &)> &callable)
        {
            auto func = std::make_shared<jspp::JsFunction>(jspp::JsFunction{callable, {}});
            Prototype::set_data_property(
                func->prototype,
                "toString",
                std::function<jspp::JsValue()>([]() mutable -> jspp::JsValue
                                               { return "function(){}"; }));
            return func;
        }

        inline std::shared_ptr<jspp::JsString> make_string(const std::string &value)
        {
            auto str_obj = std::make_shared<jspp::JsString>(jspp::JsString{value, {}});
            // Define and set prototype methods and accessors
            // Accessors
            Prototype::set_accessor_property(
                str_obj->prototype,
                "length",
                Prototype::to_handler(std::function<jspp::JsValue()>([str_obj]() mutable -> jspp::JsValue
                                                                     { return (int)str_obj->value.length(); })),
                undefined // length is read-only for now
            );
            // Methods
            Prototype::set_data_property(
                str_obj->prototype,
                "toString",
                std::function<jspp::JsValue()>([str_obj]() mutable -> jspp::JsValue
                                               { return str_obj->value; }));
            Prototype::set_data_property(
                str_obj->prototype,
                "charAt",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return "";
                        }
                        int index = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            index = std::any_cast<int>(unwrapped_val);
                        }
                        if (index < 0 || index >= (int)str_obj->value.length())
                        {
                            return "";
                        }
                        return std::string(1, str_obj->value[index]);
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "concat",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        for (const auto &arg : args)
                        {
                            result += Convert::to_string(arg);
                        }
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "endsWith",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return false;
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        int endPosition = str_obj->value.length();
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            endPosition = std::any_cast<int>(args[1]);
                        }
                        int start = std::min(endPosition, (int)str_obj->value.length()) - searchString.length();
                        if (start < 0)
                        {
                            return false;
                        }
                        return str_obj->value.substr(start, searchString.length()) == searchString;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "includes",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return false;
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        int position = 0;
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            position = std::any_cast<int>(args[1]);
                        }
                        return str_obj->value.find(searchString, position) != std::string::npos;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "indexOf",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return -1;
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        int position = 0;
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            position = std::any_cast<int>(args[1]);
                        }
                        size_t index = str_obj->value.find(searchString, position);
                        return index == std::string::npos ? -1 : (int)index;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "lastIndexOf",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return -1;
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        int position = str_obj->value.length();
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            position = std::any_cast<int>(args[1]);
                        }
                        size_t index = str_obj->value.rfind(searchString, position);
                        return index == std::string::npos ? -1 : (int)index;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "padEnd",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return str_obj->value;
                        }
                        int targetLength = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            targetLength = std::any_cast<int>(unwrapped_val);
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if ((int)str_obj->value.length() >= targetLength)
                        {
                            return str_obj->value;
                        }
                        std::string result = str_obj->value;
                        size_t padLen = targetLength - result.length();
                        for (size_t i = 0; i < padLen; ++i)
                        {
                            result += padString[i % padString.length()];
                        }
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "padStart",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return str_obj->value;
                        }
                        int targetLength = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            targetLength = std::any_cast<int>(unwrapped_val);
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if ((int)str_obj->value.length() >= targetLength)
                        {
                            return str_obj->value;
                        }
                        std::string result = str_obj->value;
                        size_t padLen = targetLength - result.length();
                        std::string padding = "";
                        for (size_t i = 0; i < padLen; ++i)
                        {
                            padding += padString[i % padString.length()];
                        }
                        return padding + result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "repeat",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return "";
                        }
                        int count = 0;
                        auto unwrapped_val = jspp::Convert::unwrap_number(args[0]);
                        if (unwrapped_val.type() == typeid(int))
                        {
                            count = std::any_cast<int>(unwrapped_val);
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
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "replace",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.size() < 2)
                        {
                            return str_obj->value;
                        }
                        std::string searchValue = Convert::to_string(args[0]);
                        std::string replaceValue = Convert::to_string(args[1]);
                        std::string result = str_obj->value;
                        size_t pos = result.find(searchValue);
                        if (pos != std::string::npos)
                        {
                            result.replace(pos, searchValue.length(), replaceValue);
                        }
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "replaceAll",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.size() < 2)
                        {
                            return str_obj->value;
                        }
                        std::string searchValue = Convert::to_string(args[0]);
                        std::string replaceValue = Convert::to_string(args[1]);
                        std::string result = str_obj->value;
                        size_t pos = 0;
                        while ((pos = result.find(searchValue, pos)) != std::string::npos)
                        {
                            result.replace(pos, searchValue.length(), replaceValue);
                            pos += replaceValue.length();
                        }
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "slice",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                        return str_obj->value.substr(start, end - start);
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "split",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string separator = "";
                        if (!args.empty())
                        {
                            separator = Convert::to_string(args[0]);
                        }
                        std::vector<JsValue> result;
                        if (separator.empty())
                        {
                            for (char c : str_obj->value)
                            {
                                result.push_back(std::string(1, c));
                            }
                        }
                        else
                        {
                            size_t start = 0;
                            size_t end = str_obj->value.find(separator);
                            while (end != std::string::npos)
                            {
                                result.push_back(str_obj->value.substr(start, end - start));
                                start = end + separator.length();
                                end = str_obj->value.find(separator, start);
                            }
                            result.push_back(str_obj->value.substr(start));
                        }
                        return Object::make_array(result);
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "startsWith",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return false;
                        }
                        std::string searchString = Convert::to_string(args[0]);
                        int position = 0;
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            position = std::any_cast<int>(args[1]);
                        }
                        return str_obj->value.rfind(searchString, position) == position;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "substring",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                        if (start > end)
                        {
                            std::swap(start, end);
                        }
                        start = std::max(0, start);
                        end = std::min((int)str_obj->value.length(), end);
                        return str_obj->value.substr(start, end - start);
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "toLowerCase",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        std::transform(result.begin(), result.end(), result.begin(),
                                       [](unsigned char c)
                                       { return std::tolower(c); });
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "toUpperCase",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        std::transform(result.begin(), result.end(), result.begin(),
                                       [](unsigned char c)
                                       { return std::toupper(c); });
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "trim",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
                        result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "trimEnd",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "trimStart",
                Object::make_function(
                    [str_obj](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
                        return result;
                    }));
            // return object shared pointer
            return str_obj;
        }

        inline std::shared_ptr<jspp::JsNumber> make_number(const JsNumberValue &value)
        {
            auto num_obj = std::make_shared<jspp::JsNumber>(jspp::JsNumber{value, {}});
            // Define and set prototype methods
            Prototype::set_data_property(
                num_obj->prototype,
                "toString",
                std::function<jspp::JsValue()>([num_obj]() mutable -> jspp::JsValue
                                               { return jspp::Convert::to_string(num_obj->value); }));
            // return object shared pointer
            return num_obj;
        }

        inline std::shared_ptr<jspp::JsBoolean> make_boolean(const bool &value)
        {
            auto bool_obj = std::make_shared<jspp::JsBoolean>(jspp::JsBoolean{value, {}});
            // Define and set prototype methods
            Prototype::set_data_property(
                bool_obj->prototype,
                "toString",
                std::function<jspp::JsValue()>([bool_obj]() mutable -> jspp::JsValue
                                               { return jspp::Convert::to_string(bool_obj->value); }));
            // return object shared pointer
            return bool_obj;
        }

    }
}
