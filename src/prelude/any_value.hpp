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

namespace jspp
{
    // NAN BOXING SCHEME (64-bit)
    // -------------------------
    // Any value that is NOT a NaN double or is a "quiet" NaN is stored as a double.
    // We use the top bits of the NaN space for tagging.
    //
    // Any value >= 0xFFFC000000000000 is a special tag.
    //
    // Tagging (bits 48-63):
    // 0xFFFC: Pointer to HeapObject
    // 0xFFFD: Boolean (bit 0 is value)
    // 0xFFFE: Special (Undefined, Null, Uninitialized)

    class AnyValue
    {
    private:
        uint64_t storage;

        static constexpr uint64_t TAG_BASE = 0xFFFC000000000000ULL;
        static constexpr uint64_t TAG_POINTER = 0xFFFC000000000000ULL;
        static constexpr uint64_t TAG_BOOLEAN = 0xFFFD000000000000ULL;
        static constexpr uint64_t TAG_SPECIAL = 0xFFFE000000000000ULL;

        static constexpr uint64_t VAL_UNDEFINED = 0x1ULL;
        static constexpr uint64_t VAL_NULL = 0x2ULL;
        static constexpr uint64_t VAL_UNINITIALIZED = 0x3ULL;

        static constexpr uint64_t TAG_MASK = 0xFFFF000000000000ULL;
        static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFFULL;

        inline bool has_tag(uint64_t tag) const noexcept
        {
            return (storage & TAG_MASK) == tag;
        }

    public:
        inline HeapObject *get_ptr() const noexcept
        {
            return reinterpret_cast<HeapObject *>(storage & PAYLOAD_MASK);
        }

        // default ctor (Undefined)
        AnyValue() noexcept : storage(TAG_SPECIAL | VAL_UNDEFINED) {}

        explicit AnyValue(double d) noexcept
        {
            std::memcpy(&storage, &d, 8);
            if (storage >= TAG_BASE) [[unlikely]]
            {
                storage = 0x7FF8000000000000ULL;
            }
        }

        explicit AnyValue(bool b) noexcept : storage(TAG_BOOLEAN | (b ? 1 : 0)) {}

        AnyValue(const AnyValue &other) noexcept : storage(other.storage)
        {
            if (is_heap_object())
                get_ptr()->ref();
        }

        AnyValue(AnyValue &&other) noexcept : storage(other.storage)
        {
            other.storage = TAG_SPECIAL | VAL_UNDEFINED;
        }

        AnyValue &operator=(const AnyValue &other) noexcept
        {
            if (this != &other)
            {
                if (is_heap_object())
                    get_ptr()->deref();
                storage = other.storage;
                if (is_heap_object())
                    get_ptr()->ref();
            }
            return *this;
        }

        AnyValue &operator=(AnyValue &&other) noexcept
        {
            if (this != &other)
            {
                if (is_heap_object())
                    get_ptr()->deref();
                storage = other.storage;
                other.storage = TAG_SPECIAL | VAL_UNDEFINED;
            }
            return *this;
        }

        ~AnyValue()
        {
            if (is_heap_object())
                get_ptr()->deref();
        }

        // Assignments
        AnyValue &operator=(double val) noexcept
        {
            if (is_heap_object())
                get_ptr()->deref();
            std::memcpy(&storage, &val, 8);
            if (storage >= TAG_BASE) [[unlikely]]
            {
                storage = 0x7FF8000000000000ULL;
            }
            return *this;
        }

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
            return AnyValue(d);
        }
        static AnyValue make_nan() noexcept
        {
            AnyValue v;
            v.storage = 0x7FF8000000000000ULL;
            return v;
        }
        static AnyValue make_uninitialized() noexcept
        {
            AnyValue v;
            v.storage = TAG_SPECIAL | VAL_UNINITIALIZED;
            return v;
        }
        static AnyValue make_undefined() noexcept
        {
            AnyValue v;
            v.storage = TAG_SPECIAL | VAL_UNDEFINED;
            return v;
        }
        static AnyValue make_null() noexcept
        {
            AnyValue v;
            v.storage = TAG_SPECIAL | VAL_NULL;
            return v;
        }
        static AnyValue make_boolean(bool b) noexcept
        {
            return AnyValue(b);
        }

