#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <memory>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cmath>
#include <optional>
#include <coroutine>

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/iterator.hpp"
#include "values/promise.hpp"
#include "values/symbol.hpp"
#include "values/string.hpp"
#include "exception.hpp"
#include "values/descriptors.hpp"
#include "utils/well_known_symbols.hpp"

namespace jspp
{
    enum class JsType : uint8_t
    {
        Undefined = 0,
        Null = 1,
        Uninitialized = 2,
        Boolean = 3,
        Number = 4,
        String = 5,
        Object = 6,
        Array = 7,
        Function = 8,
        Iterator = 9,
        Symbol = 10,
        Promise = 11,
        DataDescriptor = 12,
        AccessorDescriptor = 13,
    };

    // Tagged storage with a union for payload
    struct TaggedValue
    {
        JsType type;
        union
        {
            JsUndefined undefined;
            JsNull null;
            JsUninitialized uninitialized;
            bool boolean;
            double number;
            std::shared_ptr<JsString> str;
            std::shared_ptr<JsObject> object;
            std::shared_ptr<JsArray> array;
            std::shared_ptr<JsFunction> function;
            std::shared_ptr<JsIterator<AnyValue>> iterator;
            std::shared_ptr<JsSymbol> symbol;
            std::shared_ptr<JsPromise> promise;
            std::shared_ptr<DataDescriptor> data_desc;
            std::shared_ptr<AccessorDescriptor> accessor_desc;
        };

        TaggedValue() noexcept : type(JsType::Undefined), undefined{} {}
        ~TaggedValue() {}
    };

    class AnyValue
    {
    private:
        TaggedValue storage;

        void destroy_value() noexcept
        {
            switch (storage.type)
            {
            case JsType::String:
                storage.str.~shared_ptr();
                break;
            case JsType::Object:
                storage.object.~shared_ptr();
                break;
            case JsType::Array:
                storage.array.~shared_ptr();
                break;
            case JsType::Function:
                storage.function.~shared_ptr();
                break;
            case JsType::Iterator:
                storage.iterator.~shared_ptr();
                break;
            case JsType::Symbol:
                storage.symbol.~shared_ptr();
                break;
            case JsType::Promise:
                storage.promise.~shared_ptr();
                break;
            case JsType::DataDescriptor:
                storage.data_desc.~shared_ptr();
                break;
            case JsType::AccessorDescriptor:
                storage.accessor_desc.~shared_ptr();
                break;
            default:
                break;
            }
        }

        void reset_to_undefined() noexcept
        {
            destroy_value();
            storage.type = JsType::Undefined;
            storage.undefined = JsUndefined{};
        }

        void move_from(AnyValue &other) noexcept
        {
            storage.type = other.storage.type;
            switch (other.storage.type)
            {
            case JsType::Undefined:
                storage.undefined = JsUndefined{};
                break;
            case JsType::Null:
                storage.null = JsNull{};
                break;
            case JsType::Uninitialized:
                storage.uninitialized = JsUninitialized{};
                break;
            case JsType::Boolean:
                storage.boolean = other.storage.boolean;
                break;
            case JsType::Number:
                storage.number = other.storage.number;
                break;
            case JsType::String:
                new (&storage.str) std::shared_ptr<JsString>(std::move(other.storage.str));
                break;
            case JsType::Object:
                new (&storage.object) std::shared_ptr<JsObject>(std::move(other.storage.object));
                break;
            case JsType::Array:
                new (&storage.array) std::shared_ptr<JsArray>(std::move(other.storage.array));
                break;
            case JsType::Function:
                new (&storage.function) std::shared_ptr<JsFunction>(std::move(other.storage.function));
                break;
            case JsType::Iterator:
                new (&storage.iterator) std::shared_ptr<JsIterator<AnyValue>>(std::move(other.storage.iterator));
                break;
            case JsType::Symbol:
                new (&storage.symbol) std::shared_ptr<JsSymbol>(std::move(other.storage.symbol));
                break;
            case JsType::Promise:
                new (&storage.promise) std::shared_ptr<JsPromise>(std::move(other.storage.promise));
                break;
            case JsType::DataDescriptor:
                new (&storage.data_desc) std::shared_ptr<DataDescriptor>(std::move(other.storage.data_desc));
                break;
            case JsType::AccessorDescriptor:
                new (&storage.accessor_desc) std::shared_ptr<AccessorDescriptor>(std::move(other.storage.accessor_desc));
                break;
            }
        }

