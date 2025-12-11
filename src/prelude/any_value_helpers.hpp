#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/string.hpp"

const bool jspp::AnyValue::is_truthy() const noexcept
{
    switch (storage.type)
    {
    case JsType::Boolean:
        return storage.boolean;
    case JsType::Number:
        return storage.number != 0.0;
    case JsType::String:
        return !storage.str->value.empty();
    case JsType::Undefined:
        return false;
    case JsType::Null:
        return false;
    case JsType::Uninitialized:
        return false;
    default:
        return true;
    }
}

const bool jspp::AnyValue::is_strictly_equal_to_primitive(const AnyValue &other) const noexcept
{
    if (storage.type == other.storage.type)
    {
        switch (storage.type)
        {
        case JsType::Boolean:
            return storage.boolean == other.storage.boolean;
        case JsType::Number:
            return storage.number == other.storage.number;
        case JsType::String:
            return (storage.str->value == other.storage.str->value);
        case JsType::Array:
            return (storage.array == other.storage.array);
        case JsType::Object:
            return (storage.object == other.storage.object);
        case JsType::Function:
            return (storage.function == other.storage.function);
        case JsType::Symbol:
            // Symbols are unique by reference/pointer identity
            return (storage.symbol == other.storage.symbol);
        case JsType::DataDescriptor:
            return storage.data_desc == other.storage.data_desc;
        case JsType::AccessorDescriptor:
            return storage.accessor_desc == other.storage.accessor_desc;
        default:
            return true;
        }
    }
    return false;
}
const bool jspp::AnyValue::is_equal_to_primitive(const AnyValue &other) const noexcept
{
    // Implements JavaScript's Abstract Equality Comparison Algorithm (==)
    // Step 1: If types are the same, use strict equality (===)
    if (storage.type == other.storage.type)
    {
        return is_strictly_equal_to_primitive(other);
    }
    // Steps 2 & 3: null == undefined
    if ((is_null() && other.is_undefined()) || (is_undefined() && other.is_null()))
    {
        return true;
    }
    // Step 4 & 5: number == string
    if (is_number() && other.is_string())
    {
        double num_this = this->as_double();
        double num_other;
        try
        {
            const std::string &s = other.as_string()->value;
            // JS considers empty string or whitespace-only string to be 0
            if (s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c)
                                         { return std::isspace(c); }))
            {
                num_other = 0.0;
            }
            else
            {
                size_t pos;
                num_other = std::stod(s, &pos);
                // Check if the entire string was consumed, allowing for trailing whitespace
                while (pos < s.length() && std::isspace(static_cast<unsigned char>(s[pos])))
                {
                    pos++;
                }
                if (pos != s.length())
                {
                    num_other = std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
        catch (...)
        {
            num_other = std::numeric_limits<double>::quiet_NaN();
        }
        return num_this == num_other;
    }
    if (is_string() && other.is_number())
    {
        // Delegate to the other operand to avoid code duplication
        return other.is_equal_to_primitive(*this);
    }
    // Step 6 & 7: boolean == any
    if (is_boolean())
    {
        // Convert boolean to number and re-compare
        return AnyValue::make_number(as_boolean() ? 1.0 : 0.0).is_equal_to_primitive(other);
    }
    if (other.is_boolean())
    {
        // Convert boolean to number and re-compare
        return is_equal_to_primitive(AnyValue::make_number(other.as_boolean() ? 1.0 : 0.0));
    }
    // Step 8 & 9: object == (string or number or symbol)
    // Simplified: Objects convert to primitives.
    if ((is_object() || is_array() || is_function()) && (other.is_string() || other.is_number() || other.is_symbol()))
    {
        // Convert object to primitive (string) and re-compare.
        // This is a simplification of JS's ToPrimitive.
        return AnyValue::make_string(to_std_string()).is_equal_to_primitive(other);
    }
    if ((other.is_object() || other.is_array() || other.is_function()) && (is_string() || is_number() || is_symbol()))
    {
        return other.is_equal_to_primitive(*this);
    }
    // Step 10: Datacriptor or accessor descriptor
    if (is_data_descriptor() || is_accessor_descriptor())
    {
        return (*this).is_strictly_equal_to_primitive(other);
    }
    // Step 11: All other cases (e.g., object == null) are false.
    return false;
}

const jspp::AnyValue jspp::AnyValue::is_strictly_equal_to(const AnyValue &other) const noexcept
{
    return AnyValue::make_boolean(is_strictly_equal_to_primitive(other));
}
const jspp::AnyValue jspp::AnyValue::is_equal_to(const AnyValue &other) const noexcept
{
    return AnyValue::make_boolean(is_equal_to_primitive(other));
}

const jspp::AnyValue jspp::AnyValue::not_strictly_equal_to(const AnyValue &other) const noexcept
{
    return AnyValue::make_boolean(!is_strictly_equal_to_primitive(other));
}
const jspp::AnyValue jspp::AnyValue::not_equal_to(const AnyValue &other) const noexcept
{
    return AnyValue::make_boolean(!is_equal_to_primitive(other));
}

const std::string jspp::AnyValue::to_std_string() const noexcept
{
    switch (storage.type)
    {
    case JsType::Undefined:
        return "undefined";
    case JsType::Null:
        return "null";
    case JsType::Boolean:
        return storage.boolean ? "true" : "false";
    case JsType::String:
        return storage.str->to_std_string();
    case JsType::Object:
        return storage.object->to_std_string();
    case JsType::Array:
        return storage.array->to_std_string();
    case JsType::Function:
        return storage.function->to_std_string();
    case JsType::Iterator:
        return storage.iterator->to_std_string();
    case JsType::Symbol:
        return storage.symbol->to_std_string();
    case JsType::DataDescriptor:
        return storage.data_desc->value->to_std_string();
    case JsType::AccessorDescriptor:
    {
        if (storage.accessor_desc->get.has_value())
            return storage.accessor_desc->get.value()(*this, {}).to_std_string();
        else
            return "undefined";
    }
    case JsType::Number:
    {
        if (std::isnan(storage.number))
        {
            return "NaN";
        }
        if (std::abs(storage.number) >= 1e21 || (std::abs(storage.number) > 0 && std::abs(storage.number) < 1e-6))
        {
            std::ostringstream oss;
            oss << std::scientific << std::setprecision(4) << storage.number;
            return oss.str();
        }
        else
        {
            std::ostringstream oss;
            oss << std::setprecision(6) << std::fixed << storage.number;
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
        return "<uninitialized>";
    default:
        return "";
    }
}

void jspp::AnyValue::set_prototype(const AnyValue &proto)
{
    if (is_object())
    {
        storage.object->proto = std::make_shared<AnyValue>(proto);
    }
    else if (is_function())
    {
        storage.function->proto = std::make_shared<AnyValue>(proto);
    }
}
