#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>
#include <memory>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <cmath>

struct Undefined
{
};
inline Undefined undefined;
struct Null
{
};
inline Null null;

// JSPP standard library
namespace jspp
{
    // Dynamic JsValue
    using JsValue = std::any;

    inline JsValue pow(const JsValue &lhs, const JsValue &rhs);

    // Temporal Dead Zone
    struct Uninitialized
    {
    };
    inline constexpr Uninitialized uninitialized;

    // Object and array prototypes
    struct DataDescriptor
    {
        JsValue value = undefined;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };
    struct AccessorDescriptor
    {
        std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> get = undefined; // getter
        std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> set = undefined; // setter
        bool enumerable = false;
        bool configurable = true;
    };

    // Objects
    struct JsObject
    {
        std::map<std::string, JsValue> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Arrays
    struct JsArray
    {
        std::vector<JsValue> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Strings
    struct JsString
    {
        std::string value;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Functions
    struct JsFunction
    {
        std::function<JsValue(const std::vector<JsValue> &)> call;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    namespace Convert
    {
        inline std::string to_string(const JsValue &val)
        {
            if (!val.has_value())
                return "undefined";
            if (val.type() == typeid(Uninitialized))
                return "<uninitialized>"; // This should ideally not be returned if the TDZ logic is correct
            if (val.type() == typeid(std::string))
                return std::any_cast<std::string>(val);
            if (val.type() == typeid(const char *))
                return std::any_cast<const char *>(val);
            if (val.type() == typeid(int))
                return std::to_string(std::any_cast<int>(val));
            if (val.type() == typeid(double))
                return std::to_string(std::any_cast<double>(val));
            if (val.type() == typeid(bool))
                return std::any_cast<bool>(val) ? "true" : "false";
            if (val.type() == typeid(Null))
                return "null";
            if (val.type() == typeid(Undefined))
                return "undefined";
            if (val.type() == typeid(std::shared_ptr<jspp::JsObject>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsObject>>(val);
                const auto it = ptr->properties.find("toString");
                if (it != ptr->properties.end())
                {
                    const auto &prop = it->second;
                    if (prop.type() == typeid(std::function<JsValue()>))
                    {
                        auto result = std::any_cast<std::function<JsValue()>>(prop)();
                        return jspp::Convert::to_string(result);
                    }
                }
                const auto proto_it = ptr->prototype.find("toString");
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto d = std::get<DataDescriptor>(prop);
                        if (d.value.type() == typeid(std::function<JsValue()>))
                        {
                            auto s = std::any_cast<std::function<JsValue()>>(d.value)();
                            return jspp::Convert::to_string(s);
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            auto result = std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                            return jspp::Convert::to_string(result);
                        }
                    }
                }
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsArray>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsArray>>(val);
                const auto proto_it = ptr->prototype.find("toString");
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto d = std::get<DataDescriptor>(prop);
                        if (d.value.type() == typeid(std::function<JsValue()>))
                        {
                            auto s = std::any_cast<std::function<JsValue()>>(d.value)();
                            return jspp::Convert::to_string(s);
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            auto result = std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                            return jspp::Convert::to_string(result);
                        }
                    }
                }
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsString>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsString>>(val);
                return ptr->value;
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsFunction>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsFunction>>(val);
                const auto proto_it = ptr->prototype.find("toString");
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto d = std::get<DataDescriptor>(prop);
                        if (d.value.type() == typeid(std::function<JsValue()>))
                        {
                            auto s = std::any_cast<std::function<JsValue()>>(d.value)();
                            return jspp::Convert::to_string(s);
                        }
                    }
                }
            }
            return "[Object Object]";
        }
    }

    namespace Prototype
    {
        using PrototypeMap = std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>>;

        inline std::function<JsValue(const std::vector<JsValue> &)> to_handler(std::function<JsValue()> fn)
        {
            return [fn = std::move(fn)](const std::vector<JsValue> &)
            {
                return fn();
            };
        }

        inline std::function<JsValue(const std::vector<JsValue> &)> to_handler(std::function<JsValue(JsValue)> fn)
        {
            return [fn = std::move(fn)](const std::vector<JsValue> &args)
            {
                return fn(args.empty() ? undefined : args[0]);
            };
        }

        inline void set_data_property(
            PrototypeMap &prototype,
            const std::string &name,
            const JsValue &value,
            bool writable = true,
            bool enumerable = false,
            bool configurable = true)
        {
            prototype[name] = DataDescriptor{value, writable, enumerable, configurable};
        }

        inline void set_accessor_property(
            PrototypeMap &prototype,
            const std::string &name,
            const std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> &getter,
            const std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> &setter,
            bool enumerable = false,
            bool configurable = true)
        {
            prototype[name] = AccessorDescriptor{getter, setter, enumerable, configurable};
        }
    }

    namespace Exception
    {
        inline jspp::JsValue make_error_with_name(const JsValue &message, const JsValue &name)
        {
            auto error = std::make_shared<JsObject>(JsObject{{{"message", message}, {"name", name}}});
            // Define and set prototype methods
            Prototype::set_data_property(
                error->prototype,
                "toString",
                std::function<jspp::JsValue()>([=]() -> jspp::JsValue
                                               {  
                        std::string name_str = "Error";
                        if (error->properties.count("name") > 0) {
                            if (error->properties["name"].type() == typeid(std::string)) {
                                name_str = std::any_cast<std::string>(error->properties["name"]);
                            } else if (error->properties["name"].type() == typeid(const char *)) {
                                name_str = std::any_cast<const char *>(error->properties["name"]);
                            } else if (error->properties["name"].type() == typeid(jspp::JsString)) {
                                name_str = std::any_cast<std::string>(std::any_cast<jspp::JsString>(error->properties["name"]).value);
                            }
                        }
                        std::string message_str = "";
                        if (error->properties.count("message") > 0) {
                            if (error->properties["message"].type() == typeid(std::string)) {
                                message_str = std::any_cast<std::string>(error->properties["message"]);
                            } else if (error->properties["message"].type() == typeid(const char *)) {
                                message_str = std::any_cast<const char *>(error->properties["message"]);
                            } else if (error->properties["message"].type() == typeid(jspp::JsString)) {
                                message_str = std::any_cast<std::string>(std::any_cast<jspp::JsString>(error->properties["message"]).value);
                            }
                        }
                        std::string stack_str = "";
                        if (error->properties.count("stack") > 0) {
                            if (error->properties["stack"].type() == typeid(std::string)) {
                                stack_str = std::any_cast<std::string>(error->properties["stack"]);
                            } else if (error->properties["stack"].type() == typeid(const char *)) {
                                stack_str = std::any_cast<const char *>(error->properties["stack"]);
                            } else if (error->properties["stack"].type() == typeid(jspp::JsString)) {
                                stack_str = std::any_cast<std::string>(std::any_cast<jspp::JsString>(error->properties["stack"]).value);
                            }
                        }
                        return name_str + ": " + message_str + "\n    at " + stack_str; }));
            // return object
            return jspp::JsValue(error);
        }
        inline jspp::JsValue make_default_error(const JsValue &message)
        {
            return Exception::make_error_with_name(message, "Error");
        }
        inline jspp::JsValue parse_error_from_value(const JsValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<std::exception>))
            {
                return Exception::make_error_with_name(std::string(std::any_cast<std::exception>(val).what()), "Error");
            }
            return val;
        }
        inline jspp::JsValue throw_unresolved_reference(const std::string &varName)
        {
            throw Exception::make_error_with_name(varName + " is not defined", "ReferenceError");
        }
        inline jspp::JsValue throw_immutable_assignment()
        {
            throw Exception::make_error_with_name("Assignment to constant variable.", "TypeError");
        }
        inline jspp::JsValue throw_invalid_return_statement()
        {
            throw Exception::make_error_with_name("Return statements are only valid inside functions.", "SyntaxError");
        }
    };

    namespace Object
    {
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
                std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                               {
                std::string str = "[";
                for (size_t i = 0; i < array->properties.size(); ++i)
                {
                    str += jspp::Convert::to_string(array->properties[i]);
                    if (i < array->properties.size() - 1)
                        str += ", ";
                }
                str += "]";
                return str; }));
            Prototype::set_accessor_property(
                array->prototype,
                "length",
                Prototype::to_handler(std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                                                     { return (int)array->properties.size(); })),
                Prototype::to_handler(std::function<jspp::JsValue(jspp::JsValue)>([=](auto val) mutable -> jspp::JsValue
                                                                                  {
                                                                size_t new_length = 0;
                                                                if (val.type() == typeid(int))
                                                                {
                                                                    int v = std::any_cast<int>(val);
                                                                    if (v < 0)
                                                                    {
                                                                        throw Exception::make_error_with_name("Invalid array length", "RangeError");
                                                                    }
                                                                    new_length = static_cast<size_t>(v);
                                                                }
                                                                else if (val.type() == typeid(double))
                                                                {
                                                                    double v = std::any_cast<double>(val);
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
                                                                array->properties.resize(new_length, undefined);
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
                std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
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
                Prototype::to_handler(std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                                                     { return (int)str_obj->value.length(); })),
                undefined // length is read-only for now
            );
            // Methods
            Prototype::set_data_property(
                str_obj->prototype,
                "toString",
                std::function<jspp::JsValue()>([=]() mutable -> jspp::JsValue
                                               { return str_obj->value; }));
            Prototype::set_data_property(
                str_obj->prototype,
                "charAt",
                Object::make_function(
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return "";
                        }
                        int index = 0;
                        if (args[0].type() == typeid(int))
                        {
                            index = std::any_cast<int>(args[0]);
                        }
                        if (index < 0 || index >= str_obj->value.length())
                        {
                            return "";
                        }
                        return std::string(1, str_obj->value[index]);
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "concat",
                Object::make_function(
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return str_obj->value;
                        }
                        int targetLength = 0;
                        if (args[0].type() == typeid(int))
                        {
                            targetLength = std::any_cast<int>(args[0]);
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if (str_obj->value.length() >= targetLength)
                        {
                            return str_obj->value;
                        }
                        std::string result = str_obj->value;
                        if (result.length() >= targetLength)
                        {
                            return result;
                        }
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return str_obj->value;
                        }
                        int targetLength = 0;
                        if (args[0].type() == typeid(int))
                        {
                            targetLength = std::any_cast<int>(args[0]);
                        }
                        std::string padString = " ";
                        if (args.size() > 1)
                        {
                            padString = Convert::to_string(args[1]);
                        }
                        if (str_obj->value.length() >= targetLength)
                        {
                            return str_obj->value;
                        }
                        std::string result = str_obj->value;
                        if (result.length() >= targetLength)
                        {
                            return result;
                        }
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        if (args.empty())
                        {
                            return "";
                        }
                        int count = 0;
                        if (args[0].type() == typeid(int))
                        {
                            count = std::any_cast<int>(args[0]);
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        int start = 0;
                        if (!args.empty() && args[0].type() == typeid(int))
                        {
                            start = std::any_cast<int>(args[0]);
                        }
                        int end = str_obj->value.length();
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            end = std::any_cast<int>(args[1]);
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        int start = 0;
                        if (!args.empty() && args[0].type() == typeid(int))
                        {
                            start = std::any_cast<int>(args[0]);
                        }
                        int end = str_obj->value.length();
                        if (args.size() > 1 && args[1].type() == typeid(int))
                        {
                            end = std::any_cast<int>(args[1]);
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
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
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
                        return result;
                    }));
            Prototype::set_data_property(
                str_obj->prototype,
                "trimStart",
                Object::make_function(
                    [=](const std::vector<JsValue> &args) mutable -> jspp::JsValue
                    {
                        std::string result = str_obj->value;
                        result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
                        return result;
                    }));
            // return object shared pointer
            return str_obj;
        }

    }

    namespace Access
    {
        // Helper function to check for TDZ and deref variables
        inline JsValue deref(const std::shared_ptr<JsValue> &var, const std::string &name)
        {
            if (!var)
                // This case should ideally not be hit in normal operation
                throw Exception::make_error_with_name("null variable pointer for " + name, "Internal compiler error");
            if ((*var).type() == typeid(Uninitialized))
                throw Exception::make_error_with_name("Cannot access '" + name + "' before initialization", "ReferenceError");
            return *var;
        }

        inline bool is_truthy(const JsValue &val)
        {
            if (!val.has_value())
                return false; // undefined
            if (val.type() == typeid(Undefined))
                return false;
            if (val.type() == typeid(Null))
                return false;
            if (val.type() == typeid(bool))
                return std::any_cast<bool>(val);
            if (val.type() == typeid(int))
                return std::any_cast<int>(val) != 0;
            if (val.type() == typeid(double))
                return std::any_cast<double>(val) != 0.0;
            if (val.type() == typeid(std::string))
                return !std::any_cast<std::string>(val).empty();
            if (val.type() == typeid(const char *))
                return std::any_cast<const char *>(val)[0] != '\0';
            if (val.type() == typeid(Uninitialized))
                return false;
            if (val.type() == typeid(std::shared_ptr<JsObject>))
            {
                return std::any_cast<std::shared_ptr<JsObject>>(val) != nullptr;
            }
            if (val.type() == typeid(std::shared_ptr<JsArray>))
            {
                return std::any_cast<std::shared_ptr<JsArray>>(val) != nullptr;
            }
            if (val.type() == typeid(std::shared_ptr<JsString>))
            {
                auto s = std::any_cast<std::shared_ptr<JsString>>(val);
                return s && !s->value.empty();
            }
            if (val.type() == typeid(std::shared_ptr<JsFunction>))
            {
                return std::any_cast<std::shared_ptr<JsFunction>>(val) != nullptr;
            }
            if (val.type() == typeid(std::function<JsValue(const std::vector<JsValue> &)>))
            {
                return true;
            }
            return true;
        }

        inline std::vector<std::string> get_object_keys(const JsValue &obj)
        {
            std::vector<std::string> keys;
            std::set<std::string> seen_keys; // To handle duplicates and shadowing

            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (ptr)
                {
                    // Add own properties first
                    for (const auto &pair : ptr->properties)
                    {
                        if (seen_keys.find(pair.first) == seen_keys.end())
                        {
                            keys.push_back(pair.first);
                            seen_keys.insert(pair.first);
                        }
                    }

                    // Add enumerable prototype properties
                    for (const auto &pair : ptr->prototype)
                    {
                        bool is_enumerable = false;
                        if (std::holds_alternative<DataDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<DataDescriptor>(pair.second).enumerable;
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(pair.second))
                        {
                            is_enumerable = std::get<AccessorDescriptor>(pair.second).enumerable;
                        }
                        // If it's enumerable and not already seen (shadowed by own property)
                        if (is_enumerable && seen_keys.find(pair.first) == seen_keys.end())
                        {
                            keys.push_back(pair.first);
                            seen_keys.insert(pair.first);
                        }
                    }
                }
            }
            return keys;
        }

        inline JsValue get_property(const JsValue &obj, const JsValue &key)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");

                const auto key_str = Convert::to_string(key);
                const auto it = ptr->properties.find(key_str);
                if (it != ptr->properties.end())
                {
                    return it->second;
                }

                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    // Handle prototype property
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");

                std::string key_str = Convert::to_string(key);
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos != key_str.length())
                    {
                        throw std::invalid_argument("not a valid integer index");
                    }

                    if (index < ptr->properties.size())
                    {
                        return ptr->properties[index];
                    }
                    return undefined;
                }
                catch (...)
                {
                    // Not a numeric index, check prototype
                    const auto proto_it = ptr->prototype.find(key_str);
                    if (proto_it != ptr->prototype.end())
                    {
                        const auto &prop = proto_it->second;
                        if (std::holds_alternative<DataDescriptor>(prop))
                        {
                            return std::get<DataDescriptor>(prop).value;
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(prop))
                        {
                            const auto &accessor = std::get<AccessorDescriptor>(prop);
                            if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                            {
                                return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                            }
                        }
                    }
                    return undefined;
                }
            }
            if (obj.type() == typeid(std::shared_ptr<JsString>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsString> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                // Check prototype for properties like 'length'
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                        }
                    }
                }
                // Also handle numeric indexing for characters
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos == key_str.length() && index < ptr->value.length())
                    {
                        return jspp::Object::make_string(std::string(1, ptr->value[index]));
                    }
                }
                catch (...)
                { /* Not a numeric index, ignore */
                }
                return undefined;
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot read properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        return std::get<DataDescriptor>(prop).value;
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get))
                        {
                            return std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.get)({});
                        }
                    }
                }
                return undefined;
            }
            throw Exception::make_error_with_name("Cannot read properties of non-object type", "TypeError");
        }

        inline JsValue set_property(const JsValue &obj, const JsValue &key, const JsValue &val)
        {
            if (obj.type() == typeid(std::shared_ptr<JsObject>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsObject> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");

                const auto key_str = Convert::to_string(key);
                const auto proto_it = ptr->prototype.find(key_str);
                if (proto_it != ptr->prototype.end())
                {
                    // Handle prototype property
                    const auto &prop = proto_it->second;
                    if (std::holds_alternative<DataDescriptor>(prop))
                    {
                        auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, JsValue> &>(prop));
                        if (data_desc.writable)
                        {
                            data_desc.value = val;
                        }
                    }
                    else if (std::holds_alternative<AccessorDescriptor>(prop))
                    {
                        const auto &accessor = std::get<AccessorDescriptor>(prop);
                        if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set))
                        {
                            std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set)({val});
                        }
                    }
                }
                else
                {
                    ptr->properties[key_str] = val;
                }

                return val;
            }
            if (obj.type() == typeid(std::shared_ptr<JsArray>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsArray> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");

                std::string key_str = Convert::to_string(key);
                try
                {
                    size_t pos;
                    unsigned long index = std::stoul(key_str, &pos);
                    if (pos != key_str.length())
                    {
                        throw std::invalid_argument("not a valid integer index");
                    }

                    if (index >= ptr->properties.size())
                    {
                        ptr->properties.resize(index + 1, undefined);
                    }
                    ptr->properties[index] = val;
                    return val;
                }
                catch (...)
                {
                    // Not a numeric index, but a string
                    // handle string properties on arrays
                    const auto proto_it = ptr->prototype.find(key_str);

                    if (proto_it != ptr->prototype.end())
                    {
                        const auto &prop = proto_it->second;
                        if (std::holds_alternative<DataDescriptor>(prop))
                        {
                            auto &data_desc = std::get<DataDescriptor>(const_cast<std::variant<DataDescriptor, AccessorDescriptor, JsValue> &>(prop));
                            if (data_desc.writable)
                            {
                                data_desc.value = val;
                            }
                        }
                        else if (std::holds_alternative<AccessorDescriptor>(prop))
                        {
                            const auto &accessor = std::get<AccessorDescriptor>(prop);
                            if (std::holds_alternative<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set))
                            {
                                std::get<std::function<JsValue(const std::vector<JsValue> &)>>(accessor.set)({val});
                            }
                        }
                    }
                    else
                    {
                        ptr->prototype[key_str] = val;
                    }
                    return val;
                }
            }
            if (obj.type() == typeid(std::shared_ptr<JsFunction>))
            {
                auto &ptr = std::any_cast<const std::shared_ptr<JsFunction> &>(obj);
                if (!ptr)
                    throw Exception::make_error_with_name("Cannot set properties of null", "TypeError");
                const auto key_str = Convert::to_string(key);
                ptr->prototype[key_str] = val;
                return val;
            }
            throw jspp::Exception::make_error_with_name("Cannot set properties of non-object type", "TypeError");
        }

        inline bool strict_equals(const JsValue &lhs, const JsValue &rhs)
        {
            // Strict equality (===)
            if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
                return std::any_cast<int>(lhs) == std::any_cast<double>(rhs);
            if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
                return std::any_cast<double>(lhs) == std::any_cast<int>(rhs);

            if (lhs.type() != rhs.type())
                return false;

            if (!lhs.has_value() || lhs.type() == typeid(Undefined))
                return true; // both undefined
            if (lhs.type() == typeid(Null))
                return true; // both null

            if (lhs.type() == typeid(int))
                return std::any_cast<int>(lhs) == std::any_cast<int>(rhs);
            if (lhs.type() == typeid(double))
                return std::any_cast<double>(lhs) == std::any_cast<double>(rhs);
            if (lhs.type() == typeid(bool))
                return std::any_cast<bool>(lhs) == std::any_cast<bool>(rhs);
            if (lhs.type() == typeid(std::string))
                return std::any_cast<std::string>(lhs) == std::any_cast<std::string>(rhs);
            if (lhs.type() == typeid(const char *))
                return strcmp(std::any_cast<const char *>(lhs), std::any_cast<const char *>(rhs)) == 0;

            if (lhs.type() == typeid(std::shared_ptr<jspp::JsObject>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsObject>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsObject>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsArray>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsArray>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsArray>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsString>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsString>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsString>>(rhs);
            }
            if (lhs.type() == typeid(std::shared_ptr<jspp::JsFunction>))
            {
                return std::any_cast<std::shared_ptr<jspp::JsFunction>>(lhs) == std::any_cast<std::shared_ptr<jspp::JsFunction>>(rhs);
            }

            // Cannot compare functions for equality in C++.
            return false;
        }

        inline bool equals(const JsValue &lhs, const JsValue &rhs)
        {
            // Abstract/Loose Equality (==)
            if (lhs.type() == rhs.type())
            {
                return strict_equals(lhs, rhs); // Use strict equality if types are same
            }

            // null == undefined
            if ((lhs.type() == typeid(Null) && (!rhs.has_value() || rhs.type() == typeid(Undefined))) ||
                ((!lhs.has_value() || lhs.type() == typeid(Undefined)) && rhs.type() == typeid(Null)))
            {
                return true;
            }

            // number == string
            if ((lhs.type() == typeid(int) || lhs.type() == typeid(double)) && (rhs.type() == typeid(std::shared_ptr<JsString>) || rhs.type() == typeid(std::string) || rhs.type() == typeid(const char *)))
            {
                double l = lhs.type() == typeid(int) ? std::any_cast<int>(lhs) : std::any_cast<double>(lhs);
                std::string s_rhs = Convert::to_string(rhs);

                s_rhs.erase(s_rhs.begin(), std::find_if(s_rhs.begin(), s_rhs.end(), [](int ch)
                                                        { return !std::isspace(ch); }));
                s_rhs.erase(std::find_if(s_rhs.rbegin(), s_rhs.rend(), [](int ch)
                                         { return !std::isspace(ch); })
                                .base(),
                            s_rhs.end());

                if (s_rhs.empty())
                {
                    return l == 0;
                }

                try
                {
                    double r = std::stod(s_rhs);
                    return l == r;
                }
                catch (...)
                {
                    return false;
                }
            }
            if ((rhs.type() == typeid(int) || rhs.type() == typeid(double)) && (lhs.type() == typeid(std::string) || lhs.type() == typeid(const char *)))
            {
                return equals(rhs, lhs); // reuse logic
            }

            // boolean == any
            if (lhs.type() == typeid(bool))
            {
                return equals(jspp::JsValue(std::any_cast<bool>(lhs) ? 1 : 0), rhs);
            }
            if (rhs.type() == typeid(bool))
            {
                return equals(lhs, jspp::JsValue(std::any_cast<bool>(rhs) ? 1 : 0));
            }

            // object == primitive
            if ((lhs.type() == typeid(std::shared_ptr<JsObject>) || lhs.type() == typeid(std::shared_ptr<JsArray>) || lhs.type() == typeid(std::shared_ptr<JsString>) || lhs.type() == typeid(std::shared_ptr<JsFunction>)) &&
                (rhs.type() != typeid(std::shared_ptr<JsObject>) && rhs.type() != typeid(std::shared_ptr<JsArray>) && rhs.type() != typeid(std::shared_ptr<JsString>) && rhs.type() != typeid(std::shared_ptr<JsFunction>)))
            {
                return equals(jspp::JsValue(Convert::to_string(lhs)), rhs);
            }
            if ((rhs.type() == typeid(std::shared_ptr<JsObject>) || rhs.type() == typeid(std::shared_ptr<JsArray>) || rhs.type() == typeid(std::shared_ptr<JsString>) || rhs.type() == typeid(std::shared_ptr<JsFunction>)) &&
                (lhs.type() != typeid(std::shared_ptr<JsObject>) && lhs.type() != typeid(std::shared_ptr<JsArray>) && lhs.type() != typeid(std::shared_ptr<JsString>) && lhs.type() != typeid(std::shared_ptr<JsFunction>)))
            {
                return equals(lhs, jspp::JsValue(Convert::to_string(rhs)));
            }

            return false;
        }
    }

}