        void copy_from(const AnyValue &other)
        {
            storage.type = other.storage.type;
            switch (other.storage.type)
            {
            case JsType::Undefined:
                storage.undefined = JsUndefined{};
                break;
            case JsType::Null:
                storage.null = JsNull{};
                break;
            case JsType::Uninitialized:
                storage.uninitialized = JsUninitialized{};
                break;
            case JsType::Boolean:
                storage.boolean = other.storage.boolean;
                break;
            case JsType::Number:
                storage.number = other.storage.number;
                break;
            case JsType::String:
                new (&storage.str) std::shared_ptr<JsString>(std::make_shared<JsString>(*other.storage.str));
                break;
            case JsType::Object:
                new (&storage.object) std::shared_ptr<JsObject>(other.storage.object); // shallow copy
                break;
            case JsType::Array:
                new (&storage.array) std::shared_ptr<JsArray>(other.storage.array); // shallow copy
                break;
            case JsType::Function:
                new (&storage.function) std::shared_ptr<JsFunction>(other.storage.function); // shallow copy
                break;
            case JsType::Iterator:
                new (&storage.iterator) std::shared_ptr<JsIterator<AnyValue>>(other.storage.iterator); // shallow copy
                break;
            case JsType::Symbol:
                new (&storage.symbol) std::shared_ptr<JsSymbol>(other.storage.symbol); // shallow copy (shared)
                break;
            case JsType::Promise:
                new (&storage.promise) std::shared_ptr<JsPromise>(other.storage.promise); // shallow copy
                break;
            case JsType::DataDescriptor:
                new (&storage.data_desc) std::shared_ptr<DataDescriptor>(other.storage.data_desc); // shallow copy
                break;
            case JsType::AccessorDescriptor:
                new (&storage.accessor_desc) std::shared_ptr<AccessorDescriptor>(other.storage.accessor_desc); // shallow copy
                break;
            }
        }

    public:
        // default ctor (Undefined)
        AnyValue() noexcept
        {
            storage.type = JsType::Undefined;
            storage.undefined = JsUndefined{};
        }

        // 1. Destructor
        ~AnyValue() noexcept
        {
            destroy_value();
        }

        // 2. Copy Constructor (deep copy)
        AnyValue(const AnyValue &other)
        {
            copy_from(other);
        }

        // 3. Copy Assignment Operator
        AnyValue &operator=(const AnyValue &other)
        {
            if (this != &other)
            {
                destroy_value();
                copy_from(other);
            }
            return *this;
        }

        // 4. Move Constructor
        AnyValue(AnyValue &&other) noexcept
        {
            storage.type = JsType::Undefined;
            storage.undefined = JsUndefined{};
            move_from(other);
            other.reset_to_undefined();
        }

        // 5. Move Assignment Operator
        AnyValue &operator=(AnyValue &&other) noexcept
        {
            if (this != &other)
            {
                destroy_value();
                move_from(other);
                other.reset_to_undefined();
            }
            return *this;
        }

        friend void swap(AnyValue &a, AnyValue &b) noexcept
        {
            AnyValue tmp(std::move(a));
            a = std::move(b);
            b = std::move(tmp);
        }

