#include "jspp.hpp"
#include "any_value.hpp"
#include "values/iterator.hpp"
#include "values/async_iterator.hpp"
#include "values/string.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/promise.hpp"
#include "values/symbol.hpp"
#include "values/descriptors.hpp"
#include "exception.hpp"

namespace jspp
{

    // --- AnyValue methods that were in any_value_helpers.hpp ---

    bool AnyValue::operator==(const AnyValue &other) const noexcept
    {
        return storage == other.storage;
    }
    bool AnyValue::operator==(const std::string &other) const noexcept
    {
        if (!is_string())
            return false;
        return as_string()->value == other;
    }
    bool AnyValue::operator<(const AnyValue &other) const noexcept
    {
        return storage < other.storage;
    }

    JsString *AnyValue::as_string() const noexcept { return static_cast<JsString *>(get_ptr()); }
    JsObject *AnyValue::as_object() const noexcept { return static_cast<JsObject *>(get_ptr()); }
    JsArray *AnyValue::as_array() const noexcept { return static_cast<JsArray *>(get_ptr()); }
    JsFunction *AnyValue::as_function() const noexcept { return static_cast<JsFunction *>(get_ptr()); }
    JsSymbol *AnyValue::as_symbol() const noexcept { return static_cast<JsSymbol *>(get_ptr()); }
    JsPromise *AnyValue::as_promise() const noexcept { return static_cast<JsPromise *>(get_ptr()); }
    DataDescriptor *AnyValue::as_data_descriptor() const noexcept { return static_cast<DataDescriptor *>(get_ptr()); }
    AccessorDescriptor *AnyValue::as_accessor_descriptor() const noexcept { return static_cast<AccessorDescriptor *>(get_ptr()); }

    bool AnyValue::is_generator() const noexcept { return is_function() && as_function()->is_generator; }

