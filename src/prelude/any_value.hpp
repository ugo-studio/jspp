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
#include <variant>

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/iterator.hpp"
#include "values/promise.hpp"
#include "values/symbol.hpp"
#include "values/string.hpp"
#include "values/descriptors.hpp"
#include "exception.hpp"
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

    // The variant order MUST match JsType
    using AnyValueVariant = std::variant<
        JsUndefined,
        JsNull,
        JsUninitialized,
        bool,
        double,
        std::shared_ptr<JsString>,
        std::shared_ptr<JsObject>,
        std::shared_ptr<JsArray>,
        std::shared_ptr<JsFunction>,
        std::shared_ptr<JsIterator<AnyValue>>,
        std::shared_ptr<JsSymbol>,
        std::shared_ptr<JsPromise>,
        std::shared_ptr<DataDescriptor>,
        std::shared_ptr<AccessorDescriptor>>;

    class AnyValue
    {
    private:
        AnyValueVariant storage;

    public:
        // default ctor (Undefined)
        AnyValue() noexcept = default;

        // Copy/Move handled by std::variant
        AnyValue(const AnyValue &) = default;
        AnyValue(AnyValue &&) noexcept = default;
        AnyValue &operator=(const AnyValue &) = default;
        AnyValue &operator=(AnyValue &&) noexcept = default;

        ~AnyValue() = default;

        // Assignments
        AnyValue &operator=(const double &val)
        {
            storage = val;
            return *this;
        };

        friend void swap(AnyValue &a, AnyValue &b) noexcept
        {
            std::swap(a.storage, b.storage);
        }

        // --- FRIENDS for Optimized Operators
        friend AnyValue &operator+=(AnyValue &lhs, const AnyValue &rhs);
        friend AnyValue &operator-=(AnyValue &lhs, const AnyValue &rhs);
        friend AnyValue &operator*=(AnyValue &lhs, const AnyValue &rhs);
        friend AnyValue &operator/=(AnyValue &lhs, const AnyValue &rhs);
        friend AnyValue &operator%=(AnyValue &lhs, const AnyValue &rhs);
        friend AnyValue &operator++(AnyValue &val);
        friend AnyValue operator++(AnyValue &val, int);
        friend AnyValue &operator--(AnyValue &val);
        friend AnyValue operator--(AnyValue &val, int);

        // factories -------------------------------------------------------
        static AnyValue make_number(double d) noexcept
        {
            AnyValue v;
            v.storage = d;
            return v;
        }
        static AnyValue make_nan() noexcept
        {
            AnyValue v;
            v.storage = std::numeric_limits<double>::quiet_NaN();
            return v;
        }
        static AnyValue make_uninitialized() noexcept
        {
            AnyValue v;
            v.storage = JsUninitialized{};
            return v;
        }
        static AnyValue make_undefined() noexcept
        {
            AnyValue v;
            v.storage = JsUndefined{};
            return v;
        }
        static AnyValue make_null() noexcept
        {
            AnyValue v;
            v.storage = JsNull{};
            return v;
        }
        static AnyValue make_boolean(bool b) noexcept
        {
            AnyValue v;
            v.storage = b;
            return v;
        }
        static AnyValue make_string(const std::string &raw_s) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsString>(raw_s);
            return v;
        }
        static AnyValue make_object(std::initializer_list<std::pair<std::string, AnyValue>> props) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsObject>(props);
            return v;
        }
        static AnyValue make_object(const std::map<std::string, AnyValue> &props) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsObject>(props);
            return v;
        }
        static AnyValue make_object_with_proto(std::initializer_list<std::pair<std::string, AnyValue>> props, const AnyValue &proto) noexcept
        {
            AnyValue v;
            auto protoPtr = std::make_shared<AnyValue>(proto);
            v.storage = std::make_shared<JsObject>(props, protoPtr);
            return v;
        }
        static AnyValue make_object_with_proto(const std::map<std::string, AnyValue> &props, const AnyValue &proto) noexcept
        {
            AnyValue v;
            auto protoPtr = std::make_shared<AnyValue>(proto);
            v.storage = std::make_shared<JsObject>(props, protoPtr);
            return v;
        }
        static AnyValue make_array(std::span<const AnyValue> dense) noexcept
        {
            AnyValue v;
            std::vector<AnyValue> vec;
            vec.reserve(dense.size());
            for (const auto &item : dense)
                vec.push_back(item);
            v.storage = std::make_shared<JsArray>(std::move(vec));
            return v;
        }
        static AnyValue make_array(const std::vector<AnyValue> &dense) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsArray>(dense);
            return v;
        }
        static AnyValue make_array(std::vector<AnyValue> &&dense) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsArray>(std::move(dense));
            return v;
        }
        static AnyValue make_array_with_proto(std::span<const AnyValue> dense, const AnyValue &proto) noexcept
        {
            AnyValue v;
            std::vector<AnyValue> vec;
            vec.reserve(dense.size());
            for (const auto &item : dense)
                vec.push_back(item);
            v.storage = std::make_shared<JsArray>(std::move(vec));
            std::get<std::shared_ptr<JsArray>>(v.storage)->proto = std::make_shared<AnyValue>(proto);
            return v;
        }
        static AnyValue make_array_with_proto(const std::vector<AnyValue> &dense, const AnyValue &proto) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsArray>(dense);
            std::get<std::shared_ptr<JsArray>>(v.storage)->proto = std::make_shared<AnyValue>(proto);
            return v;
        }
        static AnyValue make_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt, bool is_constructor = true) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsFunction>(call, name, std::unordered_map<std::string, AnyValue>{}, false, is_constructor);

            auto proto = make_object({});
            proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_class(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            // use Constructor A with is_cls = true
            v.storage = std::make_shared<JsFunction>(call, name, std::unordered_map<std::string, AnyValue>{}, true);

            auto proto = make_object({});
            proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_generator(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            // use Constructor B with is_gen = true
            v.storage = std::make_shared<JsFunction>(call, true, name);

            auto proto = make_object({});
            proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_async_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept
        {
            AnyValue v;
            // use Constructor C with is_async_func = true
            v.storage = std::make_shared<JsFunction>(call, false, true, name);

            auto proto = make_object({});
            proto.define_data_property("constructor", AnyValue::make_data_descriptor(v, true, false, false));
            v.define_data_property("prototype", AnyValue::make_data_descriptor(proto, false, false, false));

            return v;
        }
        static AnyValue make_symbol(const std::string &description = "") noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsSymbol>(description);
            return v;
        }
        static AnyValue make_promise(const JsPromise &promise) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsPromise>(promise);
            return v;
        }
        static AnyValue make_data_descriptor(const AnyValue &value, bool writable, bool enumerable, bool configurable) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<DataDescriptor>(std::make_shared<AnyValue>(value), writable, enumerable, configurable);
            return v;
        }
        static AnyValue make_accessor_descriptor(const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &get,
                                                 const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &set,
                                                 bool enumerable,
                                                 bool configurable) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<AccessorDescriptor>(get, set, enumerable, configurable);
            return v;
        }

        static AnyValue from_symbol(std::shared_ptr<JsSymbol> sym) noexcept
        {
            AnyValue v;
            v.storage = std::move(sym);
            return v;
        }
        static AnyValue from_string(std::shared_ptr<JsString> str) noexcept
        {
            AnyValue v;
            v.storage = std::move(str);
            return v;
        }
        static AnyValue from_iterator(JsIterator<AnyValue> &&iterator) noexcept
        {
            AnyValue v;
            v.storage = std::make_shared<JsIterator<AnyValue>>(std::move(iterator));
            return v;
        }
        static AnyValue from_iterator_ref(JsIterator<AnyValue> *iterator) noexcept
        {
            AnyValue v;
            v.storage = std::shared_ptr<JsIterator<AnyValue>>(iterator, [](JsIterator<AnyValue> *) {});
            return v;
        }

        // PROPERTY RESOLUTION HELPERS ---------------------------------------
        static AnyValue resolve_property_for_read(const AnyValue &val, const AnyValue &thisVal, const std::string &propName) noexcept
        {
            switch (val.get_type())
            {
            case JsType::DataDescriptor:
            {
                return *(val.as_data_descriptor()->value);
            }
            case JsType::AccessorDescriptor:
            {
                const auto &accessor = val.as_accessor_descriptor();
                if (accessor->get.has_value())
                    return accessor->get.value()(thisVal, std::span<const AnyValue>{});
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
            switch (val.get_type())
            {
            case JsType::DataDescriptor:
            {
                const auto &data = val.as_data_descriptor();
                if (data->writable)
                {
                    *(data->value) = new_val;
                    return new_val;
                }
                else
                {
                    throw Exception::make_exception("Cannot assign to read only property '" + propName + "' of object '#<Object>'", "TypeError");
                }
            }
            case JsType::AccessorDescriptor:
            {
                const auto &accessor = val.as_accessor_descriptor();
                if (accessor->set.has_value())
                {
                    const AnyValue args[] = {new_val};
                    accessor->set.value()(thisVal, args);
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
        JsType get_type() const noexcept { return static_cast<JsType>(storage.index()); }
        bool is_number() const noexcept { return storage.index() == 4; }
        bool is_string() const noexcept { return storage.index() == 5; }
        bool is_object() const noexcept { return storage.index() == 6; }
        bool is_array() const noexcept { return storage.index() == 7; }
        bool is_function() const noexcept { return storage.index() == 8; }
        bool is_iterator() const noexcept { return storage.index() == 9; }
        bool is_boolean() const noexcept { return storage.index() == 3; }
        bool is_symbol() const noexcept { return storage.index() == 10; }
        bool is_promise() const noexcept { return storage.index() == 11; }
        bool is_null() const noexcept { return storage.index() == 1; }
        bool is_undefined() const noexcept { return storage.index() == 0; }
        bool is_uninitialized() const noexcept { return storage.index() == 2; }
        bool is_data_descriptor() const noexcept { return storage.index() == 12; }
        bool is_accessor_descriptor() const noexcept { return storage.index() == 13; }
        bool is_generator() const noexcept { return is_function() && as_function()->is_generator; }

        // --- TYPE CASTERS
        double as_double() const noexcept
        {
            return std::get<double>(storage);
        }
        bool as_boolean() const noexcept
        {
            return std::get<bool>(storage);
        }
        JsString *as_string() const noexcept
        {
            return std::get<std::shared_ptr<JsString>>(storage).get();
        }
        JsObject *as_object() const noexcept
        {
            return std::get<std::shared_ptr<JsObject>>(storage).get();
        }
        JsArray *as_array() const noexcept
        {
            return std::get<std::shared_ptr<JsArray>>(storage).get();
        }
        JsFunction *as_function(const std::optional<std::string> &expression = std::nullopt) const
        {
            if (is_function())
                return std::get<std::shared_ptr<JsFunction>>(storage).get();
            throw Exception::make_exception(expression.value_or(to_std_string()) + " is not a function", "TypeError");
        }
        JsSymbol *as_symbol() const noexcept
        {
            return std::get<std::shared_ptr<JsSymbol>>(storage).get();
        }
        JsPromise *as_promise() const noexcept
        {
            return std::get<std::shared_ptr<JsPromise>>(storage).get();
        }
        std::shared_ptr<JsIterator<AnyValue>> as_iterator() const
        {
            return std::get<std::shared_ptr<JsIterator<AnyValue>>>(storage);
        }
        DataDescriptor *as_data_descriptor() const noexcept
        {
            return std::get<std::shared_ptr<DataDescriptor>>(storage).get();
        }
        AccessorDescriptor *as_accessor_descriptor() const noexcept
        {
            return std::get<std::shared_ptr<AccessorDescriptor>>(storage).get();
        }

        // --- CO_AWAIT Operator ---
        auto operator co_await() const;

        // --- PROPERTY ACCESS OPERATORS
        bool has_property(const std::string &key) const;
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
        const AnyValue construct(std::span<const AnyValue> args, const std::optional<std::string> &name) const;
        void set_prototype(const AnyValue &proto);
        std::string to_std_string() const noexcept;
    };

    // Inline implementation of operator co_await
    inline auto AnyValue::operator co_await() const
    {
        return AnyValueAwaiter{*this};
    }

    // Global Constants for Optimization
    namespace Constants
    {
        inline const AnyValue UNINITIALIZED = AnyValue::make_uninitialized();
        inline const AnyValue UNDEFINED = AnyValue::make_undefined();
        inline const AnyValue Null = AnyValue::make_null();
        inline const AnyValue NaN = AnyValue::make_nan();
        inline const AnyValue TRUE = AnyValue::make_boolean(true);
        inline const AnyValue FALSE = AnyValue::make_boolean(false);
        inline const AnyValue ZERO = AnyValue::make_number(0.0);
        inline const AnyValue ONE = AnyValue::make_number(1.0);
    }
}