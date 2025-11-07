// any_value_nan_fallback.hpp
#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdlib> // malloc/free
#include <new>     // placement new
#include <type_traits>

#include "values/non-values.hpp"
#include "values/boolean.hpp"
#include "values/number.hpp"
#include "values/string.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"

namespace jspp
{

    // keep your JsType
    enum class JsType : uint8_t
    {
        Undefined,
        Null,
        Uninitialized,
        Boolean,
        Number,
        String,
        Object,
        Array,
        Function,
    };

    // A fallback tagged struct (same fields you had previously)
    struct TaggedValue
    {
        JsType type;
        union
        {
            JsUndefined undefined;
            JsNull null;
            JsUninitialized uninitialized;
            JsBoolean boolean;
            JsNumber number;
            JsString *str;
            JsObject *object;
            JsArray *array;
            JsFunction *function;
        };

        // default-construct as undefined
        TaggedValue() noexcept
        {
            type = JsType::Undefined;
            // placement-new the appropriate trivial members if needed
            // (we rely on these types being trivially constructible or POD-like)
        }
    };

    class AnyValue
    {
    private:
        // NaN-boxing constants
        static constexpr uint64_t QNAN_MASK = 0x7FF8000000000000ULL; // quiet NaN pattern
        static constexpr uint64_t TAG_SHIFT = 48;
        static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFFULL; // low 48 bits

        // storage: either 64-bit bitpattern (nan-box) OR a full TaggedValue
        union Storage
        {
            uint64_t bits;
            TaggedValue tagged;
            Storage() noexcept { bits = 0; } // default
            ~Storage() noexcept {}           // destruction handled manually if needed
        } storage;

        // Global runtime decision (computed on first use)
        static bool detect_nan_boxing() noexcept
        {
            // Must be on 64-bit pointers
            if (sizeof(void *) < 8)
                return false;

            // Quick runtime check whether pointers' high bits fit in 48-bit payload.
            // Allocate a few small blocks to see typical addresses returned by allocator.
            bool ok = true;
            for (int i = 0; i < 8; ++i)
            {
                void *p = std::malloc(1);
                if (!p)
                {
                    ok = false;
                    break;
                }
                uintptr_t bits = reinterpret_cast<uintptr_t>(p);
                std::free(p);
                if ((bits & ~PAYLOAD_MASK) != 0)
                {
                    ok = false;
                    break;
                }
            }
            return ok;
        }

        static bool nan_boxing_available() noexcept
        {
            static const bool available = detect_nan_boxing();
            return available;
        }

        // helpers for NaN-box form
        static inline uint64_t double_to_bits(double d) noexcept
        {
            uint64_t x;
            std::memcpy(&x, &d, sizeof(double));
            return x;
        }
        static inline double bits_to_double(uint64_t x) noexcept
        {
            double d;
            std::memcpy(&d, &x, sizeof(double));
            return d;
        }

        // internal constructors for setting storage in each mode
        void set_bits(uint64_t b) noexcept { storage.bits = b; }
        void set_tagged(const TaggedValue &t) noexcept
        {
            // placement-new to properly initialize union member
            new (&storage.tagged) TaggedValue(t);
        }

