#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <sstream>
#include <iomanip>
#include <type_traits>

#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"

namespace jspp
{

    // Keep your JsType enum
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
            std::string *str;
            JsObject *object;
            JsArray *array;
            JsFunction *function;
        };

        TaggedValue() noexcept : type(JsType::Undefined), undefined{} {}
    };

    class AnyValue
    {
    private:
        TaggedValue storage;

    public:
        // default ctor (Undefined)
        AnyValue() noexcept : storage() {}

        // factories -------------------------------------------------------
        static AnyValue make_number(double d) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Number;
            v.storage.number = d;
            return v;
        }
        // explicit NaN factory
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

        // pointer boxing (strings/objects/arrays/functions)
        static AnyValue make_string(std::string *s) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::String;
            v.storage.str = s;
            return v;
        }
        static AnyValue make_string(std::string &raw_s) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::String;
            v.storage.str = &raw_s;
            return v;
        }

        static AnyValue make_object(JsObject *o) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Object;
            v.storage.object = o;
            return v;
        }
        static AnyValue make_object(std::unordered_map<std::string, AnyValue> &props) noexcept
        {
            // Heap-allocate to avoid dangling pointer
            auto *obj = new JsObject{props};
            return make_object(obj);
        }

        static AnyValue make_array(JsArray *a) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Array;
            v.storage.array = a;
            return v;
        }
        static AnyValue make_array(std::vector<AnyValue> &dense) noexcept
        {
            auto *arr = new JsArray{dense};
            return make_array(arr);
        }

        static AnyValue make_function(JsFunction *f) noexcept
        {
            AnyValue v;
            v.storage.type = JsType::Function;
            v.storage.function = f;
            return v;
        }
        static AnyValue make_function(std::function<AnyValue(const std::vector<AnyValue> &)> &call, const std::string &name) noexcept
        {
            auto *fn = new JsFunction{call, name};
            return make_function(fn);
        }

        // predicates -----------------------------------------------------
        bool is_number() const noexcept { return storage.type == JsType::Number; }
        bool is_string() const noexcept { return storage.type == JsType::String; }
        bool is_object() const noexcept { return storage.type == JsType::Object; }
        bool is_array() const noexcept { return storage.type == JsType::Array; }
        bool is_function() const noexcept { return storage.type == JsType::Function; }
        bool is_boolean() const noexcept { return storage.type == JsType::Boolean; }
        bool is_null() const noexcept { return storage.type == JsType::Null; }
        bool is_undefined() const noexcept { return storage.type == JsType::Undefined; }
        bool is_uninitialized() const noexcept { return storage.type == JsType::Uninitialized; }

        // extractors ----------------------------------------------------
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
            return storage.str;
        }

        JsObject *as_object() const noexcept
        {
            assert(is_object());
            return storage.object;
        }

        JsArray *as_array() const noexcept
        {
            assert(is_array());
            return storage.array;
        }

        JsFunction *as_function() const noexcept
        {
            assert(is_function());
            return storage.function;
        }

        // convert value to raw string
        std::string convert_to_raw_string() const noexcept
        {
            if (is_undefined())
                return "undefined";
            if (is_null())
                return "null";
            if (is_uninitialized())
                return "<uninitialized>";
            if (is_boolean())
                return as_boolean() ? "true" : "false";
            if (is_number())
            {
                auto value = as_double();
                if (std::isnan(value))
                {
                    return "NaN";
                }
                if (std::abs(value) >= 1e21 || (std::abs(value) > 0 && std::abs(value) < 1e-6))
                {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(4) << value;
                    return oss.str();
                }
                else
                {
                    std::ostringstream oss;
                    oss << std::setprecision(6) << std::fixed << value;
                    std::string s = oss.str();
                    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                    if (!s.empty() && s.back() == '.')
                    {
                        s.pop_back();
                    }
                    return s;
                }
            }
            if (is_string())
                return *as_string();
            if (is_object())
                return as_object()->to_raw_string();
            if (is_array())
                return as_array()->to_raw_string();
            if (is_function())
                return as_function()->to_raw_string();
            // should not be reached
            return "";
        }

        const AnyValue &operator[](const std::string &key) const noexcept
        {
            if (is_object())
                return (*as_object())[key];
            static AnyValue undefined = AnyValue{};
            return undefined;
        }
        const AnyValue &operator[](const AnyValue &key) const noexcept
        {
            return (*this)[key.convert_to_raw_string()];
        }
        // AnyValue get_property(const std::string &key) const noexcept {}
        // AnyValue get_property(const AnyValue &key) const noexcept {}
        // AnyValue set_property(const std::string &key) const noexcept {}
        // AnyValue set_property(const AnyValue &key) const noexcept {}
    };

} // namespace jspp