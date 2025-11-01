#pragma once

#include "types.hpp"
#include <sstream>
#include <iomanip>

namespace jspp
{
    namespace Convert
    {
        inline JsValue unwrap_number(const JsValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<jspp::JsNumber>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsNumber>>(val);
                if (std::holds_alternative<int>(ptr->value))
                {
                    return std::get<int>(ptr->value);
                }
                else if (std::holds_alternative<double>(ptr->value))
                {
                    return std::get<double>(ptr->value);
                }
            }
            return val;
        }

        inline JsValue unwrap_boolean(const JsValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<jspp::JsBoolean>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsBoolean>>(val);
                return ptr->value;
            }
            return val;
        }

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
            if (val.type() == typeid(double)) {
                double d_val = std::any_cast<double>(val);
                if (std::abs(d_val) >= 1e21 || (std::abs(d_val) > 0 && std::abs(d_val) < 1e-6)) {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(4) << d_val;
                    return oss.str();
                } else {
                    std::ostringstream oss;
                    oss << std::setprecision(6) << std::fixed << d_val;
                    std::string s = oss.str();
                    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                    if (s.back() == '.') {
                        s.pop_back();
                    }
                    return s;
                }
            }
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
            if (val.type() == typeid(std::shared_ptr<jspp::JsNumber>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsNumber>>(val);
                if (std::holds_alternative<int>(ptr->value))
                {
                    return jspp::Convert::to_string(std::get<int>(ptr->value));
                }
                if (std::holds_alternative<double>(ptr->value))
                {
                    return jspp::Convert::to_string(std::get<double>(ptr->value));
                }
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsBoolean>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsBoolean>>(val);
                return jspp::Convert::to_string(ptr->value);
            }
            return "[Object Object]";
        }
    }
}
