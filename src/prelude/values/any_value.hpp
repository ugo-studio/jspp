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
#include <unordered_map>
#include <vector>
#include <functional>
#include <cmath>

#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"

namespace jspp
{

    enum class JsType : uint8_t
    {
        Undefined = 0,
        Null,
        Uninitialized,
        Boolean,
        Number,
        String,
        Object,
        Array,
        Function,
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
            std::unique_ptr<JsObject> object;
            std::unique_ptr<JsArray> array;
            std::unique_ptr<JsFunction> function;
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
                storage.object.~unique_ptr();
                break;
            case JsType::Array:
                storage.array.~unique_ptr();
                break;
            case JsType::Function:
                storage.function.~unique_ptr();
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
                new (&storage.object) std::unique_ptr<JsObject>(std::move(other.storage.object));
                break;
            case JsType::Array:
                new (&storage.array) std::unique_ptr<JsArray>(std::move(other.storage.array));
                break;
            case JsType::Function:
                new (&storage.function) std::unique_ptr<JsFunction>(std::move(other.storage.function));
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
                new (&storage.object) std::unique_ptr<JsObject>(std::make_unique<JsObject>(*other.storage.object));
                break;
            case JsType::Array:
                new (&storage.array) std::unique_ptr<JsArray>(std::make_unique<JsArray>(*other.storage.array));
                break;
            case JsType::Function:
                new (&storage.function) std::unique_ptr<JsFunction>(std::make_unique<JsFunction>(*other.storage.function));
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
        static AnyValue make_object(const std::unordered_map<std::string, AnyValue> &props) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Object;
            new (&v.storage.object) std::unique_ptr<JsObject>(std::make_unique<JsObject>(props));
            return v;
        }
        static AnyValue make_array(const std::vector<AnyValue> &dense) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Array;
            new (&v.storage.array) std::unique_ptr<JsArray>(std::make_unique<JsArray>(dense));
            return v;
        }
        static AnyValue make_function(const std::function<AnyValue(const std::vector<AnyValue> &)> &call, const std::string &name) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            new (&v.storage.function) std::unique_ptr<JsFunction>(std::make_unique<JsFunction>(call, name));
            return v;
        }

        bool is_number() const noexcept { return storage.type == JsType::Number; }
        bool is_string() const noexcept { return storage.type == JsType::String; }
        bool is_object() const noexcept { return storage.type == JsType::Object; }
        bool is_array() const noexcept { return storage.type == JsType::Array; }
        bool is_function() const noexcept { return storage.type == JsType::Function; }
        bool is_boolean() const noexcept { return storage.type == JsType::Boolean; }
        bool is_null() const noexcept { return storage.type == JsType::Null; }
        bool is_undefined() const noexcept { return storage.type == JsType::Undefined; }
        bool is_uninitialized() const noexcept { return storage.type == JsType::Uninitialized; }

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
        JsFunction *as_function() const noexcept
        {
            assert(is_function());
            return storage.function.get();
        }

        std::string convert_to_raw_string() const noexcept
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
                return storage.object->to_raw_string();
            case JsType::Array:
                return storage.array->to_raw_string();
            case JsType::Function:
                return storage.object->to_raw_string();
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

        const AnyValue &operator[](const std::string &key) const noexcept
        {
            switch (storage.type)
            {
            case JsType::Object:
                return (*as_object())[key];
            case JsType::Array:
                return (*as_array())[key];
            case JsType::Function:
                return (*as_function())[key];
            default:
                static AnyValue undefined = AnyValue{};
                return undefined;
            }
        }
        const AnyValue &operator[](const AnyValue &key) const noexcept
        {
            return (*this)[key.convert_to_raw_string()];
        }

        // non-const property/index access
        AnyValue &operator[](const std::string &key)
        {
            switch (storage.type)
            {
            case JsType::Object:
                return (*as_object())[key];
            case JsType::Array:
                return (*as_array())[key];
            case JsType::Function:
                return (*as_function())[key];
            default:
                static AnyValue undefined = AnyValue{};
                return undefined;
            }
        }
        AnyValue &operator[](const AnyValue &key)
        {
            return (*this)[key.convert_to_raw_string()];
        }
    };
}