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

#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/generator.hpp"
#include "error.hpp"
#include "descriptors.hpp"

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
        Generator = 9,
        DataDescriptor = 10,
        AccessorDescriptor = 11,
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
            std::unique_ptr<std::string> str;
            std::shared_ptr<JsObject> object;
            std::shared_ptr<JsArray> array;
            std::shared_ptr<JsFunction> function;
            std::shared_ptr<JsGenerator<AnyValue>> generator;
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
                storage.str.~unique_ptr();
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
            case JsType::Generator:
                storage.generator.~shared_ptr();
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
                new (&storage.str) std::unique_ptr<std::string>(std::move(other.storage.str));
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
            case JsType::Generator:
                new (&storage.generator) std::shared_ptr<JsGenerator<AnyValue>>(std::move(other.storage.generator));
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
                new (&storage.str) std::unique_ptr<std::string>(std::make_unique<std::string>(*other.storage.str));
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
            case JsType::Generator:
                new (&storage.generator) std::shared_ptr<JsGenerator<AnyValue>>(other.storage.generator); // shallow copy
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
            new (&v.storage.str) std::unique_ptr<std::string>(std::make_unique<std::string>(raw_s));
            return v;
        }
        static AnyValue make_object(const std::map<std::string, AnyValue> &props) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Object;
            new (&v.storage.object) std::shared_ptr<JsObject>(std::make_shared<JsObject>(props));
            return v;
        }
        static AnyValue make_array(const std::vector<std::optional<AnyValue>> &dense) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Array;
            new (&v.storage.array) std::shared_ptr<JsArray>(std::make_shared<JsArray>(dense));
            return v;
        }
        static AnyValue make_function(const std::function<AnyValue(const std::vector<AnyValue> &)> &call, const std::string &name) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(call, name));
            return v;
        }
        template <typename Callable>
        static AnyValue make_generator_function(Callable &&call, const std::string &name) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            new (&v.storage.function) std::shared_ptr<JsFunction>(std::make_shared<JsFunction>(
                [func = std::forward<Callable>(call)](const std::vector<AnyValue> &args) mutable -> AnyValue
                { return AnyValue::from_generator(func(args)); },
                name));
            return v;
        }
        static AnyValue make_data_descriptor(const AnyValue &value, bool writable, bool enumerable, bool configurable) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::DataDescriptor;
            new (&v.storage.data_desc) std::shared_ptr<DataDescriptor>(std::make_shared<DataDescriptor>(std::make_shared<AnyValue>(value), writable, enumerable, configurable));
            return v;
        }
        static AnyValue make_accessor_descriptor(const std::optional<std::function<AnyValue(const std::vector<AnyValue> &)>> &get,
                                                 const std::optional<std::function<AnyValue(const std::vector<AnyValue> &)>> &set,
                                                 bool enumerable,
                                                 bool configurable) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::AccessorDescriptor;
            new (&v.storage.accessor_desc) std::shared_ptr<AccessorDescriptor>(std::make_shared<AccessorDescriptor>(get, set, enumerable, configurable));
            return v;
        }

        static AnyValue from_generator(JsGenerator<AnyValue> &&generator) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Generator;
            new (&v.storage.generator) std::shared_ptr<JsGenerator<AnyValue>>(std::make_shared<JsGenerator<AnyValue>>(std::move(generator)));
            return v;
        }

        // property resolution helpers ---------------------------------------
        static AnyValue resolve_property_for_read(const AnyValue &val) noexcept
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
                    return val.storage.accessor_desc->get.value()({});
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
        static AnyValue resolve_property_for_write(AnyValue &val, const AnyValue &new_val)
        {
            switch (val.storage.type)
            {
            case JsType::DataDescriptor:
            {
                *(val.storage.data_desc->value) = new_val;
                return new_val;
            }
            case JsType::AccessorDescriptor:
            {
                if (val.storage.accessor_desc->set.has_value())
                {
                    val.storage.accessor_desc->set.value()({new_val});
                    return new_val;
                }
                else
                {
                    throw RuntimeError::make_error("Cannot set property of #<Object> which has only a getter", "TypeError");
                }
            }
            default:
            {
                val = new_val;
                return new_val;
            }
            }
        }

        // type checkers and accessors ---------------------------------------
        bool is_number() const noexcept { return storage.type == JsType::Number; }
        bool is_string() const noexcept { return storage.type == JsType::String; }
        bool is_object() const noexcept { return storage.type == JsType::Object; }
        bool is_array() const noexcept { return storage.type == JsType::Array; }
        bool is_function() const noexcept { return storage.type == JsType::Function; }
        bool is_generator() const noexcept { return storage.type == JsType::Generator; }
        bool is_boolean() const noexcept { return storage.type == JsType::Boolean; }
        bool is_null() const noexcept { return storage.type == JsType::Null; }
        bool is_undefined() const noexcept { return storage.type == JsType::Undefined; }
        bool is_uninitialized() const noexcept { return storage.type == JsType::Uninitialized; }
        bool is_data_descriptor() const noexcept { return storage.type == JsType::DataDescriptor; }
        bool is_accessor_descriptor() const noexcept { return storage.type == JsType::AccessorDescriptor; }

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
        std::string *as_string() const noexcept
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
            throw RuntimeError::make_error(expression.value_or(to_std_string()) + " is not a function", "TypeError");
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

        // --- PROPERTY ACCESS OPERATORS
        AnyValue get_own_property(const std::string &key)
        {
            switch (storage.type)
            {
            case JsType::Object:
                return storage.object->get_property(key);
            case JsType::Array:
                return storage.array->get_property(key);
            case JsType::Function:
                return storage.function->get_property(key);
            case JsType::Generator:
                return storage.generator->get_property(key);
            case JsType::String:
            {
                // Check for prototype methods
                auto proto_fn = StringPrototypes::get(key, this->storage.str);
                if (proto_fn.has_value())
                {
                    return resolve_property_for_read(proto_fn.value());
                }
                // Handle character access by string index (e.g., "abc"["1"])
                if (JsArray::is_array_index(key))
                {
                    uint32_t idx = static_cast<uint32_t>(std::stoull(key));
                    return get_own_property(idx);
                }
            }
            case JsType::Undefined:
                throw RuntimeError::make_error("Cannot read properties of undefined (reading '" + key + "')", "TypeError");
            case JsType::Null:
                throw RuntimeError::make_error("Cannot read properties of null (reading '" + key + "')", "TypeError");
            default:
                return AnyValue::make_undefined();
            }
        }
        AnyValue get_own_property(uint32_t idx) noexcept
        {
            switch (storage.type)
            {
            case JsType::Array:
                return storage.array->get_property(idx);
            case JsType::String: // Handle character access by index (e.g., "abc"[1])
            {
                if (idx < storage.str->length())
                {
                    return AnyValue::make_string(std::string(1, (*storage.str)[idx]));
                }
                return AnyValue::make_undefined();
            }
            default:
                return get_own_property(std::to_string(idx));
            }
        }
        AnyValue get_own_property(const AnyValue &key) noexcept
        {
            if (key.storage.type == JsType::Number && storage.type == JsType::Array)
                return storage.array->get_property(key.storage.number);
            return get_own_property(key.to_std_string());
        }
        // for setting values
        AnyValue set_own_property(const std::string &key, const AnyValue &value)
        {
            switch (storage.type)
            {
            case JsType::Object:
                return storage.object->set_property(key, value);
            case JsType::Array:
                return storage.array->set_property(key, value);
            case JsType::Function:
                return storage.function->set_property(key, value);
            case JsType::Undefined:
                throw RuntimeError::make_error("Cannot set properties of undefined (setting '" + key + "')", "TypeError");
            case JsType::Null:
                throw RuntimeError::make_error("Cannot set properties of null (setting '" + key + "')", "TypeError");
            default:
                return value;
            }
        }
        AnyValue set_own_property(uint32_t idx, const AnyValue &value)
        {
            if (storage.type == JsType::Array)
            {
                return storage.array->set_property(idx, value);
            }
            return set_own_property(std::to_string(idx), value);
        }
        AnyValue set_own_property(const AnyValue &key, const AnyValue &value)
        {
            if (key.storage.type == JsType::Number && storage.type == JsType::Array)
            {
                return storage.array->set_property(key.storage.number, value);
            }
            return set_own_property(key.to_std_string(), value);
        }

        // --- HELPERS
        const bool is_truthy() const noexcept
        {
            switch (storage.type)
            {
            case JsType::Boolean:
                return storage.boolean;
            case JsType::Number:
                return storage.number != 0.0;
            case JsType::String:
                return !storage.str->empty();
            default:
                return true;
            }
        }
        const bool is_strictly_equal_to(const AnyValue &other) const noexcept
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
                    return (*storage.str.get() == *other.storage.str.get());
                case JsType::Array:
                    return (storage.array == other.storage.array);
                case JsType::Object:
                    return (storage.object == other.storage.object);
                case JsType::Function:
                    return (storage.function == other.storage.function);
                case JsType::DataDescriptor:
                    return (resolve_property_for_read(*this).is_strictly_equal_to(resolve_property_for_read(other)));
                case JsType::AccessorDescriptor:
                    return (resolve_property_for_read(*this).is_strictly_equal_to(resolve_property_for_read(other)));
                default:
                    return true;
                }
            }
            return false;
        }
        const bool is_equal_to(const AnyValue &other) const noexcept
        {
            // Implements JavaScript's Abstract Equality Comparison Algorithm (==)
            // Step 1: If types are the same, use strict equality (===)
            if (storage.type == other.storage.type)
            {
                return is_strictly_equal_to(other);
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
                    const std::string &s = *other.as_string();
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
                return other.is_equal_to(*this);
            }
            // Step 6 & 7: boolean == any
            if (is_boolean())
            {
                // Convert boolean to number and re-compare
                return AnyValue::make_number(as_boolean() ? 1.0 : 0.0).is_equal_to(other);
            }
            if (other.is_boolean())
            {
                // Convert boolean to number and re-compare
                return is_equal_to(AnyValue::make_number(other.as_boolean() ? 1.0 : 0.0));
            }
            // Step 8 & 9: object == (string or number)
            if ((other.is_object() || other.is_array() || other.is_function()) && (is_string() || is_number()))
            {
                // Delegate to the other operand to avoid code duplication
                return other.is_equal_to(*this);
            }
            if ((is_object() || is_array() || is_function()) && (other.is_string() || other.is_number()))
            {
                // Convert object to primitive (string) and re-compare.
                // This is a simplification of JS's ToPrimitive which would try valueOf() first.
                return AnyValue::make_string(to_std_string()).is_equal_to(other);
            }
            // Step 10: Parse datacriptor or accessor descriptor to primitive and re-compare
            if (is_data_descriptor() || is_accessor_descriptor())
            {
                AnyValue prim = resolve_property_for_read(*this);
                return prim.is_equal_to(other);
            }
            // Step 11: All other cases (e.g., object == null) are false.
            return false;
        }

        const AnyValue is_strictly_equal_to_primitive(const AnyValue &other) const noexcept
        {
            return AnyValue::make_boolean(is_strictly_equal_to(other));
        }
        const AnyValue is_equal_to_primitive(const AnyValue &other) const noexcept
        {
            return AnyValue::make_boolean(is_equal_to(other));
        }
        const AnyValue not_strictly_equal_to_primitive(const AnyValue &other) const noexcept
        {
            return AnyValue::make_boolean(!is_strictly_equal_to(other));
        }
        const AnyValue not_equal_to_primitive(const AnyValue &other) const noexcept
        {
            return AnyValue::make_boolean(!is_equal_to(other));
        }

        const std::string to_std_string() const noexcept
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
                return *storage.str.get();
            case JsType::Object:
                return storage.object->to_std_string();
            case JsType::Array:
                return storage.array->to_std_string();
            case JsType::Function:
                return storage.function->to_std_string();
            case JsType::Generator:
                return storage.generator->to_std_string();
            case JsType::DataDescriptor:
                return storage.data_desc->value->to_std_string();
            case JsType::AccessorDescriptor:
            {
                if (storage.accessor_desc->get.has_value())
                    return storage.accessor_desc->get.value()({}).to_std_string();
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
    };
}