        static AnyValue make_string(const std::string &raw_s) noexcept;
        static AnyValue make_object(std::initializer_list<std::pair<std::string, AnyValue>> props) noexcept;
        static AnyValue make_object(const std::map<std::string, AnyValue> &props) noexcept;
        static AnyValue make_object_with_proto(std::initializer_list<std::pair<std::string, AnyValue>> props, AnyValue proto) noexcept;
        static AnyValue make_object_with_proto(const std::map<std::string, AnyValue> &props, AnyValue proto) noexcept;
        static AnyValue make_array(std::span<const AnyValue> dense) noexcept;
        static AnyValue make_array(const std::vector<AnyValue> &dense) noexcept;
        static AnyValue make_array(std::vector<AnyValue> &&dense) noexcept;
        static AnyValue make_array_with_proto(std::span<const AnyValue> dense, AnyValue proto) noexcept;
        static AnyValue make_array_with_proto(const std::vector<AnyValue> &dense, AnyValue proto) noexcept;
        static AnyValue make_array_with_proto(std::vector<AnyValue> &&dense, AnyValue proto) noexcept;
        static AnyValue make_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt, bool is_constructor = true) noexcept;
        static AnyValue make_class(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept;
        static AnyValue make_generator(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept;
        static AnyValue make_async_function(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept;
        static AnyValue make_async_generator(const JsFunctionCallable &call, const std::optional<std::string> &name = std::nullopt) noexcept;
        static AnyValue make_symbol(const std::string &description = "") noexcept;
        static AnyValue make_promise(const JsPromise &promise) noexcept;
        static AnyValue make_data_descriptor(AnyValue value, bool writable, bool enumerable, bool configurable) noexcept;
        static AnyValue make_accessor_descriptor(const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &get,
                                                 const std::optional<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>> &set,
                                                 bool enumerable,
                                                 bool configurable) noexcept;

        static AnyValue from_symbol(JsSymbol *sym) noexcept;
        static AnyValue from_string(JsString *str) noexcept;
        static AnyValue from_promise(JsPromise &&promise) noexcept;
        static AnyValue from_iterator(JsIterator<AnyValue> &&iterator) noexcept;
        static AnyValue from_iterator_ref(JsIterator<AnyValue> *iterator) noexcept;
        static AnyValue from_async_iterator(JsAsyncIterator<AnyValue> &&iterator) noexcept;

        // internal factory for wrapping raw heap object pointers
        static AnyValue from_ptr(HeapObject *ptr) noexcept
        {
            AnyValue v;
            v.storage = TAG_POINTER | reinterpret_cast<uint64_t>(ptr);
            ptr->ref();
            return v;
        }

        static AnyValue resolve_property_for_read(const AnyValue &val, const AnyValue &thisVal, const std::string &propName) noexcept;
        static AnyValue resolve_property_for_write(AnyValue &val, const AnyValue &thisVal, const AnyValue &new_val, const std::string &propName);

        // TYPE CHECKERS AND ACCESSORS ---------------------------------------
        inline JsType get_type() const noexcept
        {
            if (is_number())
                return JsType::Number;
            if (is_heap_object())
                return get_ptr()->get_heap_type();
            if (is_boolean())
                return JsType::Boolean;
            if (is_undefined())
                return JsType::Undefined;
            if (is_null())
                return JsType::Null;
            if (is_uninitialized())
                return JsType::Uninitialized;
            return JsType::Undefined;
        }

        inline bool is_number() const noexcept { return storage < TAG_BASE; }
        inline bool is_heap_object() const noexcept { return has_tag(TAG_POINTER); }
        inline bool is_string() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::String; }
        inline bool is_object() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Object; }
        inline bool is_array() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Array; }
        inline bool is_function() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Function; }
        inline bool is_iterator() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Iterator; }
        inline bool is_boolean() const noexcept { return has_tag(TAG_BOOLEAN); }
        inline bool is_symbol() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Symbol; }
        inline bool is_promise() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::Promise; }
        inline bool is_null() const noexcept { return storage == (TAG_SPECIAL | VAL_NULL); }
        inline bool is_undefined() const noexcept { return storage == (TAG_SPECIAL | VAL_UNDEFINED); }
        inline bool is_uninitialized() const noexcept { return storage == (TAG_SPECIAL | VAL_UNINITIALIZED); }
        inline bool is_data_descriptor() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::DataDescriptor; }
        inline bool is_accessor_descriptor() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::AccessorDescriptor; }
        inline bool is_async_iterator() const noexcept { return is_heap_object() && get_ptr()->get_heap_type() == JsType::AsyncIterator; }

        JsString *as_string() const noexcept;
        JsObject *as_object() const noexcept;
        JsArray *as_array() const noexcept;
        JsFunction *as_function() const noexcept;
        JsSymbol *as_symbol() const noexcept;
        JsPromise *as_promise() const noexcept;
        JsIterator<AnyValue> *as_iterator() const noexcept;
        JsAsyncIterator<AnyValue> *as_async_iterator() const noexcept;
        DataDescriptor *as_data_descriptor() const noexcept;
        AccessorDescriptor *as_accessor_descriptor() const noexcept;

        bool is_generator() const noexcept;

        double as_double() const noexcept
        {
            double d;
            std::memcpy(&d, &storage, 8);
            return d;
        }
        bool as_boolean() const noexcept
        {
            return static_cast<bool>(storage & 1);
        }

        auto operator co_await() const;

        bool has_property(const std::string &key) const;
        bool has_property(const char *key) const { return has_property(std::string(key)); }

        AnyValue get_own_property(const std::string &key) const;
        AnyValue get_own_property(const char *key) const { return get_own_property(std::string(key)); }
        AnyValue get_own_property(uint32_t idx) const;
        AnyValue get_own_property(int idx) const { return get_own_property(static_cast<uint32_t>(idx)); }
        AnyValue get_own_property(const AnyValue &key) const;

        AnyValue get_property_with_receiver(const std::string &key, const AnyValue &receiver) const;
        AnyValue get_property_with_receiver(const char *key, const AnyValue &receiver) const { return get_property_with_receiver(std::string(key), receiver); }

        AnyValue set_own_property(const std::string &key, const AnyValue &value) const;
        AnyValue set_own_property(const char *key, const AnyValue &value) const { return set_own_property(std::string(key), value); }
        AnyValue set_own_property(uint32_t idx, const AnyValue &value) const;
        AnyValue set_own_property(int idx, const AnyValue &value) const { return set_own_property(static_cast<uint32_t>(idx), value); }
        AnyValue set_own_property(const AnyValue &key, const AnyValue &value) const;

        AnyValue call_own_property(const std::string &key, std::span<const AnyValue> args) const;
        AnyValue call_own_property(const char *key, std::span<const AnyValue> args) const { return call_own_property(std::string(key), args); }
        AnyValue call_own_property(uint32_t idx, std::span<const AnyValue> args) const;
        AnyValue call_own_property(int idx, std::span<const AnyValue> args) const { return call_own_property(static_cast<uint32_t>(idx), args); }
        AnyValue call_own_property(const AnyValue &key, std::span<const AnyValue> args) const;

        void define_data_property(const std::string &key, const AnyValue &value);
        void define_data_property(const char *key, const AnyValue &value) { define_data_property(std::string(key), value); }
        void define_data_property(const AnyValue &key, const AnyValue &value);
        void define_data_property(const std::string &key, const AnyValue &value, bool writable, bool enumerable, bool configurable);
        void define_data_property(const char *key, const AnyValue &value, bool writable, bool enumerable, bool configurable) { define_data_property(std::string(key), value, writable, enumerable, configurable); }

        void define_getter(const std::string &key, const AnyValue &getter);
        void define_getter(const char *key, const AnyValue &getter) { define_getter(std::string(key), getter); }
        void define_getter(const AnyValue &key, const AnyValue &getter);

        void define_setter(const std::string &key, const AnyValue &setter);
        void define_setter(const char *key, const AnyValue &setter) { define_setter(std::string(key), setter); }
        void define_setter(const AnyValue &key, const AnyValue &setter);

        AnyValue call(const AnyValue &thisVal, std::span<const AnyValue> args, const std::optional<std::string> &expr = std::nullopt) const;
        AnyValue construct(std::span<const AnyValue> args, const std::optional<std::string> &name = std::nullopt) const;
        void set_prototype(const AnyValue &proto);
        std::string to_std_string() const;

        inline uint64_t get_storage() const noexcept { return storage; }
        inline void *get_raw_ptr() const noexcept { return reinterpret_cast<void *>(storage & PAYLOAD_MASK); }
    };

    struct AnyValueAwaiter
    {
        AnyValue value;
        bool await_ready();
        void await_suspend(std::coroutine_handle<> h);
        AnyValue await_resume();
    };

    inline auto AnyValue::operator co_await() const
    {
        return AnyValueAwaiter{*this};
    }

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
