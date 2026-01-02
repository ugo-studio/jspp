#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/string.hpp"
#include "exception.hpp"

namespace jspp
{
    std::string AnyValue::to_std_string() const
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
        case JsType::AsyncIterator:
            return std::get<std::shared_ptr<JsAsyncIterator<AnyValue>>>(storage)->to_std_string();
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
        case JsType::Uninitialized:
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
        else if (is_array())
        {
            std::get<std::shared_ptr<JsArray>>(storage)->proto = std::make_shared<AnyValue>(proto);
        }
        else if (is_function())
        {
            std::get<std::shared_ptr<JsFunction>>(storage)->proto = std::make_shared<AnyValue>(proto);
        }
        else if (is_uninitialized())
        {
            Exception::throw_uninitialized_reference("#<Object>");
        }
    }

    // AnyValue::call implementation
    const AnyValue AnyValue::call(const AnyValue &thisVal, std::span<const AnyValue> args, const std::optional<std::string> &expr = std::nullopt) const
    {
        if (!is_function())
        {
            throw Exception::make_exception(expr.value_or(to_std_string()) + " is not a function", "TypeError");
        }
        return as_function()->call(thisVal, args); // Convert to function before calling, to avoid an infinite loop
    }

    // AnyValue::construct implementation
    const AnyValue AnyValue::construct(std::span<const AnyValue> args, const std::optional<std::string> &name) const
    {
        if (!is_function() || !as_function()->is_constructor)
        {
            // std::cerr << "Construct fail: " << name.value_or(to_std_string()) << " is_function=" << is_function() << " is_constructor=" << (is_function() ? as_function()->is_constructor : false) << std::endl;
            throw Exception::make_exception(name.value_or(to_std_string()) + " is not a constructor", "TypeError");
        }

        // 1. Get prototype
        AnyValue proto = get_own_property("prototype");
        // If prototype is not an object, default to a plain object (which ideally inherits from Object.prototype)
        // Here we just make a plain object.
        if (!proto.is_object())
        {
            proto = AnyValue::make_object({});
        }

        // 2. Create instance
        AnyValue instance = AnyValue::make_object_with_proto({}, proto);

        // 3. Call function
        // We pass 'instance' as 'this'
        AnyValue result = call(instance, args);

        // 4. Return result if object, else instance
        if (result.is_object() || result.is_function() || result.is_array() || result.is_promise())
        {
            return result;
        }
        return instance;
    }

}