// Define operators for JsValue
inline jspp::JsValue operator+(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) + std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) + std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) + std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) + std::any_cast<int>(rhs);
    if (lhs.type() == typeid(std::shared_ptr<jspp::JsString>) || rhs.type() == typeid(std::shared_ptr<jspp::JsString>))
    {
        return jspp::Object::make_string(jspp::Convert::to_string(lhs) + jspp::Convert::to_string(rhs));
    }
    return undefined;
}
inline jspp::JsValue operator*(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) * std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) * std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) * std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) * std::any_cast<int>(rhs);
    return undefined;
}
inline jspp::JsValue operator-(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) - std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) - std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) - std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) - std::any_cast<int>(rhs);
    return undefined;
}
inline bool operator<=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) <= std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) <= std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) <= std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) <= std::any_cast<int>(rhs);
    return false;
}
inline bool operator>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) > std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) > std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) > std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) > std::any_cast<int>(rhs);
    return false;
}
inline bool operator<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) < std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) < std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) < std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) < std::any_cast<int>(rhs);
    return false;
}

inline jspp::JsValue operator/(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) / std::any_cast<int>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::any_cast<double>(lhs) / std::any_cast<double>(rhs);
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::any_cast<int>(lhs) / std::any_cast<double>(rhs);
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::any_cast<double>(lhs) / std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator%(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) % std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator^(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) ^ std::any_cast<int>(rhs);
    return undefined;
}

