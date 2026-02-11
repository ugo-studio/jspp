#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/string.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/iterator.hpp"
#include "values/async_iterator.hpp"
#include "values/promise.hpp"
#include "values/symbol.hpp"
#include "values/descriptors.hpp"
#include "exception.hpp"

namespace jspp
{
    // --- AnyValue GETTERS ---
    inline JsString *AnyValue::as_string() const noexcept { return static_cast<JsString *>(get_ptr()); }
    inline JsObject *AnyValue::as_object() const noexcept { return static_cast<JsObject *>(get_ptr()); }
    inline JsArray *AnyValue::as_array() const noexcept { return static_cast<JsArray *>(get_ptr()); }
    inline JsFunction *AnyValue::as_function() const noexcept { return static_cast<JsFunction *>(get_ptr()); }
    inline JsSymbol *AnyValue::as_symbol() const noexcept { return static_cast<JsSymbol *>(get_ptr()); }
    inline JsPromise *AnyValue::as_promise() const noexcept { return static_cast<JsPromise *>(get_ptr()); }
    inline JsIterator<AnyValue> *AnyValue::as_iterator() const noexcept { return static_cast<JsIterator<AnyValue> *>(get_ptr()); }
    inline JsAsyncIterator<AnyValue> *AnyValue::as_async_iterator() const noexcept { return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr()); }
    inline DataDescriptor *AnyValue::as_data_descriptor() const noexcept { return static_cast<DataDescriptor *>(get_ptr()); }
    inline AccessorDescriptor *AnyValue::as_accessor_descriptor() const noexcept { return static_cast<AccessorDescriptor *>(get_ptr()); }

    inline bool AnyValue::is_generator() const noexcept { return is_function() && as_function()->is_generator; }

    // --- AnyValue FACTORIES ---
    inline AnyValue AnyValue::make_string(const std::string &raw_s) noexcept
    {
        return from_ptr(new JsString(raw_s));
    }
    inline AnyValue AnyValue::make_object(std::initializer_list<std::pair<std::string, AnyValue>> props) noexcept
    {
        return from_ptr(new JsObject(props, make_null()));
    }
    inline AnyValue AnyValue::make_object(const std::map<std::string, AnyValue> &props) noexcept
    {
        return from_ptr(new JsObject(props, make_null()));
    }
    inline AnyValue AnyValue::make_object_with_proto(std::initializer_list<std::pair<std::string, AnyValue>> props, AnyValue proto) noexcept
    {
        return from_ptr(new JsObject(props, proto));
    }
    inline AnyValue AnyValue::make_object_with_proto(const std::map<std::string, AnyValue> &props, AnyValue proto) noexcept
    {
        return from_ptr(new JsObject(props, proto));
    }
    inline AnyValue AnyValue::make_array(std::span<const AnyValue> dense) noexcept
    {
        std::vector<AnyValue> vec;
        vec.reserve(dense.size());
        for (const auto &item : dense)
            vec.push_back(item);
        return from_ptr(new JsArray(std::move(vec)));
    }
    inline AnyValue AnyValue::make_array(const std::vector<AnyValue> &dense) noexcept
    {
        return from_ptr(new JsArray(dense));
    }
    inline AnyValue AnyValue::make_array(std::vector<AnyValue> &&dense) noexcept
    {
        return from_ptr(new JsArray(std::move(dense)));
    }
    inline AnyValue AnyValue::make_array_with_proto(std::span<const AnyValue> dense, AnyValue proto) noexcept
    {
        auto arr = new JsArray();
        arr->dense.reserve(dense.size());
        for (const auto &item : dense)
            arr->dense.push_back(item);
        arr->length = dense.size();
        arr->proto = proto;
        return from_ptr(arr);
    }
    inline AnyValue AnyValue::make_array_with_proto(const std::vector<AnyValue> &dense, AnyValue proto) noexcept
    {
        auto arr = new JsArray(dense);
        arr->proto = proto;
        return from_ptr(arr);
    }
    inline AnyValue AnyValue::make_array_with_proto(std::vector<AnyValue> &&dense, AnyValue proto) noexcept
    {
        auto arr = new JsArray(std::move(dense));
        arr->proto = proto;
        return from_ptr(arr);
    }
    inline AnyValue AnyValue::make_function(const JsFunctionCallable &call, const std::optional<std::string> &name, bool is_constructor) noexcept
    {
        auto v = from_ptr(new JsFunction(call, name, {}, false, is_constructor));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    inline AnyValue AnyValue::make_class(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, name, {}, true));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    inline AnyValue AnyValue::make_generator(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, true, name));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    inline AnyValue AnyValue::make_async_function(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, false, true, name));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    inline AnyValue AnyValue::make_async_generator(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, true, true, name));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    inline AnyValue AnyValue::make_symbol(const std::string &description) noexcept
    {
        return from_ptr(new JsSymbol(description));
    }
    inline AnyValue AnyValue::make_promise(const JsPromise &promise) noexcept
    {
        auto p = new JsPromise();
        *p = promise;
        return from_ptr(p);
    }
    inline AnyValue AnyValue::make_data_descriptor(AnyValue value, bool writable, bool enumerable, bool configurable) noexcept
    {
        return from_ptr(new DataDescriptor(value, writable, enumerable, configurable));
    }
    inline AnyValue AnyValue::make_accessor_descriptor(const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &get,
                                                       const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &set,
                                                       bool enumerable,
                                                       bool configurable) noexcept
    {
        return from_ptr(new AccessorDescriptor(get, set, enumerable, configurable));
    }

    inline AnyValue AnyValue::from_symbol(JsSymbol *sym) noexcept
    {
        return from_ptr(sym);
    }
    inline AnyValue AnyValue::from_string(JsString *str) noexcept
    {
        return from_ptr(str);
    }

    inline AnyValue AnyValue::from_promise(JsPromise &&promise) noexcept
    {
        auto it = new JsPromise(std::move(promise));
        return from_ptr(it);
    }

    inline AnyValue AnyValue::from_iterator(JsIterator<AnyValue> &&iterator) noexcept
    {
        auto it = new JsIterator<AnyValue>(std::move(iterator));
        return from_ptr(it);
    }
    inline AnyValue AnyValue::from_iterator_ref(JsIterator<AnyValue> *iterator) noexcept
    {
        return from_ptr(iterator);
    }
    inline AnyValue AnyValue::from_async_iterator(JsAsyncIterator<AnyValue> &&iterator) noexcept
    {
        auto it = new JsAsyncIterator<AnyValue>(std::move(iterator));
        return from_ptr(it);
    }

    inline AnyValue AnyValue::resolve_property_for_read(const AnyValue &val, const AnyValue &thisVal, const std::string &propName) noexcept
    {
        if (val.is_data_descriptor())
        {
            return val.as_data_descriptor()->value;
        }
        if (val.is_accessor_descriptor())
        {
            auto accessor = val.as_accessor_descriptor();
            if (accessor->get.has_value())
                return accessor->get.value()(thisVal, std::span<const AnyValue>{});
            else
            {
                return Constants::UNDEFINED;
            }
        }
        return val;
    }
    inline AnyValue AnyValue::resolve_property_for_write(AnyValue &val, const AnyValue &thisVal, const AnyValue &new_val, const std::string &propName)
    {
        if (val.is_data_descriptor())
        {
            auto data = val.as_data_descriptor();
            if (data->writable)
            {
                data->value = new_val;
                return new_val;
            }
            else
            {
                throw Exception::make_exception("Cannot assign to read only property '" + propName + "' of object '#<Object>'", "TypeError");
            }
        }
        if (val.is_accessor_descriptor())
        {
            auto accessor = val.as_accessor_descriptor();
            if (accessor->set.has_value())
            {
                const AnyValue args[] = {new_val};
                accessor->set.value()(thisVal, std::span<const AnyValue>(args, 1));
                return new_val;
            }
            else
            {
                throw Exception::make_exception("Cannot set property of #<Object> which has only a getter", "TypeError");
            }
        }
        val = new_val;
        return new_val;
    }

    inline std::string AnyValue::to_std_string() const
    {
        switch (get_type())
        {
        case JsType::Undefined:
            return "undefined";
        case JsType::Null:
            return "null";
        case JsType::Boolean:
            return as_boolean() ? "true" : "false";
        case JsType::String:
            return as_string()->to_std_string();
        case JsType::Object:
            return as_object()->to_std_string();
        case JsType::Array:
            return as_array()->to_std_string();
        case JsType::Function:
            return as_function()->to_std_string();
        case JsType::Iterator:
            return as_iterator()->to_std_string();
        case JsType::AsyncIterator:
            return as_async_iterator()->to_std_string();
        case JsType::Promise:
            return as_promise()->to_std_string();
        case JsType::Symbol:
            return as_symbol()->to_std_string();
        case JsType::DataDescriptor:
            return as_data_descriptor()->value.to_std_string();
        case JsType::AccessorDescriptor:
        {
            if (as_accessor_descriptor()->get.has_value())
                return as_accessor_descriptor()->get.value()(*this, {}).to_std_string();
            else
                return "undefined";
        }
        case JsType::Number:
        {
            double num = as_double();
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

    inline void AnyValue::set_prototype(const AnyValue &proto)
    {
        if (is_object())
        {
            as_object()->proto = proto;
        }
        else if (is_array())
        {
            as_array()->proto = proto;
        }
        else if (is_function())
        {
            as_function()->proto = proto;
        }
        else if (is_uninitialized())
        {
            Exception::throw_uninitialized_reference("#<Object>");
        }
    }

    // AnyValue::call implementation
    inline AnyValue AnyValue::call(const AnyValue &thisVal, std::span<const AnyValue> args, const std::optional<std::string> &expr) const
    {
        if (!is_function())
        {
            throw Exception::make_exception(expr.value_or(to_std_string()) + " is not a function", "TypeError");
        }
        return as_function()->call(thisVal, args);
    }

    // AnyValue::construct implementation
    inline AnyValue AnyValue::construct(std::span<const AnyValue> args, const std::optional<std::string> &name) const
    {
        if (!is_function() || !as_function()->is_constructor)
        {
            throw Exception::make_exception(name.value_or(to_std_string()) + " is not a constructor", "TypeError");
        }

        // 1. Get prototype
        AnyValue proto = get_own_property("prototype");
        if (!proto.is_object())
        {
            proto = AnyValue::make_object({});
        }

        // 2. Create instance
        AnyValue instance = AnyValue::make_object_with_proto({}, proto);

        // 3. Call function
        AnyValue result = call(instance, args);

        // 4. Return result if object, else instance
        if (result.is_object() || result.is_function() || result.is_array() || result.is_promise())
        {
            return result;
        }
        return instance;
    }

}