        // factories -------------------------------------------------------
        static AnyValue make_number(double d) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Number;
            v.storage.number = d;
            return v;
        }
        static AnyValue make_nan() noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Number;
            v.storage.number = std::numeric_limits<double>::quiet_NaN();
            return v;
        }
        static AnyValue make_uninitialized() noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Uninitialized;
            v.storage.uninitialized = JsUninitialized{};
            return v;
        }
        static AnyValue make_undefined() noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Undefined;
            v.storage.undefined = JsUndefined{};
            return v;
        }
        static AnyValue make_null() noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Null;
            v.storage.null = JsNull{};
            return v;
        }
        static AnyValue make_boolean(bool b) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Boolean;
            v.storage.boolean = b;
            return v;
        }
        static AnyValue make_string(const std::string &raw_s) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::String;
            new (&v.storage.str) std::shared_ptr<JsString>(std::make_shared<JsString>(raw_s));
            return v;
        }
        static AnyValue make_object(const std::map<std::string, AnyValue> &props) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Object;
            new (&v.storage.object) std::shared_ptr<JsObject>(std::make_shared<JsObject>(props));
            return v;
        }
        static AnyValue make_object_with_proto(const std::map<std::string, AnyValue> &props, const AnyValue &proto) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Object;
            auto protoPtr = std::make_shared<AnyValue>(proto);
            new (&v.storage.object) std::shared_ptr<JsObject>(std::make_shared<JsObject>(props, protoPtr));
            return v;
        }
        static AnyValue make_array(const std::vector<std::optional<AnyValue>> &dense) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Array;
            new (&v.storage.array) std::shared_ptr<JsArray>(std::make_shared<JsArray>(dense));
            return v;
        }
        static AnyValue make_array(std::vector<std::optional<AnyValue>> &&dense) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Array;
            new (&v.storage.array) std::shared_ptr<JsArray>(std::make_shared<JsArray>(std::move(dense)));
            return v;
        }
        static AnyValue make_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt, bool is_constructor = true) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(call, name, std::unordered_map<std::string, AnyValue>{}, false, is_constructor));

            auto proto = make_object({});
            proto.set_own_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.set_own_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_class(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            // use Constructor A with is_cls = true
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(call, name, std::unordered_map<std::string, AnyValue>{}, true));

            auto proto = make_object({});
            proto.set_own_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.set_own_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_generator(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            // use Constructor B with is_gen = true
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(call, true, name));

            auto proto = make_object({});
            proto.set_own_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.set_own_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_async_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            // use Constructor C with is_async_func = true
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(call, false, true, name));

            auto proto = make_object({});
            proto.set_own_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.set_own_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_symbol(const std::string &description = "") noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Symbol;
            new (&v.storage.symbol) std::shared_ptr<JsSymbol>(std::make_shared<JsSymbol>(description));
            return v;
        }
        static AnyValue make_promise(const JsPromise &promise) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Promise;
            new (&v.storage.promise) std::shared_ptr<JsPromise>(std::make_shared<JsPromise>(promise));
            return v;
        }
        static AnyValue make_data_descriptor(const AnyValue &value, bool writable, bool enumerable, bool configurable) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::DataDescriptor;
            new (&v.storage.data_desc) std::shared_ptr<DataDescriptor>(std::make_shared<DataDescriptor>(std::make_shared<AnyValue>(value), writable, enumerable, configurable));
            return v;
        }
        static AnyValue make_accessor_descriptor(const std::optional<std::function<AnyValue(const AnyValue &, const std::vector<AnyValue> &)>> &get,
                                                 const std::optional<std::function<AnyValue(const AnyValue &, const std::vector<AnyValue> &)>> &set,
                                                 bool enumerable,
                                                 bool configurable) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::AccessorDescriptor;
            new (&v.storage.accessor_desc) std::shared_ptr<AccessorDescriptor>(std::make_shared<AccessorDescriptor>(get, set, enumerable, configurable));
            return v;
        }

        static AnyValue from_symbol(std::shared_ptr<JsSymbol> sym) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Symbol;
            new (&v.storage.symbol) std::shared_ptr<JsSymbol>(std::move(sym));
            return v;
        }
        static AnyValue from_string(std::shared_ptr<JsString> str) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::String;
            new (&v.storage.str) std::shared_ptr<JsString>(std::move(str));
            return v;
        }
        static AnyValue from_iterator(JsIterator<AnyValue> &&iterator) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Iterator;
            new (&v.storage.iterator) std::shared_ptr<JsIterator<AnyValue>>(std::make_shared<JsIterator<AnyValue>>(std::move(iterator)));
            return v;
        }
        static AnyValue from_iterator_ref(JsIterator<AnyValue> *iterator) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Iterator;
            new (&v.storage.iterator) std::shared_ptr<JsIterator<AnyValue>>(iterator, [](JsIterator<AnyValue> *) {});
            return v;
        }

        // PROPERTY RESOLUTION HELPERS ---------------------------------------
        static AnyValue resolve_property_for_read(const AnyValue &val, const AnyValue &thisVal, const std::string &propName) noexcept
        {
            switch (val.storage.type)
            {
            case JsType::DataDescriptor:
            {
                return *(val.storage.data_desc->value);
            }
            case JsType::AccessorDescriptor:
            {
                if (val.storage.accessor_desc->get.has_value())
                    return val.storage.accessor_desc->get.value()(thisVal, {});
                else
                {
                    static AnyValue undefined = AnyValue{};
                    return undefined;
                }
            }
            default:
            {
                return val;
            }
            }
        }
        static AnyValue resolve_property_for_write(AnyValue &val, const AnyValue &thisVal, const AnyValue &new_val, const std::string &propName)
        {
            switch (val.storage.type)
            {
            case JsType::DataDescriptor:
            {
                if (val.storage.data_desc->writable)
                {
                    *(val.storage.data_desc->value) = new_val;
                    return new_val;
                }
                else
                {
                    throw Exception::make_exception("Cannot assign to read only property '" + propName + "' of object '#<Object>'", "TypeError");
                }
            }
            case JsType::AccessorDescriptor:
            {
                if (val.storage.accessor_desc->set.has_value())
                {
                    val.storage.accessor_desc->set.value()(thisVal, {new_val});
                    return new_val;
                }
                else
                {
                    throw Exception::make_exception("Cannot set property of #<Object> which has only a getter", "TypeError");
                }
            }
            default:
            {
                val = new_val;
                return new_val;
            }
            }
        }

        // TYPE CHECKERS AND ACCESSORS ---------------------------------------
        JsType get_type() const noexcept { return storage.type; }
        bool is_number() const noexcept { return storage.type == JsType::Number; }
        bool is_string() const noexcept { return storage.type == JsType::String; }
        bool is_object() const noexcept { return storage.type == JsType::Object; }
        bool is_array() const noexcept { return storage.type == JsType::Array; }
        bool is_function() const noexcept { return storage.type == JsType::Function; }
        bool is_iterator() const noexcept { return storage.type == JsType::Iterator; }
        bool is_boolean() const noexcept { return storage.type == JsType::Boolean; }
        bool is_symbol() const noexcept { return storage.type == JsType::Symbol; }
        bool is_promise() const noexcept { return storage.type == JsType::Promise; }
        bool is_null() const noexcept { return storage.type == JsType::Null; }
        bool is_undefined() const noexcept { return storage.type == JsType::Undefined; }
        bool is_uninitialized() const noexcept { return storage.type == JsType::Uninitialized; }
        bool is_data_descriptor() const noexcept { return storage.type == JsType::DataDescriptor; }
        bool is_accessor_descriptor() const noexcept { return storage.type == JsType::AccessorDescriptor; }
        bool is_generator() const noexcept { return storage.type == JsType::Function && storage.function->is_generator; }

        // --- TYPE CASTERS
        double as_double() const noexcept
        {
            assert(is_number());
            return storage.number;
        }
        bool as_boolean() const noexcept
        {
            assert(is_boolean());
            return storage.boolean;
        }
        JsString *as_string() const noexcept
        {
            assert(is_string());
            return storage.str.get();
        }
        JsObject *as_object() const noexcept
        {
            assert(is_object());
            return storage.object.get();
        }
        JsArray *as_array() const noexcept
        {
            assert(is_array());
            return storage.array.get();
        }
        JsFunction *as_function(const std::optional<std::string> &expression = std::nullopt) const
        {
            if (is_function())
                return storage.function.get();
            throw Exception::make_exception(expression.value_or(to_std_string()) + " is not a function", "TypeError");
        }
        JsSymbol *as_symbol() const noexcept
        {
            assert(is_symbol());
            return storage.symbol.get();
        }
        JsPromise *as_promise() const noexcept
        {
            assert(is_promise());
            return storage.promise.get();
        }
        std::shared_ptr<JsIterator<AnyValue>> as_iterator() const
        {
            assert(is_iterator());
            return storage.iterator; // Returns the shared_ptr, incrementing ref count
        }
        DataDescriptor *as_data_descriptor() const noexcept
        {
            assert(is_data_descriptor());
            return storage.data_desc.get();
        }
        AccessorDescriptor *as_accessor_descriptor() const noexcept
        {
            assert(is_accessor_descriptor());
            return storage.accessor_desc.get();
        }

        // --- CO_AWAIT Operator ---
        auto operator co_await() const;

        // --- PROPERTY ACCESS OPERATORS
        AnyValue get_own_property(const std::string &key) const;
        AnyValue get_own_property(uint32_t idx) const noexcept;
        AnyValue get_own_property(const AnyValue &key) const noexcept;
        // for getting values with a specific receiver (used in inheritance chains)
        AnyValue get_property_with_receiver(const std::string &key, const AnyValue &receiver) const;
        // for setting values
        AnyValue set_own_property(const std::string &key, const AnyValue &value) const;
        AnyValue set_own_property(uint32_t idx, const AnyValue &value) const;
        AnyValue set_own_property(const AnyValue &key, const AnyValue &value) const;

        // --- DEFINERS (Object.defineProperty semantics)
        void define_data_property(const std::string &key, const AnyValue &value);
        void define_data_property(const AnyValue &key, const AnyValue &value);
        void define_data_property(const std::string &key, const AnyValue &value, bool writable, bool enumerable, bool configurable);
        void define_getter(const std::string &key, const AnyValue &getter);
        void define_getter(const AnyValue &key, const AnyValue &getter);
        void define_setter(const std::string &key, const AnyValue &setter);
        void define_setter(const AnyValue &key, const AnyValue &setter);

        // --- HELPERS
        const bool is_truthy() const noexcept;
        const bool is_strictly_equal_to_primitive(const AnyValue &other) const noexcept;
        const bool is_equal_to_primitive(const AnyValue &other) const noexcept;

        const AnyValue is_strictly_equal_to(const AnyValue &other) const noexcept;
        const AnyValue is_equal_to(const AnyValue &other) const noexcept;
        const AnyValue not_strictly_equal_to(const AnyValue &other) const noexcept;
        const AnyValue not_equal_to(const AnyValue &other) const noexcept;

        const AnyValue construct(const std::vector<AnyValue> &args, const std::optional<std::string> &name) const;
        void set_prototype(const AnyValue &proto);
        const std::string to_std_string() const noexcept;
    };

    // Inline implementation of operator co_await
    inline auto AnyValue::operator co_await() const
    {
        return AnyValueAwaiter{*this};
    }
}