    // --- AnyValue FACTORIES ---
    AnyValue AnyValue::make_string(const std::string &raw_s) noexcept
    {
        return from_ptr(new JsString(raw_s));
    }
    AnyValue AnyValue::make_object(std::initializer_list<std::pair<std::string, AnyValue>> props) noexcept
    {
        return from_ptr(new JsObject(props, make_null()));
    }
    AnyValue AnyValue::make_object(const std::map<std::string, AnyValue> &props) noexcept
    {
        return from_ptr(new JsObject(props, make_null()));
    }
    AnyValue AnyValue::make_array(std::span<const AnyValue> dense) noexcept
    {
        std::vector<AnyValue> vec;
        vec.reserve(dense.size());
        for (const auto &item : dense)
            vec.push_back(item);
        return from_ptr(new JsArray(std::move(vec)));
    }
    AnyValue AnyValue::make_array(const std::vector<AnyValue> &dense) noexcept
    {
        return from_ptr(new JsArray(dense));
    }
    AnyValue AnyValue::make_array(std::vector<AnyValue> &&dense) noexcept
    {
        return from_ptr(new JsArray(std::move(dense)));
    }
    AnyValue AnyValue::make_function(const JsFunctionCallable &call, const std::optional<std::string> &name, bool is_constructor) noexcept
    {
        auto v = from_ptr(new JsFunction(call, name, {}, {}, false, is_constructor));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    AnyValue AnyValue::make_class(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, name, {}, {}, true));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    AnyValue AnyValue::make_generator(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, true, name, {}, {}, false));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    AnyValue AnyValue::make_async_function(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, false, true, name, {}, {}, false));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    AnyValue AnyValue::make_async_generator(const JsFunctionCallable &call, const std::optional<std::string> &name) noexcept
    {
        auto v = from_ptr(new JsFunction(call, true, true, name, {}, {}, false));
        auto proto = make_object({});
        proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
        v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));
        return v;
    }
    AnyValue AnyValue::make_symbol(const std::string &description) noexcept
    {
        return from_ptr(new JsSymbol(description));
    }
    AnyValue AnyValue::make_promise(const JsPromise &promise) noexcept
    {
        auto p = new JsPromise();
        *p = promise;
        return from_ptr(p);
    }
    AnyValue AnyValue::make_data_descriptor(AnyValue value, bool writable, bool enumerable, bool configurable) noexcept
    {
        return from_ptr(new DataDescriptor(value, writable, enumerable, configurable));
    }
    AnyValue AnyValue::make_accessor_descriptor(const std::optional<std::function<AnyValue(AnyValue, std::span<const AnyValue>)>> &get,
                                                const std::optional<std::function<AnyValue(AnyValue, std::span<const AnyValue>)>> &set,
                                                bool enumerable,
                                                bool configurable) noexcept
    {
        return from_ptr(new AccessorDescriptor(get, set, enumerable, configurable));
    }

    AnyValue AnyValue::from_symbol(JsSymbol *sym) noexcept
    {
        return from_ptr(sym);
    }
    AnyValue AnyValue::from_string(JsString *str) noexcept
    {
        return from_ptr(str);
    }
    AnyValue AnyValue::from_promise(JsPromise &&promise) noexcept
    {
        auto it = new JsPromise(std::move(promise));
        return from_ptr(it);
    }
    AnyValue AnyValue::from_iterator(JsIterator<AnyValue> &&iterator) noexcept
    {
        auto it = new JsIterator<AnyValue>(std::move(iterator));
        return from_ptr(it);
    }
    AnyValue AnyValue::from_iterator_ref(JsIterator<AnyValue> *iterator) noexcept
    {
        return from_ptr(iterator);
    }
    AnyValue AnyValue::from_async_iterator(JsAsyncIterator<AnyValue> &&iterator) noexcept
    {
        auto it = new JsAsyncIterator<AnyValue>(std::move(iterator));
        return from_ptr(it);
    }

    AnyValue AnyValue::resolve_property_for_read(const AnyValue &val, AnyValue thisVal, const std::string &propName) noexcept
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

    AnyValue AnyValue::resolve_property_for_write(AnyValue &val, AnyValue thisVal, const AnyValue &new_val, const std::string &propName)
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
                throw Exception::make_exception("Cannot assign to read only property '" + propName + "'", "TypeError");
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

    std::string AnyValue::to_std_string() const
    {
        switch (get_type())
        {
        case JsType::Undefined:
            return "undefined";
        case JsType::Null:
            return "null";
        case JsType::Boolean:
            return JsBoolean::to_std_string(as_boolean());
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
            return JsNumber::to_std_string(as_double());
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return "";
        }
    }

    AnyValue &AnyValue::set_prototype(AnyValue proto)
    {
        if (is_object())
            as_object()->proto = proto;
        else if (is_array())
            as_array()->proto = proto;
        else if (is_function())
            as_function()->proto = proto;
        else if (is_uninitialized())
            Exception::throw_uninitialized_reference("#<Object>");
        return *this;
    }

    AnyValue AnyValue::call(AnyValue thisVal, std::span<const AnyValue> args, const std::optional<std::string> &expr) const
    {
        if (!is_function())
            throw Exception::make_exception(expr.value_or(to_std_string()) + " is not a function", "TypeError");
        return as_function()->call(thisVal, args);
    }
    AnyValue AnyValue::optional_call(AnyValue thisVal, std::span<const AnyValue> args, const std::optional<std::string> &expr) const
    {
        if (is_null() || is_undefined())
            return Constants::UNDEFINED;
        return as_function()->call(thisVal, args);
    }

    AnyValue AnyValue::construct(std::span<const AnyValue> args, const std::optional<std::string> &name) const
    {
        if (!is_function() || !as_function()->is_constructor)
        {
            throw Exception::make_exception(name.value_or(to_std_string()) + " is not a constructor", "TypeError");
        }
        AnyValue proto = get_own_property("prototype");
        if (!proto.is_object())
            proto = AnyValue::make_object({});
        AnyValue instance = AnyValue::make_object({}).set_prototype(proto);
        AnyValue result = call(instance, args);
        if (result.is_object() || result.is_function() || result.is_array() || result.is_promise())
            return result;
        return instance;
    }

    // --- AnyValue methods that were in any_value_access.hpp ---

    AnyValue AnyValue::get_own_property(const std::string &key) const
    {
        return get_property_with_receiver(key, *this);
    }

    AnyValue AnyValue::get_own_property_descriptor(const AnyValue &key) const
    {
        if (key.is_symbol())
        {
            switch (get_type())
            {
            case JsType::Object:
            {
                auto obj = as_object();
                auto it = obj->symbol_props.find(key);
                if (it != obj->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            case JsType::Array:
            {
                auto arr = as_array();
                auto it = arr->symbol_props.find(key);
                if (it != arr->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            case JsType::Function:
            {
                auto func = as_function();
                auto it = func->symbol_props.find(key);
                if (it != func->symbol_props.end())
                    return it->second;
                return Constants::UNDEFINED;
            }
            default:
                return Constants::UNDEFINED;
            }
        }

        std::string key_str = key.to_std_string();
        switch (get_type())
        {
        case JsType::Object:
        {
            auto obj = as_object();
            if (obj->deleted_keys.count(key_str))
                return Constants::UNDEFINED;
            auto offset = obj->shape->get_offset(key_str);
            if (offset.has_value())
                return obj->storage[offset.value()];
            return Constants::UNDEFINED;
        }
        case JsType::Array:
        {
            auto arr = as_array();
            if (key_str == "length")
                return AnyValue::make_number(arr->length);
            if (JsArray::is_array_index(key_str))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key_str));
                if (idx < arr->dense.size() && !arr->dense[idx].is_uninitialized())
                    return arr->dense[idx];
                if (arr->sparse.count(idx))
                    return arr->sparse[idx];
            }
            if (arr->props.count(key_str))
                return arr->props.at(key_str);
            return Constants::UNDEFINED;
        }
        case JsType::Function:
        {
            auto func = as_function();
            if (func->props.count(key_str))
                return func->props.at(key_str);
            return Constants::UNDEFINED;
        }
        case JsType::String:
        {
            auto str = as_string();
            if (key_str == "length")
                return AnyValue::make_number(str->value.length());
            if (JsArray::is_array_index(key_str))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key_str));
                if (idx < str->value.length())
                    return AnyValue::make_string(std::string(1, str->value[idx]));
            }
            return Constants::UNDEFINED;
        }
        default:
            return Constants::UNDEFINED;
        }
    }

    bool AnyValue::has_property(const std::string &key) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->has_property(key);
        case JsType::Array:
            return as_array()->has_property(key);
        case JsType::Function:
            return as_function()->has_property(key);
        case JsType::Promise:
            return as_promise()->get_property(key, *this).is_undefined() == false;
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->get_property(key, *this).is_undefined() == false;
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->get_property(key, *this).is_undefined() == false;
        case JsType::Symbol:
            return SymbolPrototypes::get(key).has_value();
        case JsType::String:
            if (key == "length")
                return true;
            if (JsArray::is_array_index(key))
            {
                uint32_t idx = static_cast<uint32_t>(std::stoull(key));
                return idx < as_string()->value.length();
            }
            return StringPrototypes::get(key).has_value();
        case JsType::Number:
            return NumberPrototypes::get(key).has_value();
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
            return false;
        default:
            return false;
        }
    }

    bool AnyValue::has_property(const AnyValue &key) const
    {
        if (key.is_symbol())
        {
            switch (get_type())
            {
            case JsType::Object:
                return as_object()->has_symbol_property(key);
            case JsType::Array:
                return as_array()->has_symbol_property(key);
            case JsType::Function:
                return as_function()->has_symbol_property(key);
            case JsType::Promise:
                return as_promise()->has_symbol_property(key);
            case JsType::Iterator:
                return static_cast<JsIterator<AnyValue> *>(get_ptr())->has_symbol_property(key);
            case JsType::AsyncIterator:
                return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->has_symbol_property(key);
            default:
                return false;
            }
        }
        return has_property(key.to_std_string());
    }

    AnyValue AnyValue::get_own_property(uint32_t idx) const
    {
        if (is_array())
            return as_array()->get_property(idx);
        if (is_string())
            return as_string()->get_property(idx);
        return get_own_property(std::to_string(idx));
    }

    AnyValue AnyValue::get_own_property(const AnyValue &key) const
    {
        if (key.is_number() && is_array())
            return as_array()->get_property(key.as_double());
        if (key.is_number() && is_string())
            return as_string()->get_property(key.as_double());
        if (key.is_symbol())
            return get_own_symbol_property(key);
        return get_own_property(key.to_std_string());
    }

    AnyValue AnyValue::get_own_symbol_property(const AnyValue &key) const
    {
        return get_symbol_property_with_receiver(key, *this);
    }

    AnyValue AnyValue::get_property_with_receiver(const std::string &key, AnyValue receiver) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->get_property(key, receiver);
        case JsType::Array:
            return as_array()->get_property(key, receiver);
        case JsType::Function:
            return as_function()->get_property(key, receiver);
        case JsType::Promise:
            return as_promise()->get_property(key, receiver);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->get_property(key, receiver);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->get_property(key, receiver);
        case JsType::Symbol:
            return as_symbol()->get_property(key, receiver);
        case JsType::String:
            return as_string()->get_property(key, receiver);
        case JsType::Number:
        {
            auto proto_it = NumberPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key);
            return Constants::UNDEFINED;
        }
        case JsType::Boolean:
        {
            auto proto_it = BooleanPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key);
            return Constants::UNDEFINED;
        }
        case JsType::Undefined:
            throw Exception::make_exception("Cannot read properties of undefined (reading '" + key + "')", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot read properties of null (reading '" + key + "')", "TypeError");
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return Constants::UNDEFINED;
        }
    }

    AnyValue AnyValue::get_symbol_property_with_receiver(const AnyValue &key, AnyValue receiver) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->get_symbol_property(key, receiver);
        case JsType::Array:
            return as_array()->get_symbol_property(key, receiver);
        case JsType::Function:
            return as_function()->get_symbol_property(key, receiver);
        case JsType::Promise:
            return as_promise()->get_symbol_property(key, receiver);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->get_symbol_property(key, receiver);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->get_symbol_property(key, receiver);
        case JsType::Symbol:
        {
            auto proto_it = SymbolPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            return Constants::UNDEFINED;
        }
        case JsType::String:
        {
            auto proto_it = StringPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            return Constants::UNDEFINED;
        }
        case JsType::Number:
        {
            auto proto_it = NumberPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            return Constants::UNDEFINED;
        }
        case JsType::Boolean:
        {
            auto proto_it = BooleanPrototypes::get(key);
            if (proto_it.has_value())
                return AnyValue::resolve_property_for_read(proto_it.value(), receiver, key.to_std_string());
            return Constants::UNDEFINED;
        }
        case JsType::Undefined:
            throw Exception::make_exception("Cannot read properties of undefined (reading Symbol)", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot read properties of null (reading Symbol)", "TypeError");
        case JsType::Uninitialized:
            Exception::throw_uninitialized_reference("#<Object>");
        default:
            return Constants::UNDEFINED;
        }
    }

    AnyValue AnyValue::set_own_property(const std::string &key, AnyValue value) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->set_property(key, value, *this);
        case JsType::Array:
            return as_array()->set_property(key, value, *this);
        case JsType::Function:
            return as_function()->set_property(key, value, *this);
        case JsType::Promise:
            return as_promise()->set_property(key, value, *this);
        case JsType::Undefined:
            throw Exception::make_exception("Cannot set properties of undefined (setting '" + key + "')", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot set properties of null (setting '" + key + "')", "TypeError");
        default:
            return value;
        }
    }

    AnyValue AnyValue::set_own_property(uint32_t idx, AnyValue value) const
    {
        if (is_array())
            return as_array()->set_property(idx, value);
        return set_own_property(std::to_string(idx), value);
    }

    AnyValue AnyValue::set_own_property(const AnyValue &key, AnyValue value) const
    {
        if (key.is_number() && is_array())
            return as_array()->set_property(key.as_double(), value);
        if (key.is_symbol())
            return set_own_symbol_property(key, value);
        return set_own_property(key.to_std_string(), value);
    }

    AnyValue AnyValue::set_own_symbol_property(const AnyValue &key, AnyValue value) const
    {
        switch (get_type())
        {
        case JsType::Object:
            return as_object()->set_symbol_property(key, value, *this);
        case JsType::Array:
            return as_array()->set_symbol_property(key, value, *this);
        case JsType::Function:
            return as_function()->set_symbol_property(key, value, *this);
        case JsType::Promise:
            return as_promise()->set_symbol_property(key, value, *this);
        case JsType::Iterator:
            return static_cast<JsIterator<AnyValue> *>(get_ptr())->set_symbol_property(key, value, *this);
        case JsType::AsyncIterator:
            return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr())->set_symbol_property(key, value, *this);
        case JsType::Undefined:
            throw Exception::make_exception("Cannot set properties of undefined (setting Symbol)", "TypeError");
        case JsType::Null:
            throw Exception::make_exception("Cannot set properties of null (setting Symbol)", "TypeError");
        default:
            return value;
        }
    }

    AnyValue AnyValue::call_own_property(const std::string &key, std::span<const AnyValue> args) const
    {
        return get_own_property(key).call((*this), args, key);
    }
    AnyValue AnyValue::call_own_property(uint32_t idx, std::span<const AnyValue> args) const
    {
        if (is_array())
            return as_array()->get_property(idx).call((*this), args, "[" + std::to_string(idx) + "]");
        if (is_string())
            return as_string()->get_property(idx).call((*this), args, "[" + std::to_string(idx) + "]");
        return call_own_property(std::to_string(idx), args);
    }
    AnyValue AnyValue::call_own_property(const AnyValue &key, std::span<const AnyValue> args) const
    {
        if (key.is_number() && is_array())
            return as_array()->get_property(key.as_double()).call((*this), args, "[" + key.to_std_string() + "]");
        if (key.is_number() && is_string())
            return as_string()->get_property(key.as_double()).call((*this), args, "[" + key.to_std_string() + "]");
        if (key.is_symbol())
            return get_own_symbol_property(key).call((*this), args, key.to_std_string());
        return call_own_property(key.to_std_string(), args);
    }

    // --- AnyValue methods that were in any_value_defines.hpp ---

    void AnyValue::define_data_property(const std::string &key, AnyValue value)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);
            if (offset.has_value())
                obj->storage[offset.value()] = value;
            else
            {
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(value);
            }
        }
        else if (is_function())
            as_function()->props[key] = value;
    }

    void AnyValue::define_data_property(const AnyValue &key, AnyValue value)
    {
        if (key.is_symbol())
        {
            if (is_object())
                as_object()->symbol_props[key] = value;
            else if (is_function())
                as_function()->symbol_props[key] = value;
        }
        else
            define_data_property(key.to_std_string(), value);
    }

    void AnyValue::define_data_property(const std::string &key, AnyValue value, bool writable, bool enumerable, bool configurable)
    {
        define_data_property(key, AnyValue::make_data_descriptor(value, writable, enumerable, configurable));
    }

    void AnyValue::define_getter(const std::string &key, AnyValue getter)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);
            if (offset.has_value())
            {
                auto &val = obj->storage[offset.value()];
                if (val.is_accessor_descriptor())
                {
                    auto desc = val.as_accessor_descriptor();
                    desc->get = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    { return getter.call(thisVal, args); };
                }
                else
                {
                    auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    { return getter.call(thisVal, args); };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
                }
            }
            else
            {
                auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                { return getter.call(thisVal, args); };
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true));
            }
        }
        else if (is_function())
        {
            auto &props = as_function()->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->get = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                { return getter.call(thisVal, args); };
            }
            else
            {
                auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                { return getter.call(thisVal, args); };
                props[key] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            }
        }
    }

    void AnyValue::define_data_property(const AnyValue &key, AnyValue value, bool writable, bool enumerable, bool configurable)
    {
        if (key.is_symbol())
        {
            auto desc = AnyValue::make_data_descriptor(value, writable, enumerable, configurable);
            if (is_object())
                as_object()->symbol_props[key] = desc;
            else if (is_function())
                as_function()->symbol_props[key] = desc;
        }
        else
            define_data_property(key.to_std_string(), value, writable, enumerable, configurable);
    }

    void AnyValue::define_getter(const AnyValue &key, AnyValue getter)
    {
        if (key.is_symbol())
        {
            auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
            { return getter.call(thisVal, args); };
            auto desc = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            if (is_object())
                as_object()->symbol_props[key] = desc;
            else if (is_function())
                as_function()->symbol_props[key] = desc;
        }
        else
            define_getter(key.to_std_string(), getter);
    }

    void AnyValue::define_setter(const std::string &key, AnyValue setter)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);
            if (offset.has_value())
            {
                auto &val = obj->storage[offset.value()];
                if (val.is_accessor_descriptor())
                {
                    auto desc = val.as_accessor_descriptor();
                    desc->set = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        if (args.empty())
                            return Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                }
                else
                {
                    auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                    {
                        if (args.empty())
                            return jspp::Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
                }
            }
            else
            {
                auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                {
                    if (args.empty())
                        return jspp::Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true));
            }
        }
        else if (is_function())
        {
            auto &props = as_function()->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->set = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
            }
            else
            {
                auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                {
                    if (args.empty())
                        return jspp::Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
            }
        }
    }

    void AnyValue::define_setter(const AnyValue &key, AnyValue setter)
    {
        if (key.is_symbol())
        {
            auto setFunc = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
            {
                if (args.empty())
                    return Constants::UNDEFINED;
                return setter.call(thisVal, args);
            };
            auto desc = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
            if (is_object())
                as_object()->symbol_props[key] = desc;
            else if (is_function())
                as_function()->symbol_props[key] = desc;
        }
        else
            define_setter(key.to_std_string(), setter);
    }

    // AnyValue iterator methods from runtime.cpp
    JsIterator<AnyValue> *AnyValue::as_iterator() const noexcept
    {
        return static_cast<JsIterator<AnyValue> *>(get_ptr());
    }

    JsAsyncIterator<AnyValue> *AnyValue::as_async_iterator() const noexcept
    {
        return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr());
    }

} // namespace jspp
