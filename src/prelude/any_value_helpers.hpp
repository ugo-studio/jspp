#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/string.hpp"

namespace jspp
{
    const std::string AnyValue::to_std_string() const noexcept
    {
        switch (get_type())
        {
        case JsType::Undefined:
            return "undefined";
        case JsType::Null:
            return "null";
        case JsType::Boolean:
            return std::get<bool>(storage) ? "true" : "false";
        case JsType::String:
            return std::get<std::shared_ptr<JsString>>(storage)->to_std_string();
        case JsType::Object:
            return std::get<std::shared_ptr<JsObject>>(storage)->to_std_string();
        case JsType::Array:
            return std::get<std::shared_ptr<JsArray>>(storage)->to_std_string();
        case JsType::Function:
            return std::get<std::shared_ptr<JsFunction>>(storage)->to_std_string();
        case JsType::Iterator:
            return std::get<std::shared_ptr<JsIterator<AnyValue>>>(storage)->to_std_string();
        case JsType::Promise:
            return std::get<std::shared_ptr<JsPromise>>(storage)->to_std_string();
        case JsType::Symbol:
            return std::get<std::shared_ptr<JsSymbol>>(storage)->to_std_string();
        case JsType::DataDescriptor:
            return std::get<std::shared_ptr<DataDescriptor>>(storage)->value->to_std_string();
        case JsType::AccessorDescriptor:
        {
            if (std::get<std::shared_ptr<AccessorDescriptor>>(storage)->get.has_value())
                return std::get<std::shared_ptr<AccessorDescriptor>>(storage)->get.value()(*this, {}).to_std_string();
            else
                return "undefined";
        }
        case JsType::Number:
        {
            double num = std::get<double>(storage);
            if (std::isnan(num))
            {
                return "NaN";
            }
            if (std::abs(num) >= 1e21 || (std::abs(num) > 0 && std::abs(num) < 1e-6))
            {
                std::ostringstream oss;
                oss << std::scientific << std::setprecision(4) << num;
                return oss.str();
            }
            else
            {
                std::ostringstream oss;
                oss << std::setprecision(6) << std::fixed << num;
                std::string s = oss.str();
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (!s.empty() && s.back() == '.')
                {
                    s.pop_back();
                }
                return s;
            }
        }
        // Uninitialized and default should not be reached under normal circumstances
        case JsType::Uninitialized:
            // return "<uninitialized>";
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return "";
        }
    }

    void AnyValue::set_prototype(const AnyValue &proto)
    {
        if (is_object())
        {
            std::get<std::shared_ptr<JsObject>>(storage)->proto = std::make_shared<AnyValue>(proto);
        }
        else if (is_function())
        {
            std::get<std::shared_ptr<JsFunction>>(storage)->proto = std::make_shared<AnyValue>(proto);
        }
    }

}