inline bool operator>=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return (lhs > rhs) || jspp::Access::equals(lhs, rhs);
}

inline bool operator!=(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    return !jspp::Access::equals(lhs, rhs);
}

inline jspp::JsValue operator&(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) & std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator|(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) | std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator~(const jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        return ~std::any_cast<int>(val);
    return undefined;
}

inline jspp::JsValue operator<<(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) << std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue operator>>(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::any_cast<int>(lhs) >> std::any_cast<int>(rhs);
    return undefined;
}

inline jspp::JsValue pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<int>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<int>(rhs));
    return undefined;
}

inline jspp::JsValue &operator++(jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        val = std::any_cast<int>(val) + 1;
    else if (val.type() == typeid(double))
        val = std::any_cast<double>(val) + 1;
    return val;
}

inline jspp::JsValue operator++(jspp::JsValue &val, int)
{
    jspp::JsValue old = val;
    ++val;
    return old;
}

inline jspp::JsValue &operator--(jspp::JsValue &val)
{
    if (val.type() == typeid(int))
        val = std::any_cast<int>(val) - 1;
    else if (val.type() == typeid(double))
        val = std::any_cast<double>(val) - 1;
    return val;
}

inline jspp::JsValue operator--(jspp::JsValue &val, int)
{
    jspp::JsValue old = val;
    --val;
    return old;
}