    public:
        // default ctor (Undefined)
        AnyValue() noexcept
        {
            if (nan_boxing_available())
            {
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Undefined) << TAG_SHIFT);
                set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Undefined;
                new (&storage.tagged) TaggedValue(tv);
            }
        }

        // factories -------------------------------------------------------
        static AnyValue from_number(double d) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                // if d is NOT a QNaN encoded pattern we just store its raw bits
                v.set_bits(double_to_bits(d));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Number;
                tv.number = JsNumber{d}; // assume JsNumber has constructor from double / field 'value'
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_undefined() noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Undefined) << TAG_SHIFT);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Undefined;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_null() noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Null) << TAG_SHIFT);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Null;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_boolean(bool b) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uint64_t payload = b ? 1ULL : 0ULL;
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Boolean) << TAG_SHIFT) | (payload & PAYLOAD_MASK);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Boolean;
                tv.boolean = JsBoolean{b};
                v.set_tagged(tv);
            }
            return v;
        }

        // pointer boxing (strings/objects/arrays/functions)
        static AnyValue make_string(JsString *s) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(s);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::String) << TAG_SHIFT) | (ptr_bits & PAYLOAD_MASK);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::String;
                tv.str = s;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_object(JsObject *o) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(o);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Object) << TAG_SHIFT) | (ptr_bits & PAYLOAD_MASK);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Object;
                tv.object = o;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_array(JsArray *a) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(a);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Array) << TAG_SHIFT) | (ptr_bits & PAYLOAD_MASK);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Array;
                tv.array = a;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_function(JsFunction *f) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(f);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                uint64_t bits = QNAN_MASK | (static_cast<uint64_t>(JsType::Function) << TAG_SHIFT) | (ptr_bits & PAYLOAD_MASK);
                v.set_bits(bits);
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Function;
                tv.function = f;
                v.set_tagged(tv);
            }
            return v;
        }

        // predicates -----------------------------------------------------
        bool is_number() const noexcept
        {
            if (nan_boxing_available())
            {
                // not a QNaN pattern => it's an actual double
                return (storage.bits & QNAN_MASK) != QNAN_MASK;
            }
            else
            {
                return storage.tagged.type == JsType::Number;
            }
        }

        bool is_string() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                return static_cast<JsType>(tag) == JsType::String;
            }
            else
            {
                return storage.tagged.type == JsType::String;
            }
        }

        bool is_object() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                return static_cast<JsType>(tag) == JsType::Object;
            }
            else
            {
                return storage.tagged.type == JsType::Object;
            }
        }

        bool is_boolean() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                return static_cast<JsType>(tag) == JsType::Boolean;
            }
            else
            {
                return storage.tagged.type == JsType::Boolean;
            }
        }

        bool is_null() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                return static_cast<JsType>(tag) == JsType::Null;
            }
            else
            {
                return storage.tagged.type == JsType::Null;
            }
        }

        bool is_undefined() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                return static_cast<JsType>(tag) == JsType::Undefined;
            }
            else
            {
                return storage.tagged.type == JsType::Undefined;
            }
        }

        // extractors ----------------------------------------------------
        double as_double() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_number());
                return bits_to_double(storage.bits);
            }
            else
            {
                // assume JsNumber stores the raw double in a member 'value'
                return storage.tagged.number.value;
            }
        }

        bool as_boolean() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_boolean());
                return (storage.bits & 1ULL) != 0;
            }
            else
            {
                return storage.tagged.boolean.value;
            }
        }

        JsString *as_string() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_string());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsString *>(payload);
            }
            else
            {
                return storage.tagged.str;
            }
        }

        JsObject *as_object() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_object());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsObject *>(payload);
            }
            else
            {
                return storage.tagged.object;
            }
        }

        JsArray *as_array() const noexcept
        {
            if (nan_boxing_available())
            {
                assert((storage.bits & QNAN_MASK) == QNAN_MASK);
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                assert(static_cast<JsType>(tag) == JsType::Array);
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsArray *>(payload);
            }
            else
            {
                return storage.tagged.array;
            }
        }

        JsFunction *as_function() const noexcept
        {
            if (nan_boxing_available())
            {
                assert((storage.bits & QNAN_MASK) == QNAN_MASK);
                uint64_t tag = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
                assert(static_cast<JsType>(tag) == JsType::Function);
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsFunction *>(payload);
            }
            else
            {
                return storage.tagged.function;
            }
        }

        // small utility: tell whether runtime chose NaN-boxing
        static bool runtime_using_nan_boxing() noexcept
        {
            return nan_boxing_available();
        }
    };

} // namespace jspp
