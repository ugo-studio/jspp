#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"
#include <sstream>
#include <iomanip>

namespace jspp
{
    // Forward declaration of Prototype namespace and get_prototype function
    namespace Prototype
    {
        jspp::AnyValue get_prototype(const jspp::AnyValue &obj, const jspp::AnyValue &key);
    }

    namespace Convert
    {

        inline AnyValue unwrap_number(const AnyValue &val)
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

        inline AnyValue unwrap_boolean(const AnyValue &val)
        {
            if (val.type() == typeid(std::shared_ptr<jspp::JsBoolean>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsBoolean>>(val);
                return ptr->value;
            }
            return val;
        }

        inline std::string to_string(const AnyValue &val)
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
            {
                double d_val = std::any_cast<double>(val);
                if (std::abs(d_val) >= 1e21 || (std::abs(d_val) > 0 && std::abs(d_val) < 1e-6))
                {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(4) << d_val;
                    return oss.str();
                }
                else
                {
                    std::ostringstream oss;
                    oss << std::setprecision(6) << std::fixed << d_val;
                    std::string s = oss.str();
                    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                    if (s.back() == '.')
                    {
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
                auto toStringFn = jspp::Prototype::get_prototype(val, WellKnownSymbols::toString);
                if (toStringFn.type() == typeid(std::shared_ptr<jspp::JsFunction>))
                {
                    auto fn = std::any_cast<std::shared_ptr<jspp::JsFunction>>(toStringFn);
                    return to_string(fn->call({}));
                }
                return "[Object Object]";
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsArray>))
            {
                auto toStringFn = jspp::Prototype::get_prototype(val, WellKnownSymbols::toString);
                if (toStringFn.type() == typeid(std::shared_ptr<jspp::JsFunction>))
                {
                    auto fn = std::any_cast<std::shared_ptr<jspp::JsFunction>>(toStringFn);
                    return to_string(fn->call({}));
                }
                return "";
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsString>))
            {
                auto ptr = std::any_cast<std::shared_ptr<jspp::JsString>>(val);
                return ptr->value;
            }
            if (val.type() == typeid(std::shared_ptr<jspp::JsFunction>))
            {
                auto toStringFn = jspp::Prototype::get_prototype(val, WellKnownSymbols::toString);
                if (toStringFn.type() == typeid(std::shared_ptr<jspp::JsFunction>))
                {
                    auto fn = std::any_cast<std::shared_ptr<jspp::JsFunction>>(toStringFn);
                    return to_string(fn->call({}));
                }
                return "function () { [native code] }";
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