inline jspp::JsValue &operator+=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline jspp::JsValue &operator-=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline jspp::JsValue &operator*=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

inline jspp::JsValue &operator/=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

inline jspp::JsValue &operator%=(jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    lhs = lhs % rhs;
    return lhs;
}

// Define operators for osstream
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline jspp::JsValue jspp::pow(const jspp::JsValue &lhs, const jspp::JsValue &rhs)
{
    if (lhs.type() == typeid(int) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<int>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(int) && rhs.type() == typeid(double))
        return std::pow(std::any_cast<int>(lhs), std::any_cast<double>(rhs));
    if (lhs.type() == typeid(double) && rhs.type() == typeid(int))
        return std::pow(std::any_cast<double>(lhs), std::any_cast<int>(rhs));
    return undefined;
}


inline std::ostream &operator<<(std::ostream &os, const jspp::JsValue &v)
{
    if (v.type() == typeid(std::shared_ptr<jspp::JsValue>))
    {
        const auto &ptr = std::any_cast<std::shared_ptr<jspp::JsValue>>(v);
        if (ptr)
            return os << *ptr;
        else
        {
            os << "undefined";
            return os;
        }
    }
    os << jspp::Convert::to_string(v);
    return os;
}

// Basic console API
struct Console
{
    template <typename... Args>
    Undefined log(Args... args)
    {
        ((std::cout << jspp::JsValue(args) << " "), ...);
        std::cout << std::endl;
        return undefined;
    }

    template <typename... Args>
    Undefined warn(Args... args)
    {
        std::cerr << "\033[33m"; // Yellow
        ((std::cerr << jspp::JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }

    template <typename... Args>
    Undefined error(Args... args)
    {
        std::cerr << "\033[31m"; // Red
        ((std::cerr << jspp::JsValue(args) << " "), ...);
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    }
};
inline Console console;