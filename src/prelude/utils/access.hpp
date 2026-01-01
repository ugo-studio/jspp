#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"
#include "values/function.hpp"
#include "values/symbol.hpp"
#include "exception.hpp"
#include "any_value.hpp"
#include <ranges>

namespace jspp
{
    namespace Access
    {
        // Helper function to check for TDZ and deref heap-allocated variables
        inline const AnyValue &deref_ptr(const std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return *var;
        }
        inline AnyValue &deref_ptr(std::shared_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return *var;
        }
        inline const AnyValue &deref_ptr(const std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return *var;
        }
        inline AnyValue &deref_ptr(std::unique_ptr<AnyValue> &var, const std::string &name)
        {
            if ((*var).is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return *var;
        }

        // Helper function to check for TDZ on stack-allocated variables
        inline const AnyValue &deref_stack(const AnyValue &var, const std::string &name)
        {
            if (var.is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return var;
        }
        inline AnyValue &deref_stack(AnyValue &var, const std::string &name)
        {
            if (var.is_uninitialized()) [[unlikely]]
            {
                Exception::throw_uninitialized_reference(name);
            }
            return var;
        }

        inline const AnyValue type_of(const std::optional<AnyValue> &val = std::nullopt)
        {
            if (!val.has_value())
                return AnyValue::make_string("undefined");

            switch (val.value().get_type())
            {
            case JsType::Undefined:
                return AnyValue::make_string("undefined");
            case JsType::Null:
                return AnyValue::make_string("object");
            case JsType::Boolean:
                return AnyValue::make_string("boolean");
            case JsType::Number:
                return AnyValue::make_string("number");
            case JsType::String:
                return AnyValue::make_string("string");
            case JsType::Symbol:
                return AnyValue::make_string("symbol");
            case JsType::Function:
                return AnyValue::make_string("function");
            case JsType::Object:
                return AnyValue::make_string("object");
            case JsType::Array:
                return AnyValue::make_string("object");
            case JsType::Iterator:
                return AnyValue::make_string("object");
            default:
                return AnyValue::make_string("undefined");
            }
        }

        // Helper function to get enumerable own property keys/values of an object
        inline std::vector<std::string> get_object_keys(const AnyValue &obj)
        {
            std::vector<std::string> keys;

            if (obj.is_object())
            {
                auto ptr = obj.as_object();
                // Use shape's property_names for stable iteration order
                for (const auto &key : ptr->shape->property_names)
                {
                    if (ptr->deleted_keys.count(key)) continue;

                    if (JsSymbol::is_internal_key(key))
                        continue;

                    auto offset_opt = ptr->shape->get_offset(key);
                    if (!offset_opt.has_value())
                        continue;

                    const auto &val = ptr->storage[offset_opt.value()];

                    if (val.is_data_descriptor())
                    {
                        if (val.as_data_descriptor()->enumerable)
                            keys.push_back(key);
                    }
                    else if (val.is_accessor_descriptor())
                    {
                        if (val.as_accessor_descriptor()->enumerable)
                            keys.push_back(key);
                    }
                    else
                    {
                        keys.push_back(key);
                    }
                }
            }
            if (obj.is_function())
            {
                auto ptr = obj.as_function();
                for (const auto &pair : ptr->props)
                {
                    if (!JsSymbol::is_internal_key(pair.first))
                    {
                        if (!pair.second.is_data_descriptor() && !pair.second.is_accessor_descriptor())
                            keys.push_back(pair.first);
                        else if ((pair.second.is_data_descriptor() && pair.second.as_data_descriptor()->enumerable) ||
                                 (pair.second.is_accessor_descriptor() && pair.second.as_accessor_descriptor()->enumerable))
                            keys.push_back(pair.first);
                    }
                }
            }
            if (obj.is_array())
            {
                auto len = obj.as_array()->length;
                for (auto i = 0; i < len; ++i)
                {
                    keys.push_back(std::to_string(i));
                }
            }
            if (obj.is_string())
            {
                auto len = obj.as_string()->value.length();
                for (auto i = 0; i < len; ++i)
                {
                    keys.push_back(std::to_string(i));
                }
            }

            return keys;
        }
        inline AnyValue get_object_value_iterator(const AnyValue &obj, const std::string &name)
        {
            if (obj.is_iterator())
            {
                return obj;
            }

            auto gen_fn = obj.get_own_property(WellKnownSymbols::iterator->key);
            if (gen_fn.is_function())
            {
                auto iter = gen_fn.as_function()->call(obj, {});
                if (iter.is_iterator())
                {
                    return iter;
                }
                if (iter.is_object())
                {
                    auto next_fn = iter.get_own_property("next");
                    if (next_fn.is_function())
                    {
                        return iter;
                    }
                }
            }

            throw jspp::Exception::make_exception(name + " is not iterable", "TypeError");
        }

        inline AnyValue in(const AnyValue &lhs, const AnyValue &rhs)
        {
            if (!rhs.is_object() && !rhs.is_array() && !rhs.is_function() && !rhs.is_promise() && !rhs.is_iterator())
            {
                throw jspp::Exception::make_exception("Cannot use 'in' operator to search for '" + lhs.to_std_string() + "' in " + rhs.to_std_string(), "TypeError");
            }
            return AnyValue::make_boolean(rhs.has_property(lhs.to_std_string()));
        }

        inline AnyValue instance_of(const AnyValue &lhs, const AnyValue &rhs)
        {
            if (!rhs.is_function())
            {
                throw jspp::Exception::make_exception("Right-hand side of 'instanceof' is not callable", "TypeError");
            }
            if (!lhs.is_object() && !lhs.is_array() && !lhs.is_function() && !lhs.is_promise() && !lhs.is_iterator())
            {
                return Constants::FALSE;
            }
            AnyValue targetProto = rhs.get_own_property("prototype");
            if (!targetProto.is_object() && !targetProto.is_array() && !targetProto.is_function())
            {
                throw jspp::Exception::make_exception("Function has non-object prototype in instanceof check", "TypeError");
            }
            // Walk prototype chain of lhs
            AnyValue current = lhs;

            while (true)
            {
                AnyValue proto;
                if (current.is_object())
                {
                    auto p = current.as_object()->proto;
                    if (p)
                        proto = *p;
                    else
                        break;
                }
                else if (current.is_array())
                {
                    auto p = current.as_array()->proto;
                    if (p)
                        proto = *p;
                    else
                        break;
                }
                else if (current.is_function())
                {
                    auto p = current.as_function()->proto;
                    if (p)
                        proto = *p;
                    else
                        break;
                }
                else if (current.is_promise())
                {
                    // Promises don't store explicit proto yet in our impl
                    break;
                }
                else
                {
                    break;
                }
                if (proto.is_null() || proto.is_undefined())
                    break;
                if (is_strictly_equal_to_primitive(proto, targetProto))
                    return Constants::TRUE;
                current = proto;
            }
            return Constants::FALSE;
        }

        inline AnyValue delete_property(const AnyValue &obj, const AnyValue &key)
        {
            if (obj.is_object())
            {
                auto ptr = obj.as_object();
                std::string key_str = key.to_std_string();
                if (ptr->shape->get_offset(key_str).has_value()) {
                    ptr->deleted_keys.insert(key_str);
                }
                return Constants::TRUE;
            }
            if (obj.is_array())
            {
                auto ptr = obj.as_array();
                std::string key_str = key.to_std_string();
                if (JsArray::is_array_index(key_str))
                {
                    uint32_t idx = static_cast<uint32_t>(std::stoull(key_str));
                    if (idx < ptr->dense.size())
                    {
                        ptr->dense[idx] = Constants::UNINITIALIZED;
                    }
                    else
                    {
                        ptr->sparse.erase(idx);
                    }
                }
                else
                {
                    ptr->props.erase(key_str);
                }
                return Constants::TRUE;
            }
            if (obj.is_function())
            {
                auto ptr = obj.as_function();
                ptr->props.erase(key.to_std_string());
                return Constants::TRUE;
            }
            return Constants::TRUE;
        }

        inline AnyValue optional_get_property(const AnyValue &obj, const std::string &key)
        {
            if (obj.is_null() || obj.is_undefined())
                return Constants::UNDEFINED;
            return obj.get_own_property(key);
        }

        inline AnyValue optional_get_element(const AnyValue &obj, const AnyValue &key)
        {
            if (obj.is_null() || obj.is_undefined())
                return Constants::UNDEFINED;
            return obj.get_own_property(key);
        }

        inline AnyValue optional_get_element(const AnyValue &obj, const double &key)
        {
            if (obj.is_null() || obj.is_undefined())
                return Constants::UNDEFINED;
            return obj.get_own_property(static_cast<uint32_t>(key));
        }

        inline AnyValue optional_call(const AnyValue &fn, const AnyValue &thisVal, std::span<const AnyValue> args, const std::optional<std::string> &name = std::nullopt)
        {
            if (fn.is_null() || fn.is_undefined())
                return Constants::UNDEFINED;
            return fn.as_function(name)->call(thisVal, args);
        }

    }